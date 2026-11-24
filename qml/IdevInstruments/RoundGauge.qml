// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import IdevInstruments

Item {
    id: root

    property Instrument instrument

    readonly property real size: Math.min(width, height)
    readonly property real dialRadius: size / 2 - 2
    readonly property bool large: size >= 240
    readonly property real borderWidth: Math.max(2, size / 80)
    readonly property real zoneWidth: Math.max(4, size * 0.035)
    readonly property real zoneRadius: dialRadius - borderWidth - 3
    readonly property real tickOuter: zoneRadius - zoneWidth - 3
    readonly property real majorLength: size * 0.075
    readonly property real minorLength: size * 0.035
    readonly property int labelPixelSize: large ? 20 : 12
    readonly property real labelRadius: tickOuter - majorLength - labelPixelSize * 0.85
    readonly property int minorDivisions: instrument ? Math.max(1, instrument.minorDivisions) : 1
    readonly property real minorStep: instrument ? instrument.majorStep / minorDivisions : 1
    readonly property real span: instrument ? instrument.maximum - instrument.minimum : 0
    readonly property int tickCount: instrument ? Math.floor(span / minorStep + 0.001) + 1 : 0
    readonly property int labelCount: instrument ? Math.floor(span / instrument.majorStep + 0.001) + 1 : 0
    readonly property real needleAngle: instrument ? angleOf(instrument.displayValue) : 0

    function angleOf(v) {
        return instrument.startAngle + instrument.ratioOf(v) * instrument.sweepAngle
    }

    implicitWidth: 240
    implicitHeight: 240

    Accessible.role: Accessible.Indicator
    Accessible.name: instrument ? instrument.label + " " + instrument.displayText + " " + instrument.unit : ""

    Rectangle {
        anchors.centerIn: parent
        width: root.dialRadius * 2
        height: width
        radius: width / 2
        color: InstrumentPalette.surface
        border.color: InstrumentPalette.surfaceRaised
        border.width: root.borderWidth
    }

    Repeater {
        model: root.instrument ? root.instrument.zones : []

        delegate: Shape {
            id: zone

            required property var modelData

            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                strokeColor: zone.modelData.color
                strokeWidth: root.zoneWidth
                fillColor: "transparent"
                capStyle: ShapePath.FlatCap

                PathAngleArc {
                    centerX: root.width / 2
                    centerY: root.height / 2
                    radiusX: root.zoneRadius - root.zoneWidth / 2
                    radiusY: radiusX
                    startAngle: root.angleOf(zone.modelData.from) - 90
                    sweepAngle: root.angleOf(zone.modelData.to) - root.angleOf(zone.modelData.from)
                }
            }
        }
    }

    Repeater {
        model: root.tickCount

        delegate: Item {
            id: tick

            required property int index
            readonly property bool major: index % root.minorDivisions === 0

            anchors.fill: parent
            rotation: root.angleOf(root.instrument.minimum + index * root.minorStep)

            Rectangle {
                x: (tick.width - width) / 2
                y: tick.height / 2 - root.tickOuter
                width: tick.major ? Math.max(2, root.size / 90) : Math.max(1, root.size / 180)
                height: tick.major ? root.majorLength : root.minorLength
                color: tick.major ? InstrumentPalette.foreground : InstrumentPalette.dim
                antialiasing: true
            }
        }
    }

    Repeater {
        model: root.labelCount

        delegate: Text {
            required property int index
            readonly property real tickValue: root.instrument.minimum + index * root.instrument.majorStep
            readonly property real radians: root.angleOf(tickValue) * Math.PI / 180

            x: root.width / 2 + root.labelRadius * Math.sin(radians) - width / 2
            y: root.height / 2 - root.labelRadius * Math.cos(radians) - height / 2
            text: root.instrument.scaleLabel(tickValue)
            color: InstrumentPalette.foreground
            font.pixelSize: root.labelPixelSize
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.height / 2 - root.dialRadius * 0.30 - height / 2
        visible: root.large
        text: root.instrument ? [root.instrument.label, root.instrument.scaleCaption].join(" ").trim() : ""
        color: InstrumentPalette.dim
        font.pixelSize: 16
    }

    Text {
        id: readout

        anchors.horizontalCenter: parent.horizontalCenter
        y: root.height / 2 + root.dialRadius * (root.large ? 0.42 : 0.45) - height / 2
        text: root.instrument ? root.instrument.displayText : ""
        color: root.instrument ? root.instrument.alertColor : InstrumentPalette.foreground
        font.pixelSize: root.large ? 40 : 20
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.height / 2 + root.dialRadius * (root.large ? 0.66 : 0.80) - height / 2
        text: !root.instrument ? "" : root.large ? root.instrument.unit
                                                 : root.instrument.label + " " + root.instrument.unit
        color: InstrumentPalette.dim
        font.pixelSize: root.large ? 16 : 12
    }

    Item {
        anchors.fill: parent
        visible: root.instrument !== null && root.instrument.valid
        rotation: root.needleAngle

        Rectangle {
            width: Math.max(3, root.size / 55)
            height: root.tickOuter - 2 + root.size * 0.06
            x: (parent.width - width) / 2
            y: parent.height / 2 - (root.tickOuter - 2)
            radius: width / 2
            color: InstrumentPalette.needle
            antialiasing: true
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.max(5, root.size / 22) * 2
        height: width
        radius: width / 2
        color: InstrumentPalette.surfaceRaised
        border.color: InstrumentPalette.dim
        border.width: 2
    }
}
