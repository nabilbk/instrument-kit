// SPDX-License-Identifier: MIT
import QtQuick
import IdevInstruments
import IdevDemoSim

Window {
    id: window

    property real startTime: 0
    property real stopTime: -1
    property bool sensorFault: false

    width: 800
    height: 480
    visible: true
    color: InstrumentPalette.background
    title: qsTr("IDEV instrument kit - primary flight display")

    PfdSimulator {
        id: sim

        time: window.startTime
        stopTime: window.stopTime
        sensorFault: window.sensorFault
        running: true
    }

    PrimaryFlightDisplay {
        anchors.fill: parent
        simulator: sim
    }
}
