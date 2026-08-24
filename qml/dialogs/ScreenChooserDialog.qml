import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wormhole
import "../components"
import "../components/controls"
import "../components/containers"

StyledRect {
    id: root

    property var selectedItem: null
    property int currentTab: 0 // 0: Displays, 1: Windows
    property bool includeCursor: true
    property bool rememberChoice: false

    signal accepted(var result)
    signal rejected()

    implicitWidth: 860
    implicitHeight: 590
    color: Colours.tPalette.m3surfaceContainer

    ScreensModel {
        id: screensModel
    }

    WindowsModel {
        id: windowsModel
    }

    // ================= 1. HEADER ROW (Top of outer container) =================
    RowLayout {
        id: headerRow
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Tokens.padding.large
        height: 44
        spacing: Tokens.spacing.medium

        StyledRect {
            implicitWidth: 40
            implicitHeight: 40
            radius: Tokens.rounding.medium
            color: Colours.palette.m3primaryContainer

            MaterialIcon {
                anchors.centerIn: parent
                text: root.currentTab === 0 ? "desktop_windows" : "window"
                fontStyle: Tokens.font.icon.medium
                color: Colours.palette.m3onPrimaryContainer
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            StyledText {
                text: qsTr("Share Screen or Window")
                font: Tokens.font.title.medium
                color: Colours.palette.m3onSurface
            }

            StyledText {
                text: AppController.appId.length > 0
                      ? qsTr("Choose what to share with %1").arg(AppController.appId)
                      : qsTr("Select a display or application window to share")
                font: Tokens.font.body.small
                color: Colours.palette.m3onSurfaceVariant
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        // Prism-style Pill Slider Selector
        SlidingSelector {
            id: tabSelector
            Layout.preferredWidth: 240
            buttonHeight: 36
            model: [
                { label: qsTr("Displays"), value: 0, icon: "desktop_windows" },
                { label: qsTr("Windows"), value: 1, icon: "window" }
            ]
            currentValue: root.currentTab
            onSelected: (val, idx) => {
                root.currentTab = val;
                root.selectedItem = null;
            }
        }
    }

    // ================= 2. INNER DARK CONTAINER (Center of outer container) =================
    StyledRect {
        id: innerBox
        anchors.top: headerRow.bottom
        anchors.topMargin: Tokens.spacing.medium
        anchors.bottom: bottomRow.top
        anchors.bottomMargin: Tokens.spacing.medium
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Tokens.padding.large
        anchors.rightMargin: Tokens.padding.large
        radius: Tokens.rounding.large
        color: Colours.palette.m3surfaceContainerLowest
        clip: true

        // Animated Sliding Container with Prism/Atlas Bounce
        Item {
            id: pageSlider
            anchors.fill: parent
            clip: true

            Item {
                id: pagesRow
                width: pageSlider.width * 2
                height: pageSlider.height
                x: -root.currentTab * pageSlider.width

                Behavior on x {
                    NumberAnimation {
                        duration: 420
                        easing.type: Easing.OutBack
                        easing.overshoot: 1.18
                    }
                }

                // ================= PAGE 1: DISPLAYS VIEW =================
                Item {
                    id: displaysPage
                    x: 0
                    y: 0
                    width: pageSlider.width
                    height: pageSlider.height

                    // Monitor Spatial Canvas
                    Item {
                        id: monitorCanvas
                        anchors.top: parent.top
                        anchors.bottom: summaryBar.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: Tokens.padding.medium
                        clip: true

                        readonly property real margin: 16
                        readonly property real availWidth: Math.max(100, width - margin * 2)
                        readonly property real availHeight: Math.max(100, height - margin * 2)
                        readonly property real canvasScale: Math.min(availWidth / screensModel.totalWidth, availHeight / screensModel.totalHeight)

                        readonly property real originX: (width - screensModel.totalWidth * canvasScale) / 2
                        readonly property real originY: (height - screensModel.totalHeight * canvasScale) / 2

                        // Spatial Monitor Boxes
                        Repeater {
                            model: screensModel

                            delegate: Item {
                                id: monitorItem

                                required property int index
                                required property string name
                                required property string description
                                required property string makeModel
                                required property string resolution
                                required property string refreshRate
                                required property real refreshRateHz
                                required property bool focused
                                required property string activeWorkspace
                                required property int posX
                                required property int posY
                                required property int effectiveWidth
                                required property int effectiveHeight
                                required property int rotationDeg

                                readonly property bool isSelected: root.selectedItem && root.selectedItem.type === "screen" && root.selectedItem.name === monitorItem.name

                                x: monitorCanvas.originX + (monitorItem.posX - screensModel.minX) * monitorCanvas.canvasScale
                                y: monitorCanvas.originY + (monitorItem.posY - screensModel.minY) * monitorCanvas.canvasScale
                                width: monitorItem.effectiveWidth * monitorCanvas.canvasScale
                                height: monitorItem.effectiveHeight * monitorCanvas.canvasScale

                                StyledRect {
                                    id: monitorBox
                                    anchors.fill: parent
                                    radius: Tokens.rounding.medium
                                    color: monitorItem.isSelected ? Colours.palette.m3primaryContainer : (monitorMouse.containsMouse ? Colours.palette.m3surfaceContainerHighest : Colours.palette.m3surfaceContainerHigh)
                                    border.color: monitorItem.isSelected ? Colours.palette.m3primary : (monitorMouse.containsMouse ? Colours.palette.m3outline : Colours.palette.m3outlineVariant)
                                    border.width: monitorItem.isSelected ? 2 : 1
                                    // High-resolution Screen Capture Snapshot
                                    StyledClippingRect {
                                        anchors.fill: parent
                                        radius: monitorBox.radius
                                        color: Colours.palette.m3surfaceContainerHigh

                                        Image {
                                            anchors.fill: parent
                                            source: "image://monitor/" + monitorItem.name
                                            fillMode: Image.PreserveAspectCrop
                                            asynchronous: true
                                            smooth: true
                                            mipmap: true
                                            opacity: status === Image.Ready ? (monitorItem.isSelected ? 0.88 : 1.0) : 0.0

                                            Behavior on opacity {
                                                Anim { type: Anim.DefaultEffects }
                                            }
                                        }
                                    }

                                    // Selection Tint Overlay with Matching Radius
                                    Rectangle {
                                        anchors.fill: parent
                                        radius: monitorBox.radius
                                        color: Colours.palette.m3primary
                                        opacity: monitorItem.isSelected ? 0.16 : 0.0
                                        Behavior on opacity {
                                            Anim { type: Anim.DefaultEffects }
                                        }
                                    }

                                    // Floating Bottom Info Pill (Clean, readable, un-obscured)
                                    StyledRect {
                                        anchors.left: parent.left
                                        anchors.bottom: parent.bottom
                                        anchors.margins: 8
                                        implicitWidth: pillLayout.implicitWidth + 12
                                        implicitHeight: 24
                                        radius: Tokens.rounding.full
                                        color: Qt.alpha(Colours.palette.m3surfaceContainerLowest, 0.85)

                                        RowLayout {
                                            id: pillLayout
                                            anchors.centerIn: parent
                                            spacing: 6

                                            StyledText {
                                                text: monitorItem.name
                                                font: Tokens.font.label.large
                                                color: monitorItem.isSelected ? Colours.palette.m3primary : Colours.palette.m3onSurface
                                            }

                                            StyledText {
                                                text: "•"
                                                font: Tokens.font.label.small
                                                color: Colours.palette.m3outline
                                            }

                                            StyledText {
                                                text: monitorItem.resolution
                                                font: Tokens.font.label.small
                                                color: Colours.palette.m3onSurfaceVariant
                                            }

                                            StyledRect {
                                                implicitWidth: wsText.implicitWidth + 8
                                                implicitHeight: 16
                                                radius: 8
                                                color: Colours.palette.m3surfaceContainerHighest

                                                StyledText {
                                                    id: wsText
                                                    anchors.centerIn: parent
                                                    text: qsTr("WS %1").arg(monitorItem.activeWorkspace)
                                                    font: Tokens.font.label.small
                                                    color: Colours.palette.m3onSurface
                                                }
                                            }
                                        }
                                    }

                                    // Selected Checkmark Badge (Top Right)
                                    StyledRect {
                                        visible: monitorItem.isSelected
                                        anchors.top: parent.top
                                        anchors.right: parent.right
                                        anchors.margins: 8
                                        implicitWidth: 24
                                        implicitHeight: 24
                                        radius: 12
                                        color: Colours.palette.m3primary

                                        MaterialIcon {
                                            anchors.centerIn: parent
                                            text: "check"
                                            color: Colours.palette.m3onPrimary
                                            pointSize: 15
                                        }
                                    }

                                    MouseArea {
                                        id: monitorMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            root.selectedItem = {
                                                type: "screen",
                                                name: monitorItem.name,
                                                makeModel: monitorItem.makeModel,
                                                resolution: monitorItem.resolution,
                                                refreshRate: monitorItem.refreshRate,
                                                activeWorkspace: monitorItem.activeWorkspace,
                                                x: monitorItem.posX,
                                                y: monitorItem.posY,
                                                fps: Math.max(1, Math.round(monitorItem.refreshRateHz))
                                            };
                                            screensModel.focusScreen(monitorItem.name);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Selected Monitor Summary Card (Anchored cleanly at bottom of inner container)
                    StyledRect {
                        id: summaryBar
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: Tokens.padding.medium
                        height: 38
                        radius: Tokens.rounding.medium
                        color: Colours.palette.m3surfaceContainerHigh

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Tokens.padding.large
                            anchors.rightMargin: Tokens.padding.large
                            spacing: Tokens.spacing.medium

                            MaterialIcon {
                                text: "info"
                                color: Colours.palette.m3primary
                                fontStyle: Tokens.font.icon.small
                            }

                            StyledText {
                                Layout.fillWidth: true
                                text: root.selectedItem && root.selectedItem.type === "screen"
                                      ? qsTr("Selected: %1 (%2 @ %3)").arg(root.selectedItem.name).arg(root.selectedItem.resolution).arg(root.selectedItem.refreshRate)
                                      : qsTr("Click a display in the canvas above to select it")
                                font: Tokens.font.body.medium
                                color: Colours.palette.m3onSurface
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                // ================= PAGE 2: WINDOWS VIEW =================
                Item {
                    id: windowsPage
                    x: pageSlider.width
                    y: 0
                    width: pageSlider.width
                    height: pageSlider.height

                    VerticalFadeGridView {
                        id: windowsGrid
                        anchors.fill: parent
                        anchors.margins: Tokens.padding.medium
                        cellWidth: (width - 12) / 2
                        cellHeight: 64
                        model: windowsModel

                        delegate: Item {
                            id: winDelegate
                            width: windowsGrid.cellWidth
                            height: windowsGrid.cellHeight

                            required property int index
                            required property string address
                            required property string title
                            required property string className
                            required property string initialClass
                            required property string iconName
                            required property string workspaceName
                            required property int winWidth
                            required property int winHeight

                            readonly property bool isSelected: root.selectedItem && root.selectedItem.type === "window" && root.selectedItem.address === winDelegate.address

                            StyledRect {
                                anchors.fill: parent
                                anchors.margins: 4
                                radius: Tokens.rounding.medium
                                color: winDelegate.isSelected ? Colours.palette.m3secondaryContainer : (winMouse.containsMouse ? Colours.palette.m3surfaceContainerHighest : Colours.palette.m3surfaceContainerHigh)
                                border.width: 0

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    anchors.topMargin: 6
                                    anchors.bottomMargin: 6
                                    spacing: 10

                                    Image {
                                        Layout.preferredWidth: 32
                                        Layout.preferredHeight: 32
                                        source: "image://icon/" + (winDelegate.iconName.length > 0 ? winDelegate.iconName : winDelegate.className)
                                        sourceSize: Qt.size(32, 32)
                                        fillMode: Image.PreserveAspectFit
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.alignment: Qt.AlignVCenter
                                        spacing: 1

                                        StyledText {
                                            Layout.fillWidth: true
                                            text: winDelegate.title.length > 0 ? winDelegate.title : winDelegate.className
                                            font: Tokens.font.title.small
                                            color: winDelegate.isSelected ? Colours.palette.m3onSecondaryContainer : Colours.palette.m3onSurface
                                            elide: Text.ElideRight
                                        }

                                        StyledText {
                                            Layout.fillWidth: true
                                            text: winDelegate.className
                                            font: Tokens.font.body.small
                                            color: winDelegate.isSelected ? Colours.palette.m3onSecondaryContainer : Colours.palette.m3onSurfaceVariant
                                            elide: Text.ElideRight
                                        }
                                    }

                                    StyledRect {
                                        implicitWidth: wsLabel.implicitWidth + 10
                                        implicitHeight: 20
                                        radius: 10
                                        color: winDelegate.isSelected ? Qt.alpha(Colours.palette.m3onSecondaryContainer, 0.15) : Colours.palette.m3surfaceContainerHighest
                                        Layout.alignment: Qt.AlignVCenter

                                        StyledText {
                                            id: wsLabel
                                            anchors.centerIn: parent
                                            text: qsTr("WS %1").arg(winDelegate.workspaceName)
                                            font: Tokens.font.label.small
                                            color: winDelegate.isSelected ? Colours.palette.m3onSecondaryContainer : Colours.palette.m3onSurfaceVariant
                                        }
                                    }

                                    // Selected Checkmark Badge
                                    StyledRect {
                                        visible: winDelegate.isSelected
                                        implicitWidth: 20
                                        implicitHeight: 20
                                        radius: 10
                                        color: Colours.palette.m3primary
                                        Layout.alignment: Qt.AlignVCenter

                                        MaterialIcon {
                                            anchors.centerIn: parent
                                            text: "check"
                                            color: Colours.palette.m3onPrimary
                                            pointSize: 13
                                        }
                                    }
                                }

                                MouseArea {
                                    id: winMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        root.selectedItem = {
                                            type: "window",
                                            address: winDelegate.address,
                                            title: winDelegate.title,
                                            className: winDelegate.className,
                                            iconName: winDelegate.iconName,
                                            workspace: winDelegate.workspaceName
                                        };
                                        windowsModel.focusWindow(winDelegate.address);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ================= 3. BOTTOM ACTIONS ROW (Bottom of outer container) =================
    RowLayout {
        id: bottomRow
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Tokens.padding.large
        height: 40
        spacing: Tokens.spacing.large

        StyledCheckBox {
            id: cursorCheck
            text: qsTr("Include pointer")
            checked: root.includeCursor
            onCheckedChanged: root.includeCursor = checked
        }

        StyledCheckBox {
            id: rememberCheck
            text: qsTr("Remember choice")
            checked: root.rememberChoice
            onCheckedChanged: root.rememberChoice = checked
        }

        Item { Layout.fillWidth: true }

        TextButton {
            text: qsTr("Cancel")
            onClicked: root.rejected()
        }

        TextButton {
            text: qsTr("Share")
            type: ButtonBase.Filled
            disabled: root.selectedItem === null
            onClicked: {
                if (root.selectedItem) {
                    let res = Object.assign({}, root.selectedItem);
                    res.cursorMode = root.includeCursor ? 2 : 1; // 2: Embedded, 1: Hidden
                    res.persist = root.rememberChoice;
                    root.accepted(res);
                }
            }
        }
    }
}
