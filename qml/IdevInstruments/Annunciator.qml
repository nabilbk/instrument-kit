// SPDX-License-Identifier: MIT
import QtQuick
import IdevInstruments

// Alert light. NORMAL: dark (or steady green when `active`), CAUTION: steady amber,
// WARNING: red, flashing until tapped (acknowledged).
Rectangle {
    id: root

    property string configId
    property int level: Instrument.Normal
    property bool active: false
    readonly property alias acknowledged: logic.acknowledged
    readonly property alias lit: logic.lit
    readonly property color levelColor: InstrumentPalette.levelColor(level)
    readonly property bool off: level === Instrument.Normal && !active

    implicitWidth: 120
    implicitHeight: 40
    radius: 4
    border.width: 2
    color: logic.lit ? levelColor : InstrumentPalette.surface
    border.color: logic.lit || !off ? levelColor : InstrumentPalette.surfaceRaised

    Accessible.role: Accessible.Indicator
    Accessible.name: config.label
    Accessible.description: level === Instrument.Warning ? qsTr("warning")
                          : level === Instrument.Caution ? qsTr("caution")
                          : active ? qsTr("on") : qsTr("off")

    Instrument {
        id: config

        configId: root.configId
    }

    AnnunciatorState {
        id: logic

        level: root.level
        active: root.active
        blinkHz: config.blinkHz
    }

    Text {
        anchors.centerIn: parent
        text: config.label
        color: logic.lit ? InstrumentPalette.background : root.off ? InstrumentPalette.dim : root.levelColor
        font.pixelSize: 16
    }

    TapHandler {
        onTapped: logic.acknowledge()
    }
}
