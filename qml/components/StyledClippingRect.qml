import QtQuick

Rectangle {
    id: root

    color: "transparent"
    clip: true

    Behavior on color {
        CAnim {}
    }
}
