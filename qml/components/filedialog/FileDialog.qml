import QtQuick
import QtQuick.Layouts
import wormhole
import "../"

StyledRect {
    id: root

    property var cwd: ["Home"]
    readonly property string currentPath: cwdToPath(cwd)
    property string initialDirectory: AppController.initialDirectory
    property string filterLabel: AppController.filterLabel
    property var filters: AppController.filters
    property string title: AppController.title || qsTr("Select a file")
    property bool showHidden: AppController.showHidden
    property bool directoryOnly: AppController.directoryOnly
    readonly property bool saveMode: AppController.saveMode
    property string saveName: AppController.suggestedName
    readonly property string savePath: currentPath === "/" ? "/" + saveName : currentPath + "/" + saveName
    readonly property bool saveWouldOverwrite: saveMode && saveName.length > 0 && AppController.fileExists(savePath)
    property real zoomLevel: 80
    property int viewMode: AppController.viewMode

    // Address-bar / search state, shared with the header bar and folder contents.
    property bool isEditingPath: false
    property bool isSearching: false
    property string searchText: ""
    readonly property string activeSearchQuery: isSearching ? searchText : ""
    readonly property bool isTextInputActive: isEditingPath || isSearching

    signal accepted(string path)
    signal rejected()

    function clearSearch() {
        isSearching = false;
        searchText = "";
        focusView();
    }

    function focusView() {
        if (folderContents)
            folderContents.focusView();
    }

    // Hand control from the search field back to the results view.
    function focusSearchResults() {
        if (folderContents) {
            folderContents.focusView();
            folderContents.selectIndex(0);
        }
    }

    function searchNavigateDown() {
        if (folderContents)
            folderContents.focusDown();
    }

    function searchNavigateUp() {
        if (folderContents)
            folderContents.focusUp();
    }

    function openSearch(initialChar) {
        headerBar.openSearch(initialChar);
    }

    function pathToCwd(fullPath) {
        if (!fullPath || fullPath === "") return ["Home"];
        let home = FileUtils.home;
        if (fullPath === home) return ["Home"];
        if (fullPath.startsWith(home + "/")) {
            let sub = fullPath.substring(home.length + 1).split("/").filter(s => s.length > 0);
            return ["Home"].concat(sub);
        }
        let parts = fullPath.split("/").filter(s => s.length > 0);
        return [""].concat(parts);
    }

    function cwdToPath(cwdArr) {
        if (!cwdArr || cwdArr.length === 0) return FileUtils.home;
        if (cwdArr[0] === "Home") {
            if (cwdArr.length === 1) return FileUtils.home;
            return FileUtils.home + "/" + cwdArr.slice(1).join("/");
        } else if (cwdArr[0] === "") {
            return "/" + cwdArr.slice(1).join("/");
        } else {
            return cwdArr.join("/");
        }
    }

    readonly property bool selectionValid: {
        const file = folderContents.currentItem?.modelData;
        if (directoryOnly) {
            return true;
        }
        if (!file)
            return false;
        if (file.isDir)
            return false;
        if (filters.includes("*"))
            return true;
        return filters.includes(file.suffix.toLowerCase()) || filters.includes(file.suffix);
    }

    Component.onCompleted: {
        if (AppController.initialDirectory && AppController.initialDirectory.length > 0) {
            cwd = pathToCwd(AppController.initialDirectory);
        }
    }

    Connections {
        target: AppController
        function onInitialDirectoryChanged() {
            if (AppController.initialDirectory && AppController.initialDirectory.length > 0) {
                root.cwd = root.pathToCwd(AppController.initialDirectory);
            }
        }
    }

    Connections {
        target: DriveManager
        function onDeviceMounted(mountPoint, tabIndex) {
            if (mountPoint && mountPoint.length > 0) {
                root.cwd = root.pathToCwd(mountPoint);
            }
        }
    }

    // Note: Escape/cancel is handled by the application-wide Shortcut in main.qml.
    // Do not register another Escape shortcut here, or the two become ambiguous
    // and neither fires.

    Shortcut {
        sequence: "Ctrl+H"
        enabled: root.visible
        context: Qt.ApplicationShortcut
        onActivated: root.showHidden = !root.showHidden
    }

    Shortcut {
        sequence: "Alt+."
        enabled: root.visible
        context: Qt.ApplicationShortcut
        onActivated: root.showHidden = !root.showHidden
    }

    Shortcut {
        sequence: "Alt+Up"
        enabled: root.visible
        context: Qt.ApplicationShortcut
        onActivated: {
            if (root.cwd.length > 1) {
                root.cwd = root.cwd.slice(0, root.cwd.length - 1);
            }
        }
    }

    // While the address bar or search field is active, Escape closes them
    // instead of rejecting the dialog. The global Escape shortcut in main.qml
    // is disabled in that state, so only one of the two is ever enabled.
    Shortcut {
        sequence: "Escape"
        enabled: root.visible && root.isTextInputActive
        context: Qt.ApplicationShortcut
        onActivated: {
            if (root.isSearching) {
                headerBar.closeSearch();
            } else if (headerBar.showSuggestions) {
                headerBar.showSuggestions = false;
            } else {
                root.isEditingPath = false;
                root.focusView();
            }
        }
    }

    // Open the file search bar (Atlas parity: Ctrl+F / F9).
    Shortcut {
        sequences: ["Ctrl+F", "F9", "Shift+F9"]
        enabled: root.visible
        context: Qt.ApplicationShortcut
        onActivated: root.openSearch()
    }

    // Focus the editable address bar (Atlas parity: Ctrl+L / Alt+D).
    Shortcut {
        sequence: "Ctrl+L"
        enabled: root.visible
        context: Qt.ApplicationShortcut
        onActivated: headerBar.openAddressEdit()
    }

    Shortcut {
        sequence: "Alt+D"
        enabled: root.visible
        context: Qt.ApplicationShortcut
        onActivated: headerBar.openAddressEdit()
    }

    Shortcut {
        sequence: "Ctrl+="
        enabled: root.visible
        context: Qt.ApplicationShortcut
        onActivated: root.zoomLevel = Math.min(180, root.zoomLevel + 16)
    }

    Shortcut {
        sequence: "Ctrl++"
        enabled: root.visible
        context: Qt.ApplicationShortcut
        onActivated: root.zoomLevel = Math.min(180, root.zoomLevel + 16)
    }

    Shortcut {
        sequence: "Ctrl+-"
        enabled: root.visible
        context: Qt.ApplicationShortcut
        onActivated: root.zoomLevel = Math.max(48, root.zoomLevel - 16)
    }

    Shortcut {
        sequence: "Ctrl+0"
        enabled: root.visible
        context: Qt.ApplicationShortcut
        onActivated: root.zoomLevel = 80
    }

    Shortcut {
        sequence: "Ctrl+1"
        enabled: root.visible
        context: Qt.ApplicationShortcut
        onActivated: AppController.viewMode = 0
    }

    Shortcut {
        sequence: "Ctrl+2"
        enabled: root.visible
        context: Qt.ApplicationShortcut
        onActivated: AppController.viewMode = 1
    }

    Connections {
        target: AppController
        function onViewModeChanged() {
            root.viewMode = AppController.viewMode;
        }
    }

    implicitWidth: 1000
    implicitHeight: 600
    color: Colours.tPalette.m3surface

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            Layout.fillHeight: true
            dialog: root
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            HeaderBar {
                id: headerBar

                Layout.fillWidth: true
                dialog: root
            }

            FolderContents {
                id: folderContents

                Layout.fillWidth: true
                Layout.fillHeight: true
                dialog: root
            }

            DialogButtons {
                Layout.fillWidth: true
                dialog: root
                folder: folderContents
            }
        }
    }
}
