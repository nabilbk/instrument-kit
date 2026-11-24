// SPDX-License-Identifier: MIT
#pragma once

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include "instrument.h"

// Blink / acknowledge logic of an annunciator (idev_annunciator_lit), so the
// QML item stays declarative. All annunciators share one clock and flash in phase.
class AnnunciatorState : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Instrument::Level level READ level WRITE setLevel NOTIFY levelChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(qreal blinkHz READ blinkHz WRITE setBlinkHz NOTIFY blinkHzChanged)
    Q_PROPERTY(bool acknowledged READ acknowledged NOTIFY acknowledgedChanged)
    Q_PROPERTY(bool lit READ lit NOTIFY litChanged)

public:
    explicit AnnunciatorState(QObject *parent = nullptr);

    Instrument::Level level() const { return m_level; }
    void setLevel(Instrument::Level level);
    bool active() const { return m_active; }
    void setActive(bool active);
    qreal blinkHz() const { return m_blinkHz; }
    void setBlinkHz(qreal hz);
    bool acknowledged() const { return m_acknowledged; }
    bool lit() const { return m_lit; }

    Q_INVOKABLE void acknowledge();

signals:
    void levelChanged();
    void activeChanged();
    void blinkHzChanged();
    void acknowledgedChanged();
    void litChanged();

private:
    void update();

    Instrument::Level m_level = Instrument::Normal;
    bool m_active = false;
    qreal m_blinkHz = 2.0;
    bool m_acknowledged = false;
    bool m_lit = false;
    QTimer m_timer;
};
