// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import IdevInstruments

// Fixed aircraft reference of the attitude indicator (two wing bars and a centre dot).
// Its top edge (y = 0) is the reference line; height is the drop of the wing bars.
Item {
    id: root

    readonly property real barThickness: 5
    readonly property real outline: 2
    readonly property real wingLength: width / 3
    readonly property real drop: height

    Accessible.ignored: true

    Repeater {
        model: 2

        delegate: Item {
            id: wing

            required property int index
            readonly property bool isLeft: index === 0

            x: isLeft ? 0 : root.width - root.wingLength
            width: root.wingLength
            height: root.height

            Rectangle {
                x: -root.outline
                y: -root.barThickness / 2 - root.outline
                width: wing.width + 2 * root.outline
                height: root.barThickness + 2 * root.outline
                color: "black"
            }

            Rectangle {
                x: (wing.isLeft ? wing.width - root.barThickness : 0) - root.outline
                y: -root.barThickness / 2 - root.outline
                width: root.barThickness + 2 * root.outline
                height: root.drop + root.barThickness / 2 + 2 * root.outline
                color: "black"
            }

            Rectangle {
                y: -root.barThickness / 2
                width: wing.width
                height: root.barThickness
                color: InstrumentPalette.symbol
            }

            Rectangle {
                x: wing.isLeft ? wing.width - root.barThickness : 0
                y: -root.barThickness / 2
                width: root.barThickness
                height: root.drop + root.barThickness / 2
                color: InstrumentPalette.symbol
            }
        }
    }

    Rectangle {
        x: root.width / 2 - 6
        y: -6
        width: 12
        height: 12
        color: "black"

        Rectangle {
            anchors.centerIn: parent
            width: 8
            height: 8
            color: InstrumentPalette.symbol
        }
    }
}
