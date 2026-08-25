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
    implicitHeight: 260
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
                    iconName: "install_desktop"
                    size: 24
                    color: Colours.palette.m3onPrimaryContainer
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                StyledText {
                    text: qsTr("Install Application")
                    font: Tokens.font.title.medium
                    color: Colours.palette.m3onSurface
                }

                StyledText {
                    text: qsTr("Add shortcut to your application launcher")
                    font: Tokens.font.body.small
                    color: Colours.palette.m3onSurfaceVariant
                }
            }
        }

        // Single Clean Launcher App Card
        StyledRect {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Tokens.rounding.large
            color: Colours.palette.m3surfaceContainerLowest

            RowLayout {
                anchors.fill: parent
                anchors.margins: Tokens.padding.large
                spacing: Tokens.spacing.medium

                StyledRect {
                    implicitWidth: 48
                    implicitHeight: 48
                    radius: Tokens.rounding.medium
                    color: Colours.palette.m3surfaceContainerHigh

                    SmartIcon {
                        anchors.centerIn: parent
                        iconName: AppController.launcherIcon.length > 0 ? AppController.launcherIcon : "applications-other"
                        defaultIcon: "apps"
                        size: 32
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 2

                    StyledText {
                        text: AppController.launcherName.length > 0 ? AppController.launcherName : AppController.appId
                        font: Tokens.font.title.small
                        color: Colours.palette.m3onSurface
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    StyledText {
                        text: AppController.launcherUrl.length > 0
                              ? AppController.launcherUrl
                              : (AppController.launcherExec.length > 0 ? AppController.launcherExec : AppController.appId)
                        font: Tokens.font.body.small
                        color: Colours.palette.m3primary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
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
                text: qsTr("Cancel")
                onClicked: root.rejected()
            }

            TextButton {
                text: qsTr("Install")
                type: ButtonBase.Filled
                onClicked: root.accepted({ install: true })
            }
        }
    }
}
