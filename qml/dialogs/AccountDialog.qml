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

    implicitWidth: 420
    implicitHeight: 260
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
                implicitWidth: 56
                implicitHeight: 56
                radius: Tokens.rounding.full
                color: Colours.palette.m3primaryContainer

                MaterialIcon {
                    anchors.centerIn: parent
                    text: "account_circle"
                    fontStyle: Tokens.font.icon.extraLarge
                    color: Colours.palette.m3onPrimaryContainer
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                StyledText {
                    text: qsTr("User Information")
                    font: Tokens.font.title.medium
                    color: Colours.palette.m3onSurface
                }

                StyledText {
                    text: qsTr("Share profile with %1").arg(AppController.appId)
                    font: Tokens.font.body.small
                    color: Colours.palette.m3onSurfaceVariant
                }
            }
        }

        StyledText {
            Layout.fillWidth: true
            Layout.fillHeight: true
            wrapMode: Text.WordWrap
            text: qsTr("Allow \"%1\" to read your username, display name, and avatar?").arg(AppController.appId)
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
                text: qsTr("Share Profile")
                type: ButtonBase.Filled
                onClicked: root.accepted({ allow: true })
            }
        }
    }
}
