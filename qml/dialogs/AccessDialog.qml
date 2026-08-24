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

        // Icon + Title
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
                    text: AppController.accessIcon.length > 0 ? AppController.accessIcon : "security"
                    fontStyle: Tokens.font.icon.large
                    color: Colours.palette.m3onPrimaryContainer
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                StyledText {
                    text: AppController.title.length > 0 ? AppController.title : qsTr("Permission Request")
                    font: Tokens.font.title.medium
                    color: Colours.palette.m3onSurface
                }

                StyledText {
                    text: AppController.accessSubtitle.length > 0 ? AppController.accessSubtitle : AppController.appId
                    font: Tokens.font.body.medium
                    color: Colours.palette.m3primary
                }
            }
        }

        // Body Description
        StyledText {
            Layout.fillWidth: true
            Layout.fillHeight: true
            wrapMode: Text.WordWrap
            text: AppController.accessBody.length > 0
                  ? AppController.accessBody
                  : qsTr("The application \"%1\" is requesting access to your system features.").arg(AppController.appId)
            font: Tokens.font.body.large
            color: Colours.palette.m3onSurfaceVariant
        }

        // Action Buttons
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
