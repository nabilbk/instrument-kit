// SPDX-License-Identifier: MIT
#include "simulators.h"

#include <limits>

namespace {
constexpr qreal kNaN = std::numeric_limits<qreal>::quiet_NaN();
}

SimulatorBase::SimulatorBase(QObject *parent) : QObject(parent)
{
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(16);
    connect(&m_timer, &QTimer::timeout, this, &SimulatorBase::advance);
}

void SimulatorBase::setTime(qreal t)
{
    if (!qFuzzyCompare(t, m_time)) {
        m_time = t;
        emit timeChanged();
        compute();
    }
}

void SimulatorBase::setRunning(bool running)
{
    if (running == m_timer.isActive()) {
        return;
    }
    if (running) {
        m_clock.start();
        m_timer.start();
    } else {
        m_timer.stop();
    }
    emit runningChanged();
}

void SimulatorBase::setStopTime(qreal t)
{
    if (!qFuzzyCompare(t, m_stopTime)) {
        m_stopTime = t;
        emit stopTimeChanged();
    }
}

void SimulatorBase::setSensorFault(bool fault)
{
    if (fault != m_fault) {
        m_fault = fault;
        emit sensorFaultChanged();
        compute();
    }
}

void SimulatorBase::advance()
{
    qreal next = m_time + static_cast<qreal>(m_clock.restart()) / 1000.0;
    const bool stop = m_stopTime >= 0.0 && next >= m_stopTime;
    if (stop) {
        next = m_stopTime;
    }
    setTime(next);
    if (stop) {
        setRunning(false);
    }
}

CarSimulator::CarSimulator(QObject *parent) : SimulatorBase(parent)
{
    CarSimulator::compute();
}

void CarSimulator::compute()
{
    idev_sim_car(static_cast<float>(time()), &m_frame);
    emit updated();
}

qreal CarSimulator::rpm() const
{
    return sensorFault() ? kNaN : m_frame.rpm;
}

qreal CarSimulator::coolant() const
{
    return sensorFault() ? kNaN : m_frame.coolant_c;
}

PfdSimulator::PfdSimulator(QObject *parent) : SimulatorBase(parent)
{
    PfdSimulator::compute();
}

void PfdSimulator::compute()
{
    idev_sim_pfd(static_cast<float>(time()), &m_frame);
    emit updated();
}

qreal PfdSimulator::pitch() const
{
    return sensorFault() ? kNaN : m_frame.pitch_deg;
}

qreal PfdSimulator::altitude() const
{
    return sensorFault() ? kNaN : m_frame.altitude_ft;
}
