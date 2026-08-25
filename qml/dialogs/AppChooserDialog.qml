import QtQuick
import QtQuick.Layouts
import wormhole
import "../components"
import "../components/controls"
import "../components/containers"

StyledRect {
    id: root

    property string selectedDesktopId: ""
    property bool rememberChoice: false

    signal accepted(var result)
    signal rejected()

    implicitWidth: 580
    implicitHeight: 520
    color: Colours.tPalette.m3surfaceContainer

    AppChooserModel {
        id: appModel
        mimeType: AppController.appChooserMime
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Tokens.padding.large
        spacing: Tokens.spacing.medium

        // Header Row
        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spacing.medium

            StyledRect {
                implicitWidth: 40
                implicitHeight: 40
                radius: Tokens.rounding.medium
                color: Colours.palette.m3primaryContainer

                MaterialIcon {
                    anchors.centerIn: parent
                    text: "apps"
                    fontStyle: Tokens.font.icon.medium
                    color: Colours.palette.m3onPrimaryContainer
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                StyledText {
                    text: AppController.title.length > 0 ? AppController.title : qsTr("Choose an application")
                    font: Tokens.font.title.medium
                    color: Colours.palette.m3onSurface
                }

                StyledText {
                    text: AppController.appChooserMime.length > 0
                          ? qsTr("Select an application to open %1").arg(AppController.appChooserMime)
                          : qsTr("Select an application to open this file")
                    font: Tokens.font.body.small
                    color: Colours.palette.m3onSurfaceVariant
                }
            }
        }

        // Inner Container Frame
        StyledRect {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Tokens.rounding.large
            color: Colours.palette.m3surfaceContainerLowest
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Tokens.padding.medium
                spacing: Tokens.spacing.small

                // Search Bar
                SearchBar {
                    Layout.fillWidth: true
                    onTextChanged: appModel.searchQuery = text
                }

                // Apps List
                VerticalFadeListView {
                    id: appList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: appModel
                    spacing: Tokens.spacing.extraSmall
                    clip: true

                    delegate: Item {
                        width: appList.width
                        height: 56

                        readonly property bool isSelected: root.selectedDesktopId === model.desktopId

                        StyledRect {
                            anchors.fill: parent
                            radius: Tokens.rounding.medium
                            color: isSelected ? Colours.palette.m3primaryContainer : (appMouse.containsMouse ? Colours.palette.m3surfaceContainerHighest : Colours.palette.m3surfaceContainerHigh)
                            border.width: isSelected ? 2 : 1
                            border.color: isSelected ? Colours.palette.m3primary : "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: Tokens.padding.medium
                                spacing: Tokens.spacing.medium

                                Image {
                                    source: "image://icon/" + (model.iconName || "application-x-executable")
                                    sourceSize.width: 32
                                    sourceSize.height: 32
                                    Layout.preferredWidth: 32
                                    Layout.preferredHeight: 32
                                    fillMode: Image.PreserveAspectFit
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1

                                    RowLayout {
                                        spacing: Tokens.spacing.small

                                        StyledText {
                                            text: model.name
                                            font: Tokens.font.title.small
                                            color: isSelected ? Colours.palette.m3onPrimaryContainer : Colours.palette.m3onSurface
                                        }

                                        StyledRect {
                                            visible: model.isRecommended
                                            implicitHeight: 18
                                            implicitWidth: recText.implicitWidth + 8
                                            radius: Tokens.rounding.extraSmall
                                            color: Colours.palette.m3secondaryContainer

                                            StyledText {
                                                id: recText
                                                anchors.centerIn: parent
                                                text: qsTr("Recommended")
                                                font: Tokens.font.label.small
                                                color: Colours.palette.m3onSecondaryContainer
                                            }
                                        }
                                    }

                                    StyledText {
                                        Layout.fillWidth: true
                                        text: model.comment.length > 0 ? model.comment : model.desktopId
                                        font: Tokens.font.body.small
                                        elide: Text.ElideRight
                                        color: isSelected ? Colours.palette.m3onPrimaryContainer : Colours.palette.m3onSurfaceVariant
                                    }
                                }
                            }

                            MouseArea {
                                id: appMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.selectedDesktopId = model.desktopId
                                onDoubleClicked: {
                                    root.selectedDesktopId = model.desktopId;
                                    root.accepted({
                                        choice: root.selectedDesktopId,
                                        remember: root.rememberChoice
                                    });
                                }
                            }
                        }
                    }
                }
            }
        }

        // Bottom Controls
        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spacing.medium

            StyledCheckBox {
                id: rememberCheck
                text: qsTr("Always use this application")
                checked: root.rememberChoice
                onCheckedChanged: root.rememberChoice = checked
            }

            Item { Layout.fillWidth: true }

            TextButton {
                text: qsTr("Cancel")
                onClicked: root.rejected()
            }

            TextButton {
                text: qsTr("Open")
                type: ButtonBase.Filled
                disabled: root.selectedDesktopId === ""
                onClicked: {
                    if (root.selectedDesktopId !== "") {
                        root.accepted({
                            choice: root.selectedDesktopId,
                            remember: root.rememberChoice
                        });
                    }
                }
            }
        }
    }
}
