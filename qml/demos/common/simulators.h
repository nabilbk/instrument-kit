// SPDX-License-Identifier: MIT
#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include "idev/sim.h"

// Base: a clock that advances `time` (seconds) at 60 Hz while running,
// optionally stopping at `stopTime` (used for reproducible screenshots).
class SimulatorBase : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(qreal time READ time WRITE setTime NOTIFY timeChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(qreal stopTime READ stopTime WRITE setStopTime NOTIFY stopTimeChanged)
    Q_PROPERTY(bool sensorFault READ sensorFault WRITE setSensorFault NOTIFY sensorFaultChanged)

public:
    explicit SimulatorBase(QObject *parent = nullptr);

    qreal time() const { return m_time; }
    void setTime(qreal t);
    bool running() const { return m_timer.isActive(); }
    void setRunning(bool running);
    qreal stopTime() const { return m_stopTime; }
    void setStopTime(qreal t);
    bool sensorFault() const { return m_fault; }
    void setSensorFault(bool fault);

signals:
    void timeChanged();
    void runningChanged();
    void stopTimeChanged();
    void sensorFaultChanged();

protected:
    virtual void compute() {}

private:
    void advance();

    qreal m_time = 0.0;
    qreal m_stopTime = -1.0;
    bool m_fault = false;
    QTimer m_timer;
    QElapsedTimer m_clock;
};

class CarSimulator : public SimulatorBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal speed READ speed NOTIFY updated)
    Q_PROPERTY(qreal rpm READ rpm NOTIFY updated)
    Q_PROPERTY(qreal coolant READ coolant NOTIFY updated)
    Q_PROPERTY(qreal fuel READ fuel NOTIFY updated)
    Q_PROPERTY(qreal oilPressure READ oilPressure NOTIFY updated)
    Q_PROPERTY(bool brake READ brake NOTIFY updated)

public:
    explicit CarSimulator(QObject *parent = nullptr);
    qreal speed() const { return m_frame.speed_kmh; }
    qreal rpm() const;
    qreal coolant() const;
    qreal fuel() const { return m_frame.fuel_pct; }
    qreal oilPressure() const { return m_frame.oil_bar; }
    bool brake() const { return m_frame.brake; }

signals:
    void updated();

protected:
    void compute() override;

private:
    idev_car_frame_t m_frame{};
};

class PfdSimulator : public SimulatorBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal roll READ roll NOTIFY updated)
    Q_PROPERTY(qreal pitch READ pitch NOTIFY updated)
    Q_PROPERTY(qreal airspeed READ airspeed NOTIFY updated)
    Q_PROPERTY(qreal altitude READ altitude NOTIFY updated)
    Q_PROPERTY(bool autopilot READ autopilot NOTIFY updated)

public:
    explicit PfdSimulator(QObject *parent = nullptr);
    qreal roll() const { return m_frame.roll_deg; }
    qreal pitch() const;
    qreal airspeed() const { return m_frame.airspeed_kt; }
    qreal altitude() const;
    bool autopilot() const { return m_frame.autopilot; }

signals:
    void updated();

protected:
    void compute() override;

private:
    idev_pfd_frame_t m_frame{};
};
