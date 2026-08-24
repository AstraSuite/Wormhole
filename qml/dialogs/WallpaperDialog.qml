import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import wormhole
import "../components"
import "../components/controls"
import "../components/containers"
import "../components/effects"

StyledRect {
    id: root

    signal accepted(var result)
    signal rejected()

    implicitWidth: 580
    implicitHeight: 460
    radius: Tokens.rounding.extraLarge
    color: Colours.tPalette.m3surfaceContainer

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Tokens.padding.large
        spacing: Tokens.spacing.medium

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spacing.medium

            StyledRect {
                implicitWidth: 44
                implicitHeight: 44
                radius: Tokens.rounding.medium
                color: Colours.palette.m3primaryContainer

                MaterialIcon {
                    anchors.centerIn: parent
                    text: "wallpaper"
                    fontStyle: Tokens.font.icon.medium
                    color: Colours.palette.m3onPrimaryContainer
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                StyledText {
                    text: qsTr("Set Wallpaper")
                    font: Tokens.font.title.medium
                    color: Colours.palette.m3onSurface
                }

                StyledText {
                    text: qsTr("Apply wallpaper with Caelestia")
                    font: Tokens.font.body.small
                    color: Colours.palette.m3onSurfaceVariant
                }
            }
        }

        // Full Inner Area Wallpaper Preview with Container Frame Mask
        Item {
            id: previewWrapper
            Layout.fillWidth: true
            Layout.fillHeight: true

            Item {
                id: maskTarget
                anchors.fill: parent
                layer.enabled: true
                visible: false

                Rectangle {
                    anchors.fill: parent
                    radius: Tokens.rounding.large
                    color: "black"
                }
            }

            Image {
                id: wallpaperImage
                anchors.fill: parent
                source: AppController.wallpaperUri
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                smooth: true
                mipmap: true

                layer.enabled: true
                layer.effect: Mask {
                    maskSource: maskTarget
                }
            }
        }

        // Action Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spacing.medium

            Item { Layout.fillWidth: true }

            TextButton {
                text: qsTr("Cancel")
                onClicked: root.rejected()
            }

            TextButton {
                text: qsTr("Set Wallpaper")
                type: ButtonBase.Filled
                onClicked: root.accepted({
                    success: true
                })
            }
        }
    }
}
