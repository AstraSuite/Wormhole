import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"

StyledRect {
    id: root

    required property var dialog

    // Autocomplete state. Address/edit/search visibility lives on the dialog so
    // the folder view can clear search when navigating to a result.
    property var pathSuggestions: []
    property int selectedSuggestionIndex: -1
    property bool showSuggestions: false

    // Keep the bar above the folder view while the suggestion dropdown is open.
    z: (root.dialog.isEditingPath && root.showSuggestions) ? 500 : 1

    readonly property string ghostCompletionText: {
        if (!root.dialog.isEditingPath || !pathInput.text || pathInput.text.length === 0)
            return "";
        let cur = root.dialog.currentPath;
        let comp = FileUtils.getCompletedPath(pathInput.text, cur);
        if (comp.startsWith(pathInput.text) && comp.length > pathInput.text.length) {
            return comp.substring(pathInput.text.length);
        }
        if (root.pathSuggestions.length > 0) {
            let firstSugg = root.pathSuggestions[0].displayPath;
            if (firstSugg.startsWith(pathInput.text) && firstSugg.length > pathInput.text.length) {
                return firstSugg.substring(pathInput.text.length);
            }
        }
        return "";
    }

    function updateSuggestions() {
        if (!root.dialog.isEditingPath) {
            showSuggestions = false;
            return;
        }
        pathSuggestions = FileUtils.getPathSuggestions(pathInput.text, root.dialog.currentPath);
        selectedSuggestionIndex = -1;
        showSuggestions = pathSuggestions.length > 0;
    }

    function applySuggestion(index) {
        if (index < 0 || index >= pathSuggestions.length)
            return;
        let item = pathSuggestions[index];
        pathInput.text = item.displayPath;
        pathInput.cursorPosition = pathInput.text.length;
        if (item.isDir) {
            updateSuggestions();
        } else {
            showSuggestions = false;
        }
    }

    function commitPath(customText) {
        let textToUse = (typeof customText === "string") ? customText : pathInput.text;
        if (selectedSuggestionIndex >= 0 && selectedSuggestionIndex < pathSuggestions.length && (!customText || customText.length === 0)) {
            textToUse = pathSuggestions[selectedSuggestionIndex].displayPath;
        }
        let trimmed = textToUse.trim();
        if (trimmed.length > 0) {
            let finalPath = FileUtils.expandPath(trimmed, root.dialog.currentPath);
            if (finalPath.length > 0) {
                root.dialog.cwd = root.dialog.pathToCwd(finalPath);
            }
        }
        showSuggestions = false;
        root.dialog.isEditingPath = false;
        root.dialog.focusView();
    }

    function openAddressEdit() {
        root.dialog.isSearching = false;
        pathInput.text = root.dialog.currentPath;
        pathInput.cursorPosition = pathInput.text.length;
        root.dialog.isEditingPath = true;
        pathInput.forceActiveFocus();
        pathInput.selectAll();
        updateSuggestions();
    }

    function openSearch(initialChar) {
        root.dialog.isEditingPath = false;
        showSuggestions = false;
        root.dialog.isSearching = true;
        if (typeof initialChar === "string" && initialChar.length > 0) {
            root.dialog.searchText = initialChar;
            searchInput.text = initialChar;
            searchInput.cursorPosition = searchInput.text.length;
        }
        searchInput.forceActiveFocus();
        searchInput.selectAll();
    }

    function closeSearch() {
        searchInput.text = "";
        root.dialog.clearSearch();
    }

    implicitWidth: inner.implicitWidth + Tokens.padding.medium * 2
    implicitHeight: inner.implicitHeight + Tokens.padding.medium * 2

    color: Colours.tPalette.m3surfaceContainer

    // Keep the search field in sync if something else clears the dialog state.
    Connections {
        target: root.dialog
        function onSearchTextChanged() {
            if (searchInput.text !== root.dialog.searchText)
                searchInput.text = root.dialog.searchText;
        }
        function onIsSearchingChanged() {
            if (!root.dialog.isSearching) {
                searchInput.text = "";
                root.showSuggestions = false;
            }
        }
        function onIsEditingPathChanged() {
            if (!root.dialog.isEditingPath)
                root.showSuggestions = false;
        }
    }

    // Measures the natural breadcrumb height so the bar keeps a stable height
    // while the breadcrumbs are hidden (edit/search mode).
    StyledText {
        id: barHeightProbe
        visible: false
        text: "X"
        font: Tokens.font.body.builders.small.weight(Font.Bold).build()
    }

    RowLayout {
        id: inner

        anchors.fill: parent
        anchors.margins: Tokens.padding.medium
        spacing: Tokens.spacing.small

        Item {
            implicitWidth: implicitHeight
            implicitHeight: upIcon.implicitHeight + Tokens.padding.small

            StateLayer {
                id: upFolderHover
                radius: Tokens.rounding.medium
                disabled: root.dialog.cwd.length <= 1
                onClicked: {
                    if (root.dialog.cwd.length > 1) {
                        let newCwd = root.dialog.cwd.slice(0, root.dialog.cwd.length - 1);
                        root.dialog.cwd = newCwd;
                    }
                }
            }

            MaterialIcon {
                id: upIcon

                anchors.centerIn: parent
                text: "drive_folder_upload"
                color: root.dialog.cwd.length <= 1 ? Colours.palette.m3outline : Colours.palette.m3onSurface
                grade: 200
            }

            StyledToolTip {
                text: qsTr("Go up")
                visible: upFolderHover.containsMouse && !upFolderHover.disabled
            }
        }

        StyledRect {
            id: pathBar

            Layout.fillWidth: true

            radius: Tokens.rounding.medium
            color: Colours.tPalette.m3surfaceContainerHigh
            border.color: (root.dialog.isEditingPath || root.dialog.isSearching) ? Colours.palette.m3primary : Qt.alpha(Colours.palette.m3primary, 0)
            border.width: (root.dialog.isEditingPath || root.dialog.isSearching) ? 1.5 : 0

            implicitHeight: Math.max(30, barHeightProbe.implicitHeight + Tokens.padding.small + Tokens.padding.extraSmall)

            // Normal breadcrumbs mode
            RowLayout {
                id: pathComponents

                anchors.fill: parent
                anchors.margins: Tokens.padding.extraSmall / 2
                anchors.leftMargin: 0

                visible: !root.dialog.isEditingPath && !root.dialog.isSearching

                spacing: Tokens.spacing.small

                Repeater {
                    model: root.dialog.cwd

                    RowLayout {
                        id: folder

                        required property string modelData
                        required property int index

                        spacing: 0

                        readonly property bool isActiveSegment: folder.index === root.dialog.cwd.length - 1

                        Loader {
                            Layout.rightMargin: Tokens.spacing.small
                            active: folder.index > 0
                            sourceComponent: StyledText {
                                text: "/"
                                color: Colours.palette.m3onSurfaceVariant
                                font: Tokens.font.body.builders.small.weight(Font.Bold).build()
                            }
                        }

                        Item {
                            implicitWidth: (homeIcon.active ? homeIcon.implicitWidth : (rootIcon.active ? rootIcon.implicitWidth : 0)) + ((homeIcon.active || rootIcon.active) ? Tokens.padding.extraSmall : 0) + folderName.implicitWidth + Tokens.padding.medium * 2
                            implicitHeight: folderName.implicitHeight + Tokens.padding.small

                            StateLayer {
                                radius: Tokens.rounding.medium
                                cursorShape: folder.isActiveSegment ? Qt.IBeamCursor : Qt.PointingHandCursor
                                onClicked: {
                                    if (folder.isActiveSegment) {
                                        root.openAddressEdit();
                                    } else {
                                        root.dialog.cwd = root.dialog.cwd.slice(0, folder.index + 1);
                                    }
                                }
                            }

                            Loader {
                                id: homeIcon

                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.leftMargin: Tokens.padding.medium

                                active: folder.index === 0 && folder.modelData === "Home"
                                sourceComponent: MaterialIcon {
                                    text: "home"
                                    color: folder.isActiveSegment ? Colours.palette.m3onSurface : Colours.palette.m3onSurfaceVariant
                                    fill: 1
                                }
                            }

                            Loader {
                                id: rootIcon

                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.leftMargin: Tokens.padding.medium

                                active: folder.index === 0 && folder.modelData === ""
                                sourceComponent: MaterialIcon {
                                    text: "hard_drive"
                                    color: folder.isActiveSegment ? Colours.palette.m3onSurface : Colours.palette.m3onSurfaceVariant
                                    fill: 1
                                }
                            }

                            StyledText {
                                id: folderName

                                anchors.left: homeIcon.active ? homeIcon.right : (rootIcon.active ? rootIcon.right : parent.left)
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.leftMargin: (homeIcon.active || rootIcon.active) ? Tokens.padding.extraSmall : Tokens.padding.medium

                                text: (folder.index === 0 && folder.modelData === "") ? qsTr("Root") : folder.modelData
                                color: folder.isActiveSegment ? Colours.palette.m3onSurface : Colours.palette.m3onSurfaceVariant
                                font: Tokens.font.body.builders.small.weight(Font.Bold).build()
                            }
                        }
                    }
                }

                // Clickable blank area to edit the path.
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.IBeamCursor
                        onClicked: root.openAddressEdit()
                    }
                }
            }

            // Editable address bar
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Tokens.padding.small
                anchors.rightMargin: Tokens.padding.small
                visible: root.dialog.isEditingPath && !root.dialog.isSearching
                spacing: Tokens.spacing.small

                MaterialIcon {
                    text: "folder"
                    color: Colours.palette.m3onSurfaceVariant
                    fontStyle: Tokens.font.icon.small
                }

                Item {
                    id: inputContainer
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    TextInput {
                        id: pathInput
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        color: Colours.palette.m3onSurface
                        selectionColor: Colours.palette.m3primaryContainer
                        selectedTextColor: Colours.palette.m3onPrimaryContainer
                        font: Tokens.font.body.small
                        selectByMouse: true
                        cursorVisible: focus
                        z: 2

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.IBeamCursor
                            acceptedButtons: Qt.NoButton
                        }

                        onTextChanged: {
                            if (root.dialog.isEditingPath && activeFocus)
                                root.updateSuggestions();
                        }

                        onActiveFocusChanged: {
                            if (activeFocus && root.dialog.isEditingPath)
                                root.updateSuggestions();
                        }

                        Keys.onTabPressed: event => {
                            if (root.ghostCompletionText.length > 0) {
                                pathInput.text = pathInput.text + root.ghostCompletionText;
                                pathInput.cursorPosition = pathInput.text.length;
                                root.updateSuggestions();
                                event.accepted = true;
                                return;
                            }
                            let comp = FileUtils.getCompletedPath(pathInput.text, root.dialog.currentPath);
                            if (comp !== pathInput.text) {
                                pathInput.text = comp;
                                pathInput.cursorPosition = pathInput.text.length;
                                root.updateSuggestions();
                            } else if (root.pathSuggestions.length > 0) {
                                root.showSuggestions = true;
                                root.selectedSuggestionIndex = (root.selectedSuggestionIndex + 1) % root.pathSuggestions.length;
                                pathInput.text = root.pathSuggestions[root.selectedSuggestionIndex].displayPath;
                                pathInput.cursorPosition = pathInput.text.length;
                            }
                            event.accepted = true;
                        }

                        Keys.onRightPressed: event => {
                            if (pathInput.cursorPosition === pathInput.text.length && root.ghostCompletionText.length > 0) {
                                pathInput.text = pathInput.text + root.ghostCompletionText;
                                pathInput.cursorPosition = pathInput.text.length;
                                root.updateSuggestions();
                                event.accepted = true;
                                return;
                            }
                            event.accepted = false;
                        }

                        Keys.onDownPressed: event => {
                            if (root.pathSuggestions.length > 0) {
                                root.showSuggestions = true;
                                root.selectedSuggestionIndex = (root.selectedSuggestionIndex + 1) % root.pathSuggestions.length;
                                event.accepted = true;
                            }
                        }

                        Keys.onUpPressed: event => {
                            if (root.pathSuggestions.length > 0) {
                                root.showSuggestions = true;
                                root.selectedSuggestionIndex = (root.selectedSuggestionIndex - 1 + root.pathSuggestions.length) % root.pathSuggestions.length;
                                event.accepted = true;
                            }
                        }

                        Keys.onEscapePressed: event => {
                            if (root.showSuggestions) {
                                root.showSuggestions = false;
                                event.accepted = true;
                            } else {
                                root.dialog.isEditingPath = false;
                                event.accepted = true;
                            }
                        }

                        onAccepted: root.commitPath()
                    }

                    StyledText {
                        id: ghostLabel
                        anchors.left: parent.left
                        anchors.leftMargin: pathInput.contentWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.ghostCompletionText
                        font: pathInput.font
                        color: Colours.palette.m3onSurfaceVariant
                        opacity: 0.45
                        visible: root.ghostCompletionText.length > 0 && pathInput.activeFocus && pathInput.cursorPosition === pathInput.text.length
                        z: 1
                    }
                }

                // Clear button
                Item {
                    implicitWidth: 22
                    implicitHeight: 22

                    MaterialIcon {
                        anchors.centerIn: parent
                        text: "backspace"
                        color: Colours.palette.m3onSurfaceVariant
                        fontStyle: Tokens.font.icon.small
                    }

                    MouseArea {
                        id: clearPathHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            pathInput.text = "";
                            root.updateSuggestions();
                            pathInput.forceActiveFocus();
                        }
                    }

                    StyledToolTip {
                        text: qsTr("Clear")
                        visible: clearPathHover.containsMouse
                    }
                }

                // Dropdown chevron
                Item {
                    implicitWidth: 22
                    implicitHeight: 22

                    MaterialIcon {
                        anchors.centerIn: parent
                        text: root.showSuggestions ? "expand_less" : "expand_more"
                        color: Colours.palette.m3onSurfaceVariant
                        fontStyle: Tokens.font.icon.small
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.showSuggestions = !root.showSuggestions;
                            if (root.showSuggestions)
                                root.updateSuggestions();
                        }
                    }
                }

                // Accept button
                Item {
                    implicitWidth: 22
                    implicitHeight: 22

                    MaterialIcon {
                        anchors.centerIn: parent
                        text: "check"
                        color: Colours.palette.m3onSurfaceVariant
                        fontStyle: Tokens.font.icon.small
                    }

                    MouseArea {
                        id: acceptPathHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.commitPath()
                    }

                    StyledToolTip {
                        text: qsTr("Go to location")
                        visible: acceptPathHover.containsMouse
                    }
                }
            }

            // Autocompletion suggestions dropdown
            StyledRect {
                id: suggestionsPopup

                visible: root.dialog.isEditingPath && root.showSuggestions && root.pathSuggestions.length > 0 && !root.dialog.isSearching
                z: 300
                y: parent.height + 4
                anchors.left: parent.left
                width: Math.max(parent.width, 360)
                implicitHeight: Math.min(suggestionsList.contentHeight + Tokens.padding.extraSmall * 2, 240)
                radius: Tokens.rounding.large
                color: Colours.palette.m3surfaceContainerLow
                border.color: Colours.palette.m3outlineVariant
                border.width: 1

                ListView {
                    id: suggestionsList
                    anchors.fill: parent
                    anchors.margins: Tokens.padding.extraSmall
                    clip: true
                    model: root.pathSuggestions
                    spacing: 2

                    ScrollBar.vertical: StyledScrollBar {
                        flickable: suggestionsList
                    }

                    delegate: StyledRect {
                        id: suggItem
                        required property int index
                        required property var modelData

                        width: suggestionsList.width
                        implicitHeight: 32
                        radius: Tokens.rounding.small
                        color: (root.selectedSuggestionIndex === index || suggHover.containsMouse) ? Colours.tPalette.m3surfaceContainerHigh : Qt.alpha(Colours.tPalette.m3surfaceContainerHigh, 0)

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Tokens.padding.small
                            anchors.rightMargin: Tokens.padding.small
                            spacing: Tokens.spacing.small

                            MaterialIcon {
                                text: suggItem.modelData.icon || (suggItem.modelData.isDir ? "folder" : "description")
                                color: suggItem.modelData.isDir ? Colours.palette.m3primary : Colours.palette.m3onSurfaceVariant
                                fontStyle: Tokens.font.icon.small
                            }

                            StyledText {
                                Layout.fillWidth: true
                                text: suggItem.modelData.displayPath
                                font: Tokens.font.body.small
                                color: Colours.palette.m3onSurface
                                elide: Text.ElideMiddle
                            }

                            StyledText {
                                visible: suggItem.modelData.isDir
                                text: qsTr("Folder")
                                font: Tokens.font.label.small
                                color: Colours.palette.m3outline
                            }
                        }

                        MouseArea {
                            id: suggHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.applySuggestion(suggItem.index);
                                pathInput.forceActiveFocus();
                            }
                            onDoubleClicked: root.commitPath(suggItem.modelData.displayPath)
                        }
                    }
                }
            }

            // Search bar
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Tokens.padding.small
                anchors.rightMargin: Tokens.padding.small
                visible: root.dialog.isSearching
                spacing: Tokens.spacing.small

                MaterialIcon {
                    text: "search"
                    color: Colours.palette.m3primary
                    fontStyle: Tokens.font.icon.small
                }

                TextInput {
                    id: searchInput
                    Layout.fillWidth: true
                    color: Colours.palette.m3onSurface
                    selectionColor: Colours.palette.m3primaryContainer
                    selectedTextColor: Colours.palette.m3onPrimaryContainer
                    font: Tokens.font.body.small
                    selectByMouse: true
                    cursorVisible: focus
                    verticalAlignment: TextInput.AlignVCenter

                    onTextChanged: {
                        root.dialog.searchText = text;
                    }

                    // "/" is the search hotkey, so never let it leak into the query.
                    Keys.onPressed: event => {
                        if (event.key === Qt.Key_Slash && event.modifiers === Qt.NoModifier) {
                            event.accepted = true;
                        }
                    }

                    // Enter/arrows hand control back to the results view so the
                    // user can pick the first match and navigate the list.
                    Keys.onReturnPressed: event => {
                        root.dialog.focusSearchResults();
                        event.accepted = true;
                    }
                    Keys.onEnterPressed: event => {
                        root.dialog.focusSearchResults();
                        event.accepted = true;
                    }
                    Keys.onDownPressed: event => {
                        root.dialog.searchNavigateDown();
                        event.accepted = true;
                    }
                    Keys.onUpPressed: event => {
                        root.dialog.searchNavigateUp();
                        event.accepted = true;
                    }
                    Keys.onEscapePressed: event => {
                        root.closeSearch();
                        event.accepted = true;
                    }
                }

                Item {
                    implicitWidth: 24
                    implicitHeight: 24

                    MaterialIcon {
                        anchors.centerIn: parent
                        text: "close"
                        color: Colours.palette.m3onSurfaceVariant
                        fontStyle: Tokens.font.icon.small
                    }

                    MouseArea {
                        id: closeSearchHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.closeSearch()
                    }

                    StyledToolTip {
                        text: qsTr("Close search (Esc)")
                        visible: closeSearchHover.containsMouse
                    }
                }
            }
        }

        // Search toggle button
        Item {
            implicitWidth: implicitHeight
            implicitHeight: searchIcon.implicitHeight + Tokens.padding.small

            StateLayer {
                id: searchHover
                radius: Tokens.rounding.medium
                onClicked: {
                    if (root.dialog.isSearching) {
                        root.closeSearch();
                    } else {
                        root.openSearch();
                    }
                }
            }

            MaterialIcon {
                id: searchIcon

                anchors.centerIn: parent
                text: "search"
                color: root.dialog.isSearching ? Colours.palette.m3primary : Colours.palette.m3onSurface
                fill: root.dialog.isSearching ? 1 : 0
            }

            StyledToolTip {
                text: root.dialog.isSearching ? qsTr("Close search") : qsTr("Search (Ctrl+F)")
                visible: searchHover.containsMouse
            }
        }

        // View mode toggle
        Item {
            implicitWidth: implicitHeight
            implicitHeight: viewModeIcon.implicitHeight + Tokens.padding.small

            StateLayer {
                id: viewModeHover
                radius: Tokens.rounding.medium
                onClicked: {
                    AppController.viewMode = AppController.viewMode === 0 ? 1 : 0;
                }
            }

            MaterialIcon {
                id: viewModeIcon

                anchors.centerIn: parent
                text: root.dialog.viewMode === 0 ? "view_list" : "grid_view"
                color: Colours.palette.m3onSurface
            }

            StyledToolTip {
                text: root.dialog.viewMode === 0 ? qsTr("List view (Ctrl+2)") : qsTr("Grid view (Ctrl+1)")
                visible: viewModeHover.containsMouse
            }
        }

        // Hidden files toggle
        Item {
            implicitWidth: implicitHeight
            implicitHeight: hiddenIcon.implicitHeight + Tokens.padding.small

            StateLayer {
                id: hiddenHover
                radius: Tokens.rounding.medium
                onClicked: {
                    root.dialog.showHidden = !root.dialog.showHidden;
                }
            }

            MaterialIcon {
                id: hiddenIcon

                anchors.centerIn: parent
                text: root.dialog.showHidden ? "visibility" : "visibility_off"
                color: root.dialog.showHidden ? Colours.palette.m3primary : Colours.palette.m3outline
                fill: root.dialog.showHidden ? 1 : 0
            }

            StyledToolTip {
                text: root.dialog.showHidden ? qsTr("Hide hidden files (Ctrl+H)") : qsTr("Show hidden files (Ctrl+H)")
                visible: hiddenHover.containsMouse
            }
        }
    }
}
