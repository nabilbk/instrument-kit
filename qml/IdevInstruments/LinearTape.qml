// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import IdevInstruments

Item {
    id: root

    property Instrument instrument
    // false: ticks on the right edge (speed tape, left of the horizon); true: on the left.
    property bool mirrored: false

    readonly property int headerHeight: 26
    readonly property int zoneWidth: 6
    readonly property int majorLength: 14
    readonly property int minorLength: 7
    readonly property int boxHeight: 34
    readonly property color accent: instrument ? instrument.alertColor : InstrumentPalette.foreground

    implicitWidth: 96
    implicitHeight: 360

    Accessible.role: Accessible.Indicator
    Accessible.name: instrument ? instrument.label + " " + instrument.displayText + " " + instrument.unit : ""

    Rectangle {
        anchors.fill: parent
        radius: 4
        color: InstrumentPalette.surface
        border.color: InstrumentPalette.surfaceRaised
        border.width: 2
    }

    Text {
        width: root.width
        height: root.headerHeight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: root.instrument ? root.instrument.label + " " + root.instrument.unit : ""
        color: InstrumentPalette.dim
        font.pixelSize: 14
    }

    Item {
        id: scaleArea

        readonly property real ppu: root.instrument ? height / root.instrument.visibleRange : 1
        readonly property real shown: !root.instrument ? 0
                                      : root.instrument.valid ? root.instrument.displayValue
                                                              : root.instrument.minimum
        readonly property int divisions: root.instrument ? Math.max(1, root.instrument.tapeMinorDivisions) : 1
        readonly property real minorStep: root.instrument ? root.instrument.tapeMajorStep / divisions : 1
        readonly property real halfRange: root.instrument ? root.instrument.visibleRange / 2 + minorStep : 0
        readonly property int firstIndex: Math.floor((shown - halfRange) / minorStep)
        readonly property int tickCount: Math.ceil(2 * halfRange / minorStep) + 2
        readonly property real tickX: root.mirrored ? root.zoneWidth + 1 : width - root.zoneWidth - 1
        readonly property int direction: root.mirrored ? 1 : -1
        readonly property real labelWidth: width - root.zoneWidth - root.majorLength - 8

        x: 2
        y: root.headerHeight
        width: root.width - 4
        height: root.height - root.headerHeight - 2
        clip: true

        Repeater {
            model: root.instrument && root.instrument.valid ? root.instrument.zones : []

            delegate: Rectangle {
                required property var modelData

                x: root.mirrored ? 0 : scaleArea.width - root.zoneWidth
                y: scaleArea.height / 2 - (modelData.to - scaleArea.shown) * scaleArea.ppu
                width: root.zoneWidth
                height: (modelData.to - modelData.from) * scaleArea.ppu
                color: modelData.color
            }
        }

        Repeater {
            model: root.instrument && root.instrument.valid ? scaleArea.tickCount : 0

            delegate: Item {
                id: tick

                required property int index
                readonly property int tickIndex: scaleArea.firstIndex + index
                readonly property real tickValue: tickIndex * scaleArea.minorStep
                readonly property bool major: tickIndex % scaleArea.divisions === 0
                readonly property real labelQuotient: tickValue / root.instrument.labelStep
                readonly property bool labelled: Math.abs(labelQuotient - Math.round(labelQuotient)) < 1e-3

                y: Math.round(scaleArea.height / 2 - (tickValue - scaleArea.shown) * scaleArea.ppu)
                width: scaleArea.width
                visible: tickValue >= root.instrument.minimum - 1e-3 && tickValue <= root.instrument.maximum + 1e-3

                Rectangle {
                    x: root.mirrored ? scaleArea.tickX : scaleArea.tickX - width
                    y: -1
                    width: tick.major ? root.majorLength : root.minorLength
                    height: 2
                    color: tick.major ? InstrumentPalette.foreground : InstrumentPalette.dim
                }

                Text {
                    // No label half-hidden behind the readout box.
                    visible: tick.labelled && Math.abs(tick.y - scaleArea.height / 2) > root.boxHeight / 2 + 8
                    width: scaleArea.labelWidth
                    x: scaleArea.tickX + scaleArea.direction * (root.majorLength + 2) + (root.mirrored ? 0 : -width)
                    y: -height / 2
                    horizontalAlignment: Text.AlignHCenter
                    text: Math.round(tick.tickValue)
                    color: InstrumentPalette.foreground
                    font.pixelSize: 16
                }
            }
        }
    }

    Rectangle {
        id: box

        x: root.mirrored ? scaleArea.x + 12 : scaleArea.x + 4
        y: scaleArea.y + scaleArea.height / 2 - height / 2
        width: scaleArea.width - 16
        height: root.boxHeight
        radius: 3
        color: InstrumentPalette.background
        border.color: root.accent
        border.width: 2

        Text {
            anchors.centerIn: parent
            text: root.instrument ? root.instrument.displayText : ""
            color: root.instrument && root.instrument.valid ? root.accent : InstrumentPalette.warning
            font.pixelSize: 20
        }
    }

    Shape {
        id: pointer

        readonly property real baseX: root.mirrored ? box.x : box.x + box.width
        readonly property real centreY: box.y + box.height / 2

        anchors.fill: parent

        ShapePath {
            fillColor: root.accent
            strokeColor: "transparent"
            startX: pointer.baseX
            startY: pointer.centreY - 8

            PathLine { x: pointer.baseX; y: pointer.centreY + 8 }
            PathLine { x: pointer.baseX - (root.mirrored ? 10 : -10); y: pointer.centreY }
            PathLine { x: pointer.baseX; y: pointer.centreY - 8 }
        }
    }
}
