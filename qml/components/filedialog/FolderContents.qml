import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wormhole
import "../"
import "../containers"

Item {
    id: root

    required property var dialog
    readonly property var currentItem: viewLoader.item ? viewLoader.item.currentItem : null
    readonly property real zoomSize: root.dialog.zoomLevel

    FileSystemModel {
        id: fsModel
        path: root.dialog.currentPath
        searchQuery: root.dialog.activeSearchQuery
        showHidden: root.dialog.showHidden
        caseSensitiveSort: AppController.caseSensitiveSort
        showDirsFirst: AppController.showDirsFirst
        onPathChanged: {
            if (viewLoader.item)
                viewLoader.item.currentIndex = -1;
        }
    }

    StyledRect {
        anchors.fill: parent
        color: Colours.tPalette.m3surfaceContainer

        layer.enabled: true
        layer.effect: Mask {
            maskSource: mask
            maskInverted: true
        }
    }

    Item {
        id: mask

        anchors.fill: parent
        layer.enabled: true
        visible: false

        Rectangle {
            anchors.fill: parent
            anchors.margins: Tokens.padding.extraSmall
            radius: Tokens.rounding.medium
        }
    }

    Loader {
        anchors.centerIn: parent

        opacity: viewLoader.item && viewLoader.item.count === 0 ? 1 : 0
        active: opacity > 0

        sourceComponent: ColumnLayout {
            MaterialIcon {
                Layout.alignment: Qt.AlignHCenter
                text: fsModel.isSearching ? "search_off" : "scan_delete"
                color: Colours.palette.m3outline
                fontStyle: Tokens.font.icon.builders.extraLarge.scale(2).weight(Font.Medium).build()
            }

            StyledText {
                text: fsModel.isSearching ? qsTr("No matching files found") : qsTr("This folder is empty")
                color: Colours.palette.m3outline
                font: Tokens.font.body.builders.large.weight(Font.Medium).build()
            }
        }

        Behavior on opacity {
            Anim {
                type: Anim.DefaultEffects
            }
        }
    }

    Loader {
        id: viewLoader

        anchors.fill: parent
        anchors.margins: Tokens.padding.extraSmall + Tokens.padding.medium

        sourceComponent: root.dialog.viewMode === 0 ? gridComp : listComp
    }

    Component {
        id: gridComp

        VerticalFadeGridView {
            id: gridView

            property alias currentItem: gridView.currentItem

            cellWidth: Math.max(144, root.zoomSize + 48)
            cellHeight: root.zoomSize + 84
            Component.onCompleted: forceActiveFocus()

            clip: true
            focus: true
            currentIndex: -1
            interactive: true
            boundsBehavior: Flickable.DragAndOvershootBounds
            maximumFlickVelocity: 5000
            flickDeceleration: 5000

            WheelHandler {
                target: null
                acceptedModifiers: Qt.ControlModifier
                onWheel: event => {
                    if (event.angleDelta.y > 0) {
                        root.dialog.zoomLevel = Math.min(180, root.dialog.zoomLevel + 16);
                    } else if (event.angleDelta.y < 0) {
                        root.dialog.zoomLevel = Math.max(48, root.dialog.zoomLevel - 16);
                    }
                    event.accepted = true;
                }
            }

            Keys.onEscapePressed: currentIndex = -1

            Keys.onReturnPressed: root.triggerAcceptOrOpen()
            Keys.onEnterPressed: root.triggerAcceptOrOpen()

            Keys.onPressed: event => {
                if (event.matches(StandardKey.ZoomIn) || (event.modifiers === Qt.ControlModifier && (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal))) {
                    root.dialog.zoomLevel = Math.min(180, root.dialog.zoomLevel + 16);
                    event.accepted = true;
                    return;
                } else if (event.matches(StandardKey.ZoomOut) || (event.modifiers === Qt.ControlModifier && event.key === Qt.Key_Minus)) {
                    root.dialog.zoomLevel = Math.max(48, root.dialog.zoomLevel - 16);
                    event.accepted = true;
                    return;
                } else if (event.modifiers === Qt.ControlModifier && event.key === Qt.Key_0) {
                    root.dialog.zoomLevel = 80;
                    event.accepted = true;
                    return;
                }

                if (event.key === Qt.Key_Left || event.key === Qt.Key_H) {
                    if (gridView.currentIndex <= 0 && fsModel.count > 0) {
                        gridView.currentIndex = 0;
                    } else {
                        gridView.moveCurrentIndexLeft();
                    }
                    event.accepted = true;
                } else if (event.key === Qt.Key_Right || event.key === Qt.Key_L) {
                    if (gridView.currentIndex === -1 && fsModel.count > 0) {
                        gridView.currentIndex = 0;
                    } else {
                        gridView.moveCurrentIndexRight();
                    }
                    event.accepted = true;
                } else if (event.key === Qt.Key_Up || event.key === Qt.Key_K) {
                    if (gridView.currentIndex === -1 && fsModel.count > 0) {
                        gridView.currentIndex = 0;
                    } else {
                        gridView.moveCurrentIndexUp();
                    }
                    event.accepted = true;
                } else if (event.key === Qt.Key_Down || event.key === Qt.Key_J) {
                    if (gridView.currentIndex === -1 && fsModel.count > 0) {
                        gridView.currentIndex = 0;
                    } else {
                        gridView.moveCurrentIndexDown();
                    }
                    event.accepted = true;
                } else if (event.key === Qt.Key_Backspace) {
                    if (root.dialog.cwd.length > 1) {
                        root.dialog.cwd = root.dialog.cwd.slice(0, root.dialog.cwd.length - 1);
                        event.accepted = true;
                    }
                }
            }

            ScrollBar.vertical: StyledScrollBar {
                flickable: gridView
            }

            model: fsModel

            delegate: Item {
                id: delegateContainer

                required property int index
                required property var modelData

                width: gridView.cellWidth
                height: gridView.cellHeight

                readonly property bool isSelected: gridView.currentIndex === index
                readonly property bool isHidden: delegateContainer.modelData ? (delegateContainer.modelData.isHidden || delegateContainer.modelData.name.startsWith('.')) : false

                StyledRect {
                    id: itemCard
                    anchors.centerIn: parent
                    width: parent.width - 8
                    height: parent.height - 8

                    radius: Tokens.rounding.large
                    color: delegateContainer.isSelected
                        ? Colours.palette.m3secondaryContainer
                        : (itemHover.containsMouse ? Colours.tPalette.m3surfaceContainerHigh : Qt.alpha(Colours.tPalette.m3surfaceContainerHigh, 0))

                    clip: true
                    opacity: delegateContainer.isHidden ? 0.58 : 1.0

                    Behavior on opacity {
                        Anim { type: Anim.FastEffects }
                    }

                    Component.onCompleted: popInAnim.start()

                    Behavior on scale {
                        enabled: !popInAnim.running && !modifiedBounceAnim.running
                        Anim { type: Anim.FastEffects }
                    }

                    ParallelAnimation {
                        id: popInAnim
                        NumberAnimation {
                            target: itemCard
                            property: "scale"
                            from: 0.6
                            to: 1.0
                            duration: 250
                            easing.type: Easing.OutBack
                            easing.overshoot: 1.3
                        }
                    }

                    Connections {
                        target: fsModel
                        function onFileModified(modifiedPath) {
                            if (delegateContainer.modelData && delegateContainer.modelData.path === modifiedPath) {
                                modifiedBounceAnim.restart();
                            }
                        }
                    }

                    SequentialAnimation {
                        id: modifiedBounceAnim
                        NumberAnimation {
                            target: itemCard
                            property: "scale"
                            to: 1.14
                            duration: 130
                            easing.type: Easing.OutQuad
                        }
                        NumberAnimation {
                            target: itemCard
                            property: "scale"
                            to: 1.0
                            duration: 220
                            easing.type: Easing.OutBack
                            easing.overshoot: 1.4
                        }
                    }

                    MouseArea {
                        id: itemHover
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton

                        onClicked: mouse => {
                            gridView.forceActiveFocus();
                            gridView.currentIndex = delegateContainer.index;
                            if (AppController.singleClick && mouse.button === Qt.LeftButton) {
                                root.handleItemClick(delegateContainer.modelData);
                            }
                        }

                        onDoubleClicked: mouse => {
                            if (mouse.button === Qt.LeftButton && !AppController.singleClick) {
                                root.handleItemClick(delegateContainer.modelData);
                            }
                        }
                    }

                    Item {
                        id: iconContainer
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 8
                        width: root.zoomSize
                        height: root.zoomSize

                        CachingIconImage {
                            id: icon
                            anchors.fill: parent
                            implicitSize: root.zoomSize

                            source: {
                                const file = delegateContainer.modelData;
                                if (!file) return "";
                                if (file.hasThumbnail) {
                                    let t = file.lastModified ? file.lastModified.getTime() : file.size;
                                    return "image://thumb/" + file.path + "?t=" + t;
                                } else {
                                    return FileUtils.iconForFile(file.name, file.isDir, file.mimeType);
                                }
                            }
                        }

                        StyledRect {
                            anchors.top: parent.top
                            anchors.left: parent.left
                            implicitWidth: 20
                            implicitHeight: 20
                            radius: Tokens.rounding.full
                            color: Qt.alpha(Colours.palette.m3surface, 0.9)
                            visible: delegateContainer.modelData ? delegateContainer.modelData.isReadOnly : false
                            z: 5

                            MaterialIcon {
                                anchors.centerIn: parent
                                text: "lock"
                                fontStyle: Tokens.font.icon.small
                                color: Colours.palette.m3error
                            }
                        }

                        StyledRect {
                            anchors.bottom: parent.bottom
                            anchors.right: parent.right
                            implicitWidth: 20
                            implicitHeight: 20
                            radius: Tokens.rounding.full
                            color: Qt.alpha(Colours.palette.m3surface, 0.9)
                            visible: delegateContainer.modelData ? delegateContainer.modelData.isSymLink : false
                            z: 5

                            MaterialIcon {
                                anchors.centerIn: parent
                                text: "link"
                                fontStyle: Tokens.font.icon.small
                                color: Colours.palette.m3primary
                            }
                        }
                    }

                    StyledText {
                        id: name

                        anchors.top: iconContainer.bottom
                        anchors.topMargin: 4
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 4

                        text: delegateContainer.modelData ? delegateContainer.modelData.name : ""
                        color: delegateContainer.isSelected ? Colours.palette.m3onSecondaryContainer : Colours.palette.m3onSurface
                        font: Tokens.font.body.small
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignTop
                        elide: Text.ElideMiddle
                        maximumLineCount: 4
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }

    Component {
        id: listComp

        ColumnLayout {
            id: listRoot

            property alias currentItem: listView.currentItem

            // Session column widths for the list view. The picker is transient, so
            // these are intentionally not persisted.
            property var columnWidths: ({ "size": 90, "type": 160, "date": 140 })

            readonly property int count: fsModel.count

            readonly property var columnSpecs: [
                {
                    key: "size",
                    label: qsTr("Size"),
                    width: 90,
                    sort: FileSystemModel.SortBySize,
                    descendingFirst: false,
                    alignRight: true
                },
                {
                    key: "type",
                    label: qsTr("Type"),
                    width: 160,
                    sort: FileSystemModel.SortByType,
                    descendingFirst: false,
                    alignRight: false
                },
                {
                    key: "date",
                    label: qsTr("Date Modified"),
                    width: 140,
                    sort: FileSystemModel.SortByDate,
                    descendingFirst: true,
                    alignRight: true
                }
            ]

            function colWidth(key) {
                return columnWidths[key] !== undefined ? columnWidths[key] : 100;
            }

            function setColWidth(key, value) {
                const copy = Object.assign({}, columnWidths);
                copy[key] = Math.max(60, Math.round(value));
                columnWidths = copy;
            }

            function cellText(file, key) {
                if (!file)
                    return "";
                switch (key) {
                case "size":
                    return file.isDir ? "" : (file.formattedSize || "");
                case "type":
                    return file.mimeDescription || "";
                case "date":
                    return file.formattedDate || "";
                }
                return "";
            }

            function toggleSort(field, descendingFirst) {
                if (fsModel.sortField === field) {
                    fsModel.sortOrder = fsModel.sortOrder === Qt.AscendingOrder ? Qt.DescendingOrder : Qt.AscendingOrder;
                } else {
                    fsModel.sortField = field;
                    fsModel.sortOrder = descendingFirst ? Qt.DescendingOrder : Qt.AscendingOrder;
                }
            }

            spacing: Tokens.spacing.extraSmall

            // Column header row
            StyledRect {
                Layout.fillWidth: true
                implicitHeight: 36
                radius: Tokens.rounding.small
                color: Colours.tPalette.m3surfaceContainerHigh

                RowLayout {
                    id: headerRow

                    anchors.fill: parent
                    anchors.leftMargin: Tokens.padding.medium
                    anchors.rightMargin: Tokens.padding.medium
                    spacing: Tokens.spacing.small

                    // Name column
                    Item {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 120
                        implicitHeight: parent.height
                        clip: true

                        RowLayout {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.right: parent.right
                            spacing: Tokens.spacing.extraSmall

                            StyledText {
                                Layout.fillWidth: true
                                text: qsTr("Name")
                                font: Tokens.font.label.medium
                                color: Colours.palette.m3onSurface
                                elide: Text.ElideRight
                            }

                            MaterialIcon {
                                visible: fsModel.sortField === FileSystemModel.SortByName
                                text: fsModel.sortOrder === Qt.AscendingOrder ? "arrow_upward" : "arrow_downward"
                                fontStyle: Tokens.font.icon.small
                                color: Colours.palette.m3primary
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: listRoot.toggleSort(FileSystemModel.SortByName, false)
                        }
                    }

                    Repeater {
                        id: headerRepeater

                        model: listRoot.columnSpecs

                        delegate: Item {
                            id: headerCell

                            required property var modelData

                            Layout.preferredWidth: listRoot.colWidth(modelData.key)
                            implicitHeight: parent.height

                            ColumnResizeHandle {
                                columnKey: headerCell.modelData.key
                                defaultWidth: headerCell.modelData.width
                                columnWidth: listRoot.colWidth(headerCell.modelData.key)
                                onResized: width => listRoot.setColWidth(headerCell.modelData.key, width)
                            }

                            RowLayout {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.right: parent.right
                                spacing: Tokens.spacing.extraSmall

                                StyledText {
                                    Layout.fillWidth: true
                                    text: headerCell.modelData.label
                                    font: Tokens.font.label.medium
                                    color: Colours.palette.m3onSurface
                                    elide: Text.ElideRight
                                }

                                MaterialIcon {
                                    visible: fsModel.sortField === headerCell.modelData.sort
                                    text: fsModel.sortOrder === Qt.AscendingOrder ? "arrow_upward" : "arrow_downward"
                                    fontStyle: Tokens.font.icon.small
                                    color: Colours.palette.m3primary
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: listRoot.toggleSort(headerCell.modelData.sort, headerCell.modelData.descendingFirst)
                            }
                        }
                    }
                }
            }

            // Details list view
            VerticalFadeListView {
                id: listView

                Layout.fillWidth: true
                Layout.fillHeight: true

                clip: true
                focus: true
                currentIndex: -1
                interactive: true
                boundsBehavior: Flickable.DragAndOvershootBounds
                maximumFlickVelocity: 5000
                flickDeceleration: 5000

                Component.onCompleted: forceActiveFocus()

                Keys.onEscapePressed: currentIndex = -1

                Keys.onReturnPressed: root.triggerAcceptOrOpen()
                Keys.onEnterPressed: root.triggerAcceptOrOpen()

                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Up || event.key === Qt.Key_K) {
                        if (listView.currentIndex <= 0 && fsModel.count > 0) {
                            listView.currentIndex = 0;
                        } else {
                            listView.decrementCurrentIndex();
                        }
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Down || event.key === Qt.Key_J) {
                        if (listView.currentIndex === -1 && fsModel.count > 0) {
                            listView.currentIndex = 0;
                        } else {
                            listView.incrementCurrentIndex();
                        }
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Backspace) {
                        if (root.dialog.cwd.length > 1) {
                            root.dialog.cwd = root.dialog.cwd.slice(0, root.dialog.cwd.length - 1);
                            event.accepted = true;
                        }
                    }
                }

                ScrollBar.vertical: StyledScrollBar {
                    flickable: listView
                }

                model: fsModel

                delegate: Item {
                    id: listDelegate

                    required property int index
                    required property var modelData

                    width: listView.width
                    height: 36

                    readonly property bool isSelected: listView.currentIndex === index
                    readonly property bool isHidden: listDelegate.modelData ? (listDelegate.modelData.isHidden || listDelegate.modelData.name.startsWith('.')) : false

                    StyledRect {
                        anchors.fill: parent
                        anchors.margins: 2
                        radius: Tokens.rounding.small

                        color: listDelegate.isSelected
                            ? Colours.palette.m3secondaryContainer
                            : (listHover.containsMouse ? Colours.tPalette.m3surfaceContainerHigh : Qt.alpha(Colours.tPalette.m3surfaceContainerHigh, 0))

                        opacity: listDelegate.isHidden ? 0.58 : 1.0

                        Behavior on opacity {
                            Anim { type: Anim.FastEffects }
                        }

                        MouseArea {
                            id: listHover
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.LeftButton | Qt.RightButton

                            onClicked: mouse => {
                                listView.forceActiveFocus();
                                listView.currentIndex = listDelegate.index;
                                if (AppController.singleClick && mouse.button === Qt.LeftButton) {
                                    root.handleItemClick(listDelegate.modelData);
                                }
                            }

                            onDoubleClicked: mouse => {
                                if (mouse.button === Qt.LeftButton && !AppController.singleClick) {
                                    root.handleItemClick(listDelegate.modelData);
                                }
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Tokens.padding.medium
                            anchors.rightMargin: Tokens.padding.medium
                            spacing: Tokens.spacing.small

                            // Name + icon + lock/symlink indicators
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 120
                                spacing: Tokens.spacing.small

                                Item {
                                    implicitWidth: 20
                                    implicitHeight: 20

                                    CachingIconImage {
                                        anchors.fill: parent
                                        implicitSize: 20

                                        source: {
                                            const file = listDelegate.modelData;
                                            if (!file) return "";
                                            return FileUtils.iconForFile(file.name, file.isDir, file.mimeType);
                                        }
                                    }

                                    MaterialIcon {
                                        anchors.top: parent.top
                                        anchors.left: parent.left
                                        anchors.margins: -4
                                        visible: listDelegate.modelData ? listDelegate.modelData.isReadOnly : false
                                        text: "lock"
                                        fontStyle: Tokens.font.icon.small
                                        color: Colours.palette.m3error
                                        z: 2
                                    }

                                    MaterialIcon {
                                        anchors.bottom: parent.bottom
                                        anchors.right: parent.right
                                        anchors.margins: -3
                                        visible: listDelegate.modelData ? listDelegate.modelData.isSymLink : false
                                        text: "link"
                                        fontStyle: Tokens.font.icon.small
                                        color: Colours.palette.m3primary
                                        z: 2
                                    }
                                }

                                StyledText {
                                    Layout.fillWidth: true
                                    text: listDelegate.modelData ? listDelegate.modelData.name : ""
                                    color: listDelegate.isSelected ? Colours.palette.m3onSecondaryContainer : Colours.palette.m3onSurface
                                    font: Tokens.font.body.small
                                    elide: Text.ElideMiddle
                                }
                            }

                            Repeater {
                                model: listRoot.columnSpecs

                                delegate: StyledText {
                                    required property var modelData

                                    Layout.preferredWidth: listRoot.colWidth(modelData.key)
                                    text: listRoot.cellText(listDelegate.modelData, modelData.key)
                                    color: Colours.palette.m3onSurfaceVariant
                                    font: Tokens.font.body.small
                                    horizontalAlignment: modelData.alignRight ? Text.AlignRight : Text.AlignLeft
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    function handleItemClick(file) {
        if (!file) return;
        if (file.isDir) {
            root.navigateInto(file);
        } else if (root.dialog.saveMode) {
            root.dialog.saveName = file.name;
        } else if (root.dialog.selectionValid && !root.dialog.directoryOnly) {
            root.dialog.accepted(file.path);
        }
    }

    function navigateInto(file) {
        if (!file || !file.isDir) return;
        if (fsModel.isSearching) {
            // Search results can live anywhere; jump straight to the real path.
            root.dialog.clearSearch();
            root.dialog.cwd = root.dialog.pathToCwd(file.path);
        } else {
            let newCwd = root.dialog.cwd.slice();
            newCwd.push(file.name);
            root.dialog.cwd = newCwd;
        }
    }

    function triggerAcceptOrOpen() {
        if (currentItem && currentItem.modelData) {
            if (currentItem.modelData.isDir) {
                if (root.dialog.directoryOnly) {
                    root.dialog.accepted(currentItem.modelData.path);
                } else {
                    root.navigateInto(currentItem.modelData);
                }
                return;
            }
        }

        if (root.dialog.selectionValid) {
            if (root.dialog.directoryOnly) {
                if (currentItem && currentItem.modelData && currentItem.modelData.isDir) {
                    root.dialog.accepted(currentItem.modelData.path);
                } else {
                    root.dialog.accepted(root.dialog.currentPath);
                }
            } else if (currentItem && currentItem.modelData) {
                root.dialog.accepted(currentItem.modelData.path);
            }
        }
    }

    CurrentItem {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Tokens.padding.extraSmall

        currentItem: root.currentItem
    }
}
