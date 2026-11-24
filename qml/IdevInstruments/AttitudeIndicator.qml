// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import IdevInstruments

Item {
    id: root

    property Instrument roll
    property Instrument pitch

    readonly property bool dataValid: roll !== null && pitch !== null && roll.valid && pitch.valid
    readonly property real rollAngle: roll ? roll.displayValue : 0
    readonly property real pitchAngle: pitch ? pitch.displayValue : 0
    readonly property real ppd: pitch ? height / pitch.pitchVisibleRange : 1
    readonly property real ladderStep: pitch && pitch.ladderStep > 0 ? pitch.ladderStep : 5
    readonly property real rollRadius: Math.min(width, height) * 0.40
    readonly property color pointerColor: roll ? roll.alertColor : InstrumentPalette.foreground

    implicitWidth: 360
    implicitHeight: 360

    Accessible.role: Accessible.Indicator
    Accessible.name: dataValid ? qsTr("Attitude: roll %1 degrees, pitch %2 degrees").arg(roll.displayText).arg(pitch.displayText)
                               : qsTr("Attitude data invalid")

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: root.dataValid ? InstrumentPalette.sky : InstrumentPalette.surface
        border.color: root.dataValid ? "transparent" : InstrumentPalette.surfaceRaised
        border.width: 2
    }

    Item {
        id: view

        anchors.fill: parent
        clip: true
        visible: root.dataValid

        Item {
            id: horizon

            width: (view.width + view.height) * 2
            height: width
            x: (view.width - width) / 2
            y: (view.height - height) / 2
            rotation: -root.rollAngle

            Item {
                id: pitchFrame

                anchors.fill: parent
                transform: Translate { y: root.pitchAngle * root.ppd }

                Rectangle {
                    y: pitchFrame.height / 2
                    width: pitchFrame.width
                    height: pitchFrame.height / 2
                    color: InstrumentPalette.ground
                }

                Rectangle {
                    y: pitchFrame.height / 2 - 1
                    width: pitchFrame.width
                    height: 2
                    color: InstrumentPalette.foreground
                }

                Repeater {
                    model: Math.round(180 / root.ladderStep) - 1

                    delegate: Item {
                        id: rung

                        required property int index
                        readonly property real rungPitch: -90 + (index + 1) * root.ladderStep
                        readonly property bool major: Math.abs(rungPitch % 10) < 0.01
                        readonly property real halfWidth: view.width * (major ? 0.14 : 0.06)

                        x: pitchFrame.width / 2
                        y: pitchFrame.height / 2 - rungPitch * root.ppd
                        // Rungs stay inside the roll scale so they never cross it.
                        visible: Math.abs(rungPitch) > 0.01
                                 && Math.abs(rungPitch - root.pitchAngle) * root.ppd <= root.rollRadius * 0.8

                        Rectangle {
                            x: -rung.halfWidth
                            y: -1
                            width: rung.halfWidth * 2
                            height: 2
                            color: InstrumentPalette.foreground
                        }

                        Repeater {
                            model: rung.major ? 2 : 0

                            delegate: Text {
                                required property int index

                                x: (index === 0 ? -1 : 1) * (rung.halfWidth + 18) - width / 2
                                y: -height / 2
                                text: Math.round(Math.abs(rung.rungPitch))
                                color: InstrumentPalette.foreground
                                font.pixelSize: 14
                            }
                        }
                    }
                }
            }
        }

        Shape {
            anchors.fill: parent

            ShapePath {
                strokeColor: InstrumentPalette.foreground
                strokeWidth: 2
                fillColor: "transparent"

                PathAngleArc {
                    centerX: view.width / 2
                    centerY: view.height / 2
                    radiusX: root.rollRadius
                    radiusY: root.rollRadius
                    startAngle: -150
                    sweepAngle: 120
                }
            }
        }

        Repeater {
            model: [-60, -45, -30, -20, -10, 10, 20, 30, 45, 60]

            delegate: Item {
                required property int modelData
                readonly property bool isLong: Math.abs(modelData) === 30 || Math.abs(modelData) === 60

                anchors.fill: parent
                rotation: modelData

                Rectangle {
                    x: (view.width - width) / 2
                    y: view.height / 2 - root.rollRadius - height
                    width: 2
                    height: parent.isLong ? 14 : 8
                    color: InstrumentPalette.foreground
                }
            }
        }

        Shape {
            id: zeroIndex

            readonly property real cx: view.width / 2
            readonly property real topY: view.height / 2 - root.rollRadius

            anchors.fill: parent

            ShapePath {
                fillColor: InstrumentPalette.foreground
                strokeColor: "transparent"
                startX: zeroIndex.cx
                startY: zeroIndex.topY - 2

                PathLine { x: zeroIndex.cx - 7; y: zeroIndex.topY - 14 }
                PathLine { x: zeroIndex.cx + 7; y: zeroIndex.topY - 14 }
                PathLine { x: zeroIndex.cx; y: zeroIndex.topY - 2 }
            }
        }

        Shape {
            id: skyPointer

            readonly property real cx: view.width / 2
            readonly property real topY: view.height / 2 - root.rollRadius

            anchors.fill: parent
            rotation: -root.rollAngle

            ShapePath {
                fillColor: root.pointerColor
                strokeColor: "transparent"
                startX: skyPointer.cx
                startY: skyPointer.topY + 2

                PathLine { x: skyPointer.cx - 9; y: skyPointer.topY + 17 }
                PathLine { x: skyPointer.cx + 9; y: skyPointer.topY + 17 }
                PathLine { x: skyPointer.cx; y: skyPointer.topY + 2 }
            }
        }

        AircraftSymbol {
            // y = 0 of the symbol is the horizon reference line through the view centre.
            x: view.width * 0.20
            y: view.height / 2
            width: view.width * 0.60
            height: view.width * 0.04
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 80
        height: 40
        radius: 3
        visible: !root.dataValid
        color: InstrumentPalette.warning

        Text {
            anchors.centerIn: parent
            text: qsTr("ATT")
            color: InstrumentPalette.background
            font.pixelSize: 28
            font.bold: true
        }
    }
}
