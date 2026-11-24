// SPDX-License-Identifier: MIT
import QtQuick
import IdevInstruments
import IdevDemoSim

// 800 x 480 primary flight display: horizon, airspeed and altitude tapes, alert lights.
// Same layout and bindings as lvgl/demos/cockpit_pfd.c.
Item {
    id: root

    required property PfdSimulator simulator

    Instrument { id: rollAxis; configId: "pfd.roll"; value: root.simulator.roll }
    Instrument { id: pitchAxis; configId: "pfd.pitch"; value: root.simulator.pitch }
    Instrument { id: airspeed; configId: "pfd.airspeed"; value: root.simulator.airspeed }
    Instrument { id: altitude; configId: "pfd.altitude"; value: root.simulator.altitude }

    Annunciator {
        x: 130; y: 14; width: 100; height: 36
        configId: "pfd.ann.ap"
        active: root.simulator.autopilot
    }
    Annunciator { x: 250; y: 14; width: 100; height: 36; configId: "pfd.ann.stall"; level: airspeed.alertLow }
    Annunciator { x: 370; y: 14; width: 100; height: 36; configId: "pfd.ann.bank"; level: rollAxis.alertLevel }
    Annunciator { x: 490; y: 14; width: 150; height: 36; configId: "pfd.ann.overspeed"; level: airspeed.alertHigh }

    LinearTape { x: 110; y: 70; width: 100; height: 390; instrument: airspeed }
    AttitudeIndicator { x: 222; y: 70; width: 356; height: 390; roll: rollAxis; pitch: pitchAxis }
    LinearTape { x: 590; y: 70; width: 110; height: 390; instrument: altitude; mirrored: true }
}
