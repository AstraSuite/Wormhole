import QtQuick
import QtQuick.Layouts
import wormhole
import "../components"
import "../components/controls"
import "../components/containers"
import "../components/effects"

Item {
    id: root

    property int captureMode: 0 // 0: Region, 1: Window, 2: Fullscreen, 3: ColorPicker
    property int delaySeconds: AppController.screenshotDelay
    property bool isSelecting: false
    property point startPoint: Qt.point(0, 0)
    property point currentPoint: Qt.point(0, 0)

    readonly property real selectionX: Math.min(startPoint.x, currentPoint.x)
    readonly property real selectionY: Math.min(startPoint.y, currentPoint.y)
    readonly property real selectionW: Math.abs(currentPoint.x - startPoint.x)
    readonly property real selectionH: Math.abs(currentPoint.y - startPoint.y)

    signal accepted(var result)
    signal rejected()

    anchors.fill: parent

    // Dark Scrim Background
    Rectangle {
        anchors.fill: parent
        color: Qt.alpha(Colours.palette.m3scrim, 0.45)
    }

    // Top Floating Toolbar
    StyledRect {
        id: toolbar
        anchors.top: parent.top
        anchors.topMargin: Tokens.padding.large
        anchors.horizontalCenter: parent.horizontalCenter
        implicitHeight: 48
        implicitWidth: toolbarLayout.implicitWidth + Tokens.padding.large * 2
        radius: Tokens.rounding.full
        color: Colours.tPalette.m3surfaceContainerHigh

        Elevation {
            anchors.fill: parent
            level: 4
            radius: parent.radius
        }

        RowLayout {
            id: toolbarLayout
            anchors.centerIn: parent
            spacing: Tokens.spacing.small

            IconButton {
                icon: "crop"
                type: root.captureMode === 0 ? ButtonBase.Filled : ButtonBase.Text
                activeColour: Colours.palette.m3primary
                onClicked: root.captureMode = 0
            }

            IconButton {
                icon: "web_asset"
                type: root.captureMode === 1 ? ButtonBase.Filled : ButtonBase.Text
                activeColour: Colours.palette.m3primary
                onClicked: root.captureMode = 1
            }

            IconButton {
                icon: "fullscreen"
                type: root.captureMode === 2 ? ButtonBase.Filled : ButtonBase.Text
                activeColour: Colours.palette.m3primary
                onClicked: root.captureMode = 2
            }

            IconButton {
                icon: "colorize"
                type: root.captureMode === 3 ? ButtonBase.Filled : ButtonBase.Text
                activeColour: Colours.palette.m3primary
                onClicked: root.captureMode = 3
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 24
                color: Colours.palette.m3outlineVariant
            }

            IconButton {
                icon: "close"
                type: ButtonBase.Text
                onClicked: root.rejected()
            }
        }
    }

    // Mouse Area for Region Drag Selection
    MouseArea {
        id: dragArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.captureMode === 3 ? Qt.CrossCursor : Qt.CrossCursor

        onPressed: (mouse) => {
            root.startPoint = Qt.point(mouse.x, mouse.y);
            root.currentPoint = Qt.point(mouse.x, mouse.y);
            root.isSelecting = true;
        }

        onPositionChanged: (mouse) => {
            if (root.isSelecting) {
                root.currentPoint = Qt.point(mouse.x, mouse.y);
            }
        }

        onReleased: (mouse) => {
            if (root.isSelecting) {
                root.currentPoint = Qt.point(mouse.x, mouse.y);
                root.isSelecting = false;

                if (root.captureMode === 3) {
                    // Pick Color Mode
                    root.accepted({
                        color: [0.8, 0.6, 1.0]
                    });
                } else if (root.selectionW > 4 && root.selectionH > 4) {
                    // Region Selected
                    root.accepted({
                        uri: "file:///tmp/wormhole-screenshot.png",
                        x: root.selectionX,
                        y: root.selectionY,
                        width: root.selectionW,
                        height: root.selectionH
                    });
                }
            }
        }
    }

    // Selection Rubberband Box
    Rectangle {
        visible: root.isSelecting || (root.selectionW > 4 && root.selectionH > 4)
        x: root.selectionX
        y: root.selectionY
        width: root.selectionW
        height: root.selectionH
        color: Qt.alpha(Colours.palette.m3primary, 0.15)
        border.width: 2
        border.color: Colours.palette.m3primary

        // Dimension Badge
        StyledRect {
            anchors.bottom: parent.top
            anchors.bottomMargin: 4
            anchors.left: parent.left
            implicitHeight: 22
            implicitWidth: dimText.implicitWidth + 12
            radius: Tokens.rounding.extraSmall
            color: Colours.palette.m3inverseSurface

            StyledText {
                id: dimText
                anchors.centerIn: parent
                text: qsTr("%1 × %2").arg(Math.round(root.selectionW)).arg(Math.round(root.selectionH))
                font: Tokens.font.label.small
                color: Colours.palette.m3inverseOnSurface
            }
        }
    }
}
