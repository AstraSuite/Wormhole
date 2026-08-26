import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wormhole
import "components"
import "components/controls"
import "components/filedialog"
import "dialogs"

Window {
    id: window

    readonly property bool isFullscreenOverlay: AppController.dialogMode === AppController.Screenshot
    readonly property bool isDaemon: AppController.dialogMode === AppController.None
    readonly property bool isDark: !Colours.light

    visible: AppController.dialogMode !== AppController.None
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

    function finishDialog(results, status) {
        console.log("finishDialog called: status=", status, "portalHandle=", AppController.portalHandle, "results=", JSON.stringify(results))
        if (AppController.portalHandle.length > 0) {
            console.log("finishDialog: calling finishPortalRequest")
            AppController.finishPortalRequest(AppController.portalHandle, status, results);
            AppController.portalHandle = "";
            AppController.dialogMode = AppController.None;
        } else {
            console.log("finishDialog: standalone mode")
            if (status === 0)
                AppController.accept(results);
            else
                AppController.reject();
            Qt.quit();
        }
    }

    Component.onCompleted: {
        if (isFullscreenOverlay) {
            window.showFullScreen();
        } else {
            window.x = (Screen.width - window.width) / 2;
            window.y = (Screen.height - window.height) / 2;
        }
    }

    onVisibleChanged: {
        if (visible && !isFullscreenOverlay) {
            window.x = (Screen.width - window.width) / 2;
            window.y = (Screen.height - window.height) / 2;
        }
    }

    Shortcut {
        sequence: "Escape"
        enabled: window.visible
        context: Qt.ApplicationShortcut
        onActivated: {
            finishDialog({}, 1);
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
            case AppController.FileChooser:
                return fileChooserComp;
            default:
                return null;
            }
        }
    }

    Component {
        id: screenChooserComp
        ScreenChooserDialog {
            onAccepted: (result) => window.finishDialog(result, 0)
            onRejected: window.finishDialog({}, 1)
        }
    }

    Component {
        id: screenshotComp
        ScreenshotDialog {
            onAccepted: (result) => window.finishDialog(result, 0)
            onRejected: window.finishDialog({}, 1)
        }
    }

    Component {
        id: appChooserComp
        AppChooserDialog {
            onAccepted: (result) => window.finishDialog(result, 0)
            onRejected: window.finishDialog({}, 1)
        }
    }

    Component {
        id: accessComp
        AccessDialog {
            onAccepted: (result) => window.finishDialog(result, 0)
            onRejected: window.finishDialog({}, 1)
        }
    }

    Component {
        id: accountComp
        AccountDialog {
            onAccepted: (result) => window.finishDialog(result, 0)
            onRejected: window.finishDialog({}, 1)
        }
    }

    Component {
        id: dynamicLauncherComp
        DynamicLauncherDialog {
            onAccepted: (result) => window.finishDialog(result, 0)
            onRejected: window.finishDialog({}, 1)
        }
    }

    Component {
        id: wallpaperComp
        WallpaperDialog {
            onAccepted: (result) => window.finishDialog(result, 0)
            onRejected: window.finishDialog({}, 1)
        }
    }

    Component {
        id: fileChooserComp
        FileDialog {
            implicitWidth: 1000
            implicitHeight: 600
            onAccepted: path => { console.log("FileDialog onAccepted: path=", path); window.finishDialog({ "uris": ["file://" + path] }, 0) }
            onRejected: { console.log("FileDialog onRejected"); window.finishDialog({}, 1) }
        }
    }
}
