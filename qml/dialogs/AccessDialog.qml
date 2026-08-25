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
    implicitHeight: 240
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
                    iconName: AppController.accessIcon
                    defaultIcon: "security"
                    size: 24
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
                    text: AppController.accessSubtitle.length > 0
                          ? AppController.accessSubtitle
                          : (AppController.appId.length > 0 ? qsTr("Requested by %1").arg(AppController.appId) : qsTr("System permission request"))
                    font: Tokens.font.body.small
                    color: Colours.palette.m3onSurfaceVariant
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        // Single Clean Description Card
        StyledRect {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Tokens.rounding.large
            color: Colours.palette.m3surfaceContainerLowest

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Tokens.padding.large
                spacing: Tokens.spacing.small

                StyledText {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    wrapMode: Text.WordWrap
                    verticalAlignment: Text.AlignVCenter
                    text: AppController.accessBody.length > 0
                          ? AppController.accessBody
                          : qsTr("The application \"%1\" is requesting access to your system features.").arg(AppController.appId.length > 0 ? AppController.appId : qsTr("Application"))
                    font: Tokens.font.body.medium
                    color: Colours.palette.m3onSurface
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
