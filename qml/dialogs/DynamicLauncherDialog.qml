import QtQuick
import QtQuick.Layouts
import wormhole
import "../components"
import "../components/controls"
import "../components/containers"
import "../components/effects"

StyledRect {
    id: root

    signal accepted(var result)
    signal rejected()

    implicitWidth: 460
    implicitHeight: 280
    radius: Tokens.rounding.extraLarge
    color: Colours.tPalette.m3surfaceContainer

    Elevation {
        anchors.fill: parent
        level: 3
        radius: root.radius
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Tokens.padding.large
        spacing: Tokens.spacing.medium

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spacing.medium

            StyledRect {
                implicitWidth: 48
                implicitHeight: 48
                radius: Tokens.rounding.medium
                color: Colours.palette.m3primaryContainer

                MaterialIcon {
                    anchors.centerIn: parent
                    text: "install_desktop"
                    fontStyle: Tokens.font.icon.large
                    color: Colours.palette.m3onPrimaryContainer
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                StyledText {
                    text: qsTr("Install Web Application")
                    font: Tokens.font.title.medium
                    color: Colours.palette.m3onSurface
                }

                StyledText {
                    text: AppController.launcherName.length > 0 ? AppController.launcherName : AppController.appId
                    font: Tokens.font.body.medium
                    color: Colours.palette.m3primary
                }
            }
        }

        StyledText {
            Layout.fillWidth: true
            Layout.fillHeight: true
            wrapMode: Text.WordWrap
            text: qsTr("Add \"%1\" to your application launcher? This will allow you to launch it directly from your desktop.").arg(AppController.launcherName)
            font: Tokens.font.body.large
            color: Colours.palette.m3onSurfaceVariant
        }

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
