// SPDX-License-Identifier: MIT
#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include "idev/instrument.h"

// One instrument value: configuration from spec/instruments.json (by configId),
// damping, alert level with hysteresis. Thin QObject wrapper around idev_core.
class Instrument : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString configId READ configId WRITE setConfigId NOTIFY configChanged)
    Q_PROPERTY(bool configured READ configured NOTIFY configChanged)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)
    Q_PROPERTY(qreal displayValue READ displayValue NOTIFY displayValueChanged)
    Q_PROPERTY(qreal ratio READ ratio NOTIFY displayValueChanged)
    Q_PROPERTY(QString displayText READ displayText NOTIFY displayValueChanged)
    Q_PROPERTY(Level alertLevel READ alertLevel NOTIFY alertChanged)
    Q_PROPERTY(Level alertHigh READ alertHigh NOTIFY alertChanged)
    Q_PROPERTY(Level alertLow READ alertLow NOTIFY alertChanged)
    Q_PROPERTY(QColor alertColor READ alertColor NOTIFY alertChanged)

    Q_PROPERTY(QString label READ label NOTIFY configChanged)
    Q_PROPERTY(QString unit READ unit NOTIFY configChanged)
    Q_PROPERTY(qreal minimum READ minimum NOTIFY configChanged)
    Q_PROPERTY(qreal maximum READ maximum NOTIFY configChanged)
    Q_PROPERTY(int decimals READ decimals NOTIFY configChanged)
    Q_PROPERTY(qreal dampingMs READ dampingMs NOTIFY configChanged)
    Q_PROPERTY(QVariantList zones READ zones NOTIFY configChanged)
    // RoundGauge
    Q_PROPERTY(qreal startAngle READ startAngle NOTIFY configChanged)
    Q_PROPERTY(qreal sweepAngle READ sweepAngle NOTIFY configChanged)
    Q_PROPERTY(qreal majorStep READ majorStep NOTIFY configChanged)
    Q_PROPERTY(int minorDivisions READ minorDivisions NOTIFY configChanged)
    Q_PROPERTY(QString scaleCaption READ scaleCaption NOTIFY configChanged)
    // LinearTape
    Q_PROPERTY(qreal visibleRange READ visibleRange NOTIFY configChanged)
    Q_PROPERTY(qreal tapeMajorStep READ tapeMajorStep NOTIFY configChanged)
    Q_PROPERTY(int tapeMinorDivisions READ tapeMinorDivisions NOTIFY configChanged)
    Q_PROPERTY(qreal labelStep READ labelStep NOTIFY configChanged)
    // AttitudeIndicator (pitch axis)
    Q_PROPERTY(qreal pitchVisibleRange READ pitchVisibleRange NOTIFY configChanged)
    Q_PROPERTY(qreal ladderStep READ ladderStep NOTIFY configChanged)
    // Annunciator
    Q_PROPERTY(qreal blinkHz READ blinkHz NOTIFY configChanged)

public:
    enum Level { Normal = IDEV_LEVEL_NORMAL, Caution = IDEV_LEVEL_CAUTION, Warning = IDEV_LEVEL_WARNING };
    Q_ENUM(Level)

    explicit Instrument(QObject *parent = nullptr);

    QString configId() const { return m_configId; }
    void setConfigId(const QString &id);
    bool configured() const { return m_state.cfg != nullptr; }

    qreal value() const { return m_value; }
    void setValue(qreal value);
    bool valid() const { return m_state.valid; }
    qreal displayValue() const { return m_state.display; }
    qreal ratio() const;
    QString displayText() const;
    Level alertLevel() const { return static_cast<Level>(m_state.alert); }
    Level alertHigh() const { return static_cast<Level>(idev_alert_high(&m_state)); }
    Level alertLow() const { return static_cast<Level>(idev_alert_low(&m_state)); }
    QColor alertColor() const;

    QString label() const;
    QString unit() const;
    qreal minimum() const { return cfg().min; }
    qreal maximum() const { return cfg().max; }
    int decimals() const { return cfg().decimals; }
    qreal dampingMs() const { return cfg().damping_ms; }
    QVariantList zones() const;
    qreal startAngle() const { return cfg().gauge.start_angle; }
    qreal sweepAngle() const { return cfg().gauge.sweep_angle; }
    qreal majorStep() const { return cfg().gauge.major_step; }
    int minorDivisions() const { return cfg().gauge.minor_divisions; }
    QString scaleCaption() const;
    qreal visibleRange() const { return cfg().tape.visible_range; }
    qreal tapeMajorStep() const { return cfg().tape.major_step; }
    int tapeMinorDivisions() const { return cfg().tape.minor_divisions; }
    qreal labelStep() const { return cfg().tape.label_step; }
    qreal pitchVisibleRange() const { return cfg().attitude.pitch_visible_range; }
    qreal ladderStep() const { return cfg().attitude.ladder_step; }
    qreal blinkHz() const { return cfg().annunciator.blink_hz; }

    Q_INVOKABLE qreal ratioOf(qreal value) const;
    Q_INVOKABLE QString scaleLabel(qreal value) const;

signals:
    void configChanged();
    void valueChanged();
    void validChanged();
    void displayValueChanged();
    void alertChanged();
    void unknownConfigId(const QString &id);

private:
    const idev_instrument_cfg_t &cfg() const;
    void apply(qreal value);
    void tick();

    QString m_configId;
    qreal m_value;
    idev_instrument_t m_state{};
    QTimer m_timer;
    QElapsedTimer m_clock;
};
