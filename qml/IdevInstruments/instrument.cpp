// SPDX-License-Identifier: MIT
#include "instrument.h"
#include "instrumentpalette.h"

#include <QVariantMap>
#include <QtGlobal>
#include <cmath>
#include <limits>

namespace {
// Neutral configuration used before configId is set, so getters never dereference null.
const idev_instrument_cfg_t kEmptyCfg = [] {
    idev_instrument_cfg_t c{};
    c.id = "";
    c.label = "";
    c.unit = "";
    c.max = 1.0f;
    c.gauge.major_step = 1.0f;
    c.gauge.minor_divisions = 1;
    c.gauge.label_scale = 1.0f;
    c.tape.visible_range = 1.0f;
    c.tape.major_step = 1.0f;
    c.tape.minor_divisions = 1;
    c.tape.label_step = 1.0f;
    c.attitude.pitch_visible_range = 40.0f;
    c.attitude.ladder_step = 5.0f;
    c.alerts = {NAN, NAN, NAN, NAN, 0.0f};
    return c;
}();
constexpr int kTickMs = 16; // 60 Hz damping steps
} // namespace

Instrument::Instrument(QObject *parent)
    : QObject(parent), m_value(std::numeric_limits<qreal>::quiet_NaN())
{
    idev_instrument_init(&m_state, nullptr);
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(kTickMs);
    connect(&m_timer, &QTimer::timeout, this, &Instrument::tick);
}

const idev_instrument_cfg_t &Instrument::cfg() const
{
    return m_state.cfg != nullptr ? *m_state.cfg : kEmptyCfg;
}

void Instrument::setConfigId(const QString &id)
{
    if (id == m_configId) {
        return;
    }
    m_configId = id;
    const idev_instrument_cfg_t *c = idev_cfg_find(id.toUtf8().constData());
    if (c == nullptr && !id.isEmpty()) {
        qWarning("Instrument: unknown configId '%s'", qPrintable(id));
        emit unknownConfigId(id);
    }
    idev_instrument_init(&m_state, c);
    emit configChanged();
    apply(m_value);
}

void Instrument::setValue(qreal value)
{
    // NaN means "no data"; compare bitwise so NaN -> NaN is not a change.
    if (value == m_value || (std::isnan(value) && std::isnan(m_value))) {
        return;
    }
    m_value = value;
    emit valueChanged();
    apply(value);
}

void Instrument::apply(qreal value)
{
    const bool wasValid = m_state.valid;
    const idev_level_t wasAlert = m_state.alert;
    const float wasDisplay = m_state.display;
    idev_instrument_set_value(&m_state, static_cast<float>(value));
    if (wasValid != m_state.valid) {
        emit validChanged();
    }
    if (wasAlert != m_state.alert) {
        emit alertChanged();
    }
    if (wasDisplay != m_state.display || wasValid != m_state.valid) {
        emit displayValueChanged();
    }
    if (idev_instrument_settling(&m_state) && !m_timer.isActive()) {
        m_clock.start();
        m_timer.start();
    }
}

void Instrument::tick()
{
    const qint64 dt = m_clock.restart();
    if (idev_instrument_step(&m_state, static_cast<float>(dt))) {
        emit displayValueChanged();
    }
    if (!idev_instrument_settling(&m_state)) {
        m_timer.stop();
    }
}

qreal Instrument::ratio() const
{
    return m_state.cfg != nullptr ? idev_ratio(m_state.cfg, m_state.display) : 0.0;
}

qreal Instrument::ratioOf(qreal value) const
{
    return idev_ratio(&cfg(), static_cast<float>(value));
}

QString Instrument::displayText() const
{
    char buf[32];
    idev_format(&cfg(), m_state.display, m_state.valid, buf, sizeof buf);
    return QString::fromUtf8(buf);
}

QString Instrument::scaleLabel(qreal value) const
{
    char buf[32];
    idev_format_scale_label(&cfg(), static_cast<float>(value), buf, sizeof buf);
    return QString::fromUtf8(buf);
}

QColor Instrument::alertColor() const
{
    if (m_state.alert == IDEV_LEVEL_NORMAL) {
        return InstrumentPalette::rgb(idev_palette.foreground);
    }
    return InstrumentPalette::rgb(idev_level_color(m_state.alert));
}

QString Instrument::label() const
{
    return QString::fromUtf8(cfg().label);
}

QString Instrument::unit() const
{
    return QString::fromUtf8(cfg().unit);
}

QString Instrument::scaleCaption() const
{
    return cfg().gauge.scale_caption != nullptr ? QString::fromUtf8(cfg().gauge.scale_caption)
                                                : QString();
}

QVariantList Instrument::zones() const
{
    QVariantList list;
    const idev_instrument_cfg_t &c = cfg();
    list.reserve(static_cast<qsizetype>(c.zone_count));
    for (size_t i = 0; i < c.zone_count; ++i) {
        const idev_zone_t &z = c.zones[i];
        list.append(QVariantMap{
            {QStringLiteral("from"), z.from},
            {QStringLiteral("to"), z.to},
            {QStringLiteral("level"), static_cast<int>(z.severity)},
            {QStringLiteral("color"), InstrumentPalette::rgb(idev_level_color(z.severity))},
        });
    }
    return list;
}
