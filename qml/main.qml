import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wormhole
import "components"
import "components/controls"
import "dialogs"

Window {
    id: window

    readonly property bool isFullscreenOverlay: AppController.dialogMode === AppController.Screenshot
    readonly property bool isDark: !Colours.light

    visible: true
    title: AppController.title.length > 0 ? AppController.title : qsTr("Wormhole")
    color: isFullscreenOverlay ? "transparent" : Colours.tPalette.m3surface

    flags: isFullscreenOverlay
           ? (Qt.Window | Qt.FramelessWindowHint | Qt.BypassWindowManagerHint)
           : (Qt.Dialog | Qt.WindowTitleHint | Qt.WindowCloseButtonHint)

    width: {
        if (isFullscreenOverlay) return Screen.width;
        if (activeLoader.item && activeLoader.item.implicitWidth > 0)
            return activeLoader.item.implicitWidth;
        return 680;
    }

    height: {
        if (isFullscreenOverlay) return Screen.height;
        if (activeLoader.item && activeLoader.item.implicitHeight > 0)
            return activeLoader.item.implicitHeight;
        return 520;
    }

    // Centering window
    Component.onCompleted: {
        if (isFullscreenOverlay) {
            window.showFullScreen();
        } else {
            window.x = (Screen.width - window.width) / 2;
            window.y = (Screen.height - window.height) / 2;
        }
    }

    Shortcut {
        sequence: "Escape"
        enabled: true
        context: Qt.ApplicationShortcut
        onActivated: {
            AppController.reject();
            Qt.quit();
        }
    }

    Loader {
        id: activeLoader
        anchors.fill: parent

        sourceComponent: {
            switch (AppController.dialogMode) {
            case AppController.ScreenCast:
                return screenChooserComp;
            case AppController.Screenshot:
                return screenshotComp;
            case AppController.AppChooser:
                return appChooserComp;
            case AppController.Access:
                return accessComp;
            case AppController.Account:
                return accountComp;
            case AppController.DynamicLauncher:
                return dynamicLauncherComp;
            case AppController.Wallpaper:
                return wallpaperComp;
            default:
                return screenChooserComp;
            }
        }
    }

    Component {
        id: screenChooserComp
        ScreenChooserDialog {
            onAccepted: (result) => {
                AppController.accept(result);
                Qt.quit();
            }
            onRejected: {
                AppController.reject();
                Qt.quit();
            }
        }
    }

    Component {
        id: screenshotComp
        ScreenshotDialog {
            onAccepted: (result) => {
                AppController.accept(result);
                Qt.quit();
            }
            onRejected: {
                AppController.reject();
                Qt.quit();
            }
        }
    }

    Component {
        id: appChooserComp
        AppChooserDialog {
            onAccepted: (result) => {
                AppController.accept(result);
                Qt.quit();
            }
            onRejected: {
                AppController.reject();
                Qt.quit();
            }
        }
    }

    Component {
        id: accessComp
        AccessDialog {
            onAccepted: (result) => {
                AppController.accept(result);
                Qt.quit();
            }
            onRejected: {
                AppController.reject();
                Qt.quit();
            }
        }
    }

    Component {
        id: accountComp
        AccountDialog {
            onAccepted: (result) => {
                AppController.accept(result);
                Qt.quit();
            }
            onRejected: {
                AppController.reject();
                Qt.quit();
            }
        }
    }

    Component {
        id: dynamicLauncherComp
        DynamicLauncherDialog {
            onAccepted: (result) => {
                AppController.accept(result);
                Qt.quit();
            }
            onRejected: {
                AppController.reject();
                Qt.quit();
            }
        }
    }

    Component {
        id: wallpaperComp
        WallpaperDialog {
            onAccepted: (result) => {
                AppController.accept(result);
                Qt.quit();
            }
            onRejected: {
                AppController.reject();
                Qt.quit();
            }
        }
    }
}
