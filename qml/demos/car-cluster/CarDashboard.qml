// SPDX-License-Identifier: MIT
import QtQuick
import IdevInstruments
import IdevDemoSim

// 800 x 480 cluster: speed and RPM dials, coolant and fuel gauges, warning lights.
// Same layout and bindings as lvgl/demos/car_cluster.c.
Item {
    id: root

    required property CarSimulator simulator

    Instrument { id: speed; configId: "car.speed"; value: root.simulator.speed }
    Instrument { id: rpm; configId: "car.rpm"; value: root.simulator.rpm }
    Instrument { id: coolant; configId: "car.coolant"; value: root.simulator.coolant }
    Instrument { id: fuel; configId: "car.fuel"; value: root.simulator.fuel }
    Instrument { id: oil; configId: "car.oilPressure"; value: root.simulator.oilPressure }

    RoundGauge { x: 24; y: 40; width: 330; height: 330; instrument: speed }
    RoundGauge { x: 446; y: 40; width: 330; height: 330; instrument: rpm }
    RoundGauge { x: 340; y: 70; width: 120; height: 120; instrument: coolant }
    RoundGauge { x: 340; y: 220; width: 120; height: 120; instrument: fuel }

    Row {
        x: 85
        y: 410
        spacing: 20

        Annunciator { width: 110; height: 40; configId: "car.ann.oil"; level: oil.alertLevel }
        Annunciator { width: 110; height: 40; configId: "car.ann.temp"; level: coolant.alertLevel }
        Annunciator { width: 110; height: 40; configId: "car.ann.fuel"; level: fuel.alertLevel }
        Annunciator {
            width: 110
            height: 40
            configId: "car.ann.brake"
            level: root.simulator.brake ? Instrument.Caution : Instrument.Normal
        }
        Annunciator { width: 110; height: 40; configId: "car.ann.speed"; level: speed.alertLevel }
    }
}
