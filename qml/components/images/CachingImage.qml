import QtQuick
import Quickshell
import wormhole

Image {
    id: root

    property string path

    asynchronous: true
    fillMode: Image.PreserveAspectCrop
    source: IUtils.urlForPath(path, fillMode)
    sourceSize: {
        const dpr = (QsWindow.window as QsWindow)?.devicePixelRatio ?? 1;
        return Qt.size(width * dpr, height * dpr);
    }
}
