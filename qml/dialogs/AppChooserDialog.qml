import QtQuick
import QtQuick.Layouts
import wormhole
import "../components"
import "../components/controls"
import "../components/containers"
import "../components/effects"

StyledRect {
    id: root

    property string selectedDesktopId: ""
    property bool rememberChoice: false

    signal accepted(var result)
    signal rejected()

    implicitWidth: 540
    implicitHeight: 520
    radius: Tokens.rounding.extraLarge
    color: Colours.tPalette.m3surfaceContainer

    Elevation {
        anchors.fill: parent
        level: 3
        radius: root.radius
    }

    AppChooserModel {
        id: appModel
        mimeType: AppController.appChooserMime
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Tokens.padding.large
        spacing: Tokens.spacing.medium

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spacing.small

            MaterialIcon {
                text: "apps"
                fontStyle: Tokens.font.icon.large
                color: Colours.palette.m3primary
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
                    color: isSelected ? Colours.palette.m3primaryContainer : Colours.palette.m3surfaceContainerHigh
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

                    StateLayer {
                        radius: parent.radius
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
