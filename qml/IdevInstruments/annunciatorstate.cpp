// SPDX-License-Identifier: MIT
#include "annunciatorstate.h"

#include <QElapsedTimer>

namespace {
uint32_t sharedClockMs()
{
    static QElapsedTimer clock = [] {
        QElapsedTimer c;
        c.start();
        return c;
    }();
    return static_cast<uint32_t>(clock.elapsed());
}
} // namespace

AnnunciatorState::AnnunciatorState(QObject *parent) : QObject(parent)
{
    m_timer.setInterval(50);
    connect(&m_timer, &QTimer::timeout, this, &AnnunciatorState::update);
    sharedClockMs();
}

void AnnunciatorState::setLevel(Instrument::Level level)
{
    if (level == m_level) {
        return;
    }
    if (level > m_level && m_acknowledged) {
        m_acknowledged = false; // a higher alert must be acknowledged again
        emit acknowledgedChanged();
    }
    m_level = level;
    emit levelChanged();
    update();
}

void AnnunciatorState::setActive(bool active)
{
    if (active != m_active) {
        m_active = active;
        emit activeChanged();
        update();
    }
}

void AnnunciatorState::setBlinkHz(qreal hz)
{
    if (!qFuzzyCompare(hz, m_blinkHz)) {
        m_blinkHz = hz;
        emit blinkHzChanged();
        update();
    }
}

void AnnunciatorState::acknowledge()
{
    if (!m_acknowledged) {
        m_acknowledged = true;
        emit acknowledgedChanged();
        update();
    }
}

void AnnunciatorState::update()
{
    const bool lit = idev_annunciator_lit(static_cast<idev_level_t>(m_level), m_active,
                                          m_acknowledged, sharedClockMs(),
                                          static_cast<float>(m_blinkHz));
    if (lit != m_lit) {
        m_lit = lit;
        emit litChanged();
    }
    const bool blinking = m_level == Instrument::Warning && !m_acknowledged;
    if (blinking && !m_timer.isActive()) {
        m_timer.start();
    } else if (!blinking) {
        m_timer.stop();
    }
}
