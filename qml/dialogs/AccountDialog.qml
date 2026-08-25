import QtQuick
import QtQuick.Layouts
import wormhole
import "../components"
import "../components/controls"
import "../components/containers"

StyledRect {
    id: root

    signal accepted(var result)
    signal rejected()

    implicitWidth: 460
    implicitHeight: 280
    radius: Tokens.rounding.extraLarge
    color: Colours.tPalette.m3surfaceContainer

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

                SmartIcon {
                    anchors.centerIn: parent
                    iconName: "account_circle"
                    size: 24
                    color: Colours.palette.m3onPrimaryContainer
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                StyledText {
                    text: qsTr("User Information Request")
                    font: Tokens.font.title.medium
                    color: Colours.palette.m3onSurface
                }

                StyledText {
                    text: AppController.appId.length > 0
                          ? qsTr("Permission requested by %1").arg(AppController.appId)
                          : qsTr("An application is requesting your profile details")
                    font: Tokens.font.body.small
                    color: Colours.palette.m3onSurfaceVariant
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        // Single Clean Profile Details Card
        StyledRect {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Tokens.rounding.large
            color: Colours.palette.m3surfaceContainerLowest

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Tokens.padding.medium
                spacing: Tokens.spacing.small

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Tokens.spacing.medium

                    StyledRect {
                        implicitWidth: 32
                        implicitHeight: 32
                        radius: 16
                        color: Colours.palette.m3secondaryContainer

                        SmartIcon {
                            anchors.centerIn: parent
                            iconName: "badge"
                            size: 18
                            color: Colours.palette.m3onSecondaryContainer
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        StyledText {
                            text: qsTr("User Account & Real Name")
                            font: Tokens.font.label.large
                            color: Colours.palette.m3onSurface
                        }
                        StyledText {
                            text: qsTr("Your system username and display name")
                            font: Tokens.font.body.small
                            color: Colours.palette.m3onSurfaceVariant
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Qt.alpha(Colours.palette.m3outlineVariant, 0.4)
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Tokens.spacing.medium

                    StyledRect {
                        implicitWidth: 32
                        implicitHeight: 32
                        radius: 16
                        color: Colours.palette.m3secondaryContainer

                        SmartIcon {
                            anchors.centerIn: parent
                            iconName: "face"
                            size: 18
                            color: Colours.palette.m3onSecondaryContainer
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        StyledText {
                            text: qsTr("User Avatar")
                            font: Tokens.font.label.large
                            color: Colours.palette.m3onSurface
                        }
                        StyledText {
                            text: qsTr("Your profile picture (~/.face)")
                            font: Tokens.font.body.small
                            color: Colours.palette.m3onSurfaceVariant
                        }
                    }
                }
            }
        }

        // Bottom Actions Row
        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spacing.medium

            Item { Layout.fillWidth: true }

            TextButton {
                text: qsTr("Deny")
                onClicked: root.rejected()
            }

            TextButton {
                text: qsTr("Allow")
                type: ButtonBase.Filled
                onClicked: root.accepted({ allow: true })
            }
        }
    }
}
