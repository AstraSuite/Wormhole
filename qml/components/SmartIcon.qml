import QtQuick
import wormhole

Item {
    id: root

    property string iconName: ""
    property string defaultIcon: "security"
    property color color: Colours.palette.m3onPrimaryContainer
    property real size: 24

    implicitWidth: size
    implicitHeight: size

    readonly property string resolvedMaterialIcon: {
        if (!iconName || iconName.length === 0) return defaultIcon;
        const name = iconName.toLowerCase().replace("-symbolic", "").replace(".svg", "").replace(".png", "").trim();

        switch (name) {
        case "audio-input-microphone":
        case "microphone":
        case "mic":
        case "microphone-sensitivity-high":
        case "microphone-sensitivity-medium":
        case "microphone-sensitivity-low":
        case "microphone-sensitivity-muted":
            return "mic";
        case "camera-web":
        case "camera-video":
        case "camera":
        case "videocam":
        case "video-camera":
            return "videocam";
        case "video-display":
        case "display":
        case "monitor":
            return "desktop_windows";
        case "preferences-desktop-wallpaper":
        case "wallpaper":
        case "background":
            return "wallpaper";
        case "system-lock-screen":
        case "lock":
        case "security-high":
        case "security-medium":
        case "security-low":
        case "security":
            return "security";
        case "dialog-password":
        case "key":
        case "vpn_key":
        case "password":
            return "vpn_key";
        case "applications-other":
        case "apps":
        case "application-x-executable":
            return "apps";
        case "user-info":
        case "user-identity":
        case "account":
        case "account_circle":
        case "avatar-default":
            return "account_circle";
        case "install_desktop":
        case "desktop-install":
            return "install_desktop";
        case "help-browser":
        case "internet-web-browser":
        case "web-browser":
        case "browser":
            return "language";
        case "system-file-manager":
        case "document-open":
        case "folder":
        case "folder_open":
            return "folder_open";
        case "printer":
        case "print":
        case "printer-symbolic":
            return "print";
        case "mail-message-new":
        case "mail":
        case "email":
        case "mail-read":
        case "mail-unread":
            return "mail";
        case "preferences-system-notifications":
        case "notifications":
        case "notification":
        case "bell":
            return "notifications";
        case "input-keyboard":
        case "keyboard":
            return "keyboard";
        case "input-mouse":
        case "mouse":
            return "mouse";
        case "drive-harddisk":
        case "drive-removable-media":
        case "media-flash":
        case "usb":
            return "usb";
        case "location":
        case "location_on":
        case "find-location":
            return "location_on";
        case "badge":
            return "badge";
        case "face":
            return "face";
        case "check":
            return "check";
        case "close":
            return "close";
        case "info":
            return "info";
        case "settings":
            return "settings";
        default:
            if (name.includes("-") || name.includes(".") || name.includes("/")) {
                return "";
            }
            return name;
        }
    }

    // Freedesktop System Icon
    Image {
        id: sysImg
        anchors.centerIn: parent
        width: root.size
        height: root.size
        visible: root.resolvedMaterialIcon === "" && status === Image.Ready
        source: {
            if (root.resolvedMaterialIcon !== "" || !root.iconName || root.iconName.length === 0) return "";
            if (root.iconName.startsWith("image://") || root.iconName.startsWith("file://") || root.iconName.startsWith("/")) {
                return root.iconName;
            }
            return "image://icon/" + root.iconName;
        }
        fillMode: Image.PreserveAspectFit
        sourceSize.width: root.size * 2
        sourceSize.height: root.size * 2
        smooth: true
        mipmap: true
    }

    // Material Symbol
    MaterialIcon {
        id: matIcon
        anchors.centerIn: parent
        visible: root.resolvedMaterialIcon !== "" || !sysImg.visible
        text: root.resolvedMaterialIcon !== "" ? root.resolvedMaterialIcon : root.defaultIcon
        pointSize: root.size
        color: root.color
    }
}
