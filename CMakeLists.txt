cmake_minimum_required(VERSION 3.19)

# Version selection: pass -DWORMHOLE_VERSION=X.Y.Z to pin the version manually,
# otherwise derive it from the most recent git tag (e.g. v1.2.3).
set(WORMHOLE_VERSION ""
    CACHE STRING "Version of wormhole (default: latest git tag)")

if(NOT WORMHOLE_VERSION)
    find_package(Git QUIET)
    if(GIT_EXECUTABLE)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" describe --tags --match "v[0-9]*" --abbrev=0 HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE git_describe_result
            OUTPUT_VARIABLE git_describe_version
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(git_describe_result EQUAL 0 AND git_describe_version)
            string(REGEX REPLACE "^v" "" WORMHOLE_VERSION "${git_describe_version}")
        endif()
    endif()
endif()

if(NOT WORMHOLE_VERSION)
    # Dev version: 0.YYYYMMDD (CMake requires numeric-only version components)
    find_package(Git REQUIRED)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" log -1 --format=%cd --date=format:%Y%m%d
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE git_date
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(WORMHOLE_VERSION "0.${git_date}")
endif()

project(wormhole VERSION ${WORMHOLE_VERSION} LANGUAGES C CXX)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Default install prefix" FORCE)
endif()

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    QuickEffects
    Concurrent
    Svg
    DBus
    Network
    Multimedia
    Test
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Gio IMPORTED_TARGET gio-2.0 REQUIRED)
pkg_check_modules(Glib IMPORTED_TARGET glib-2.0 REQUIRED)
pkg_check_modules(PipeWire IMPORTED_TARGET libpipewire-0.3 REQUIRED)
pkg_check_modules(WaylandClient IMPORTED_TARGET wayland-client REQUIRED)
pkg_check_modules(WaylandProtocols REQUIRED wayland-protocols>=1.38)
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)

set(WAYLAND_PROTOCOL_OUT "${CMAKE_CURRENT_BINARY_DIR}/wayland-protocols")
file(MAKE_DIRECTORY "${WAYLAND_PROTOCOL_OUT}")

set(WAYLAND_PROTOCOL_XML
    "${WAYLAND_PROTOCOLS_DIR}/staging/ext-image-capture-source/ext-image-capture-source-v1.xml"
    "${WAYLAND_PROTOCOLS_DIR}/staging/ext-image-copy-capture/ext-image-copy-capture-v1.xml"
    "${WAYLAND_PROTOCOLS_DIR}/staging/ext-foreign-toplevel-list/ext-foreign-toplevel-list-v1.xml"
)

set(WAYLAND_PROTOCOL_SOURCES "")
foreach(protocol IN LISTS WAYLAND_PROTOCOL_XML)
    get_filename_component(protocolName "${protocol}" NAME_WE)
    set(protocolHeader "${WAYLAND_PROTOCOL_OUT}/${protocolName}-client-protocol.h")
    set(protocolCode "${WAYLAND_PROTOCOL_OUT}/${protocolName}-protocol.c")

    add_custom_command(
        OUTPUT "${protocolHeader}"
        COMMAND "${WAYLAND_SCANNER}" client-header "${protocol}" "${protocolHeader}"
        DEPENDS "${protocol}"
        VERBATIM
    )
    add_custom_command(
        OUTPUT "${protocolCode}"
        COMMAND "${WAYLAND_SCANNER}" private-code "${protocol}" "${protocolCode}"
        DEPENDS "${protocol}"
        VERBATIM
    )

    list(APPEND WAYLAND_PROTOCOL_SOURCES "${protocolHeader}" "${protocolCode}")
endforeach()

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(wormhole_app
    src/main.cpp
    src/config/fontbuilder.hpp
    src/config/fontbuilder.cpp
    src/config/font.hpp
    src/config/font.cpp
    src/config/tokens.hpp
    src/config/tokens.cpp
    src/config/colours.hpp
    src/config/colours.cpp
    src/core/appcontroller.hpp
    src/core/appcontroller.cpp
    src/core/iconprovider.hpp
    src/core/fileutils.hpp
    src/core/fileutils.cpp
    src/core/trashlocations.hpp
    src/core/trashlocations.cpp
    src/core/recentfiles.hpp
    src/core/recentfiles.cpp
    src/core/placesmodel.hpp
    src/core/placesmodel.cpp
    src/core/drivemanager.hpp
    src/core/drivemanager.cpp

    src/core/thumbnailprovider.hpp
    src/core/thumbnailprovider.cpp
    src/models/filesystemmodel.hpp
    src/models/filesystemmodel.cpp
    src/screencast/hyprlandipc.hpp
    src/screencast/hyprlandipc.cpp
    src/screencast/sourcesmodel.hpp
    src/screencast/sourcesmodel.cpp
    src/screencast/pipewirestream.hpp
    src/screencast/pipewirestream.cpp
    src/screencast/waylandcapture.hpp
    src/screencast/waylandcapture.cpp
    src/screencast/previewprovider.hpp
    src/screencast/livescreenitem.hpp
    src/screencast/livescreenitem.cpp
    src/appchooser/appinfo.hpp
    src/appchooser/appinfo.cpp
    src/portal/request.hpp
    src/portal/request.cpp
    src/portal/session.hpp
    src/portal/session.cpp
    src/portal/restorestore.hpp
    src/portal/restorestore.cpp
    src/portal/accessportal.hpp
    src/portal/accessportal.cpp
    src/portal/accountportal.hpp
    src/portal/accountportal.cpp
    src/portal/appchooserportal.hpp
    src/portal/appchooserportal.cpp
    src/portal/backgroundportal.hpp
    src/portal/backgroundportal.cpp
    src/portal/cameraportal.hpp
    src/portal/cameraportal.cpp
    src/portal/clipboardportal.hpp
    src/portal/clipboardportal.cpp
    src/portal/dynamiclauncherportal.hpp
    src/portal/dynamiclauncherportal.cpp
    src/portal/emailportal.hpp
    src/portal/emailportal.cpp
    src/portal/filechooserportal.hpp
    src/portal/filechooserportal.cpp
    src/portal/globalshortcutsportal.hpp
    src/portal/globalshortcutsportal.cpp
    src/portal/inhibitportal.hpp
    src/portal/inhibitportal.cpp
    src/portal/inputcaptureportal.hpp
    src/portal/inputcaptureportal.cpp
    src/portal/locationportal.hpp
    src/portal/locationportal.cpp
    src/portal/lockdownportal.hpp
    src/portal/lockdownportal.cpp
    src/portal/notificationportal.hpp
    src/portal/notificationportal.cpp
    src/portal/printportal.hpp
    src/portal/printportal.cpp
    src/portal/realtimeportal.hpp
    src/portal/realtimeportal.cpp
    src/portal/remotedesktopportal.hpp
    src/portal/remotedesktopportal.cpp
    src/portal/screencastportal.hpp
    src/portal/screencastportal.cpp
    src/portal/screenshotportal.hpp
    src/portal/screenshotportal.cpp
    src/portal/secretportal.hpp
    src/portal/secretportal.cpp
    src/portal/settingsportal.hpp
    src/portal/settingsportal.cpp
    src/portal/usbportal.hpp
    src/portal/usbportal.cpp
    src/portal/wallpaperportal.hpp
    src/portal/wallpaperportal.cpp
    src/portal/portaldaemon.hpp
    src/portal/portaldaemon.cpp
    ${WAYLAND_PROTOCOL_SOURCES}
)

set_target_properties(wormhole_app PROPERTIES
    OUTPUT_NAME "wormhole"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(wormhole_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/screencast
    ${CMAKE_CURRENT_SOURCE_DIR}/src/appchooser
    ${CMAKE_CURRENT_SOURCE_DIR}/src/portal
    ${WAYLAND_PROTOCOL_OUT}
)

qt_add_qml_module(wormhole_app
    URI "wormhole"
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/components/AnchorAnim.qml
        qml/components/Anim.qml
        qml/components/AnimLoader.qml
        qml/components/CAnim.qml
        qml/components/CachingIconImage.qml
        qml/components/Logo.qml
        qml/components/Mask.qml
        qml/components/MaterialIcon.qml
        qml/components/SmartIcon.qml
        qml/components/ScreenState.qml
        qml/components/StateLayer.qml
        qml/components/StyledClippingRect.qml
        qml/components/StyledRect.qml
        qml/components/StyledScrollBar.qml
        qml/components/StyledText.qml
        qml/components/StyledToolTip.qml
        qml/components/containers/ConnectedRect.qml
        qml/components/containers/StyledFlickable.qml
        qml/components/containers/StyledListView.qml
        qml/components/containers/StyledWindow.qml
        qml/components/containers/VerticalFadeFlickable.qml
        qml/components/containers/VerticalFadeGridView.qml
        qml/components/containers/VerticalFadeListView.qml
        qml/components/controls/ButtonBase.qml
        qml/components/controls/ButtonGroup.qml
        qml/components/controls/ButtonGroupItem.qml
        qml/components/controls/ButtonRow.qml
        qml/components/controls/ButtonRowItem.qml
        qml/components/controls/CircularIndicator.qml
        qml/components/controls/CircularProgress.qml
        qml/components/controls/CustomMouseArea.qml
        qml/components/controls/FilledSlider.qml
        qml/components/controls/IconButton.qml
        qml/components/controls/IconTextButton.qml
        qml/components/controls/LoadingIndicator.qml
        qml/components/controls/Menu.qml
        qml/components/controls/MenuItem.qml
        qml/components/controls/RowButton.qml
        qml/components/controls/SearchBar.qml
        qml/components/controls/SlidingSelector.qml
        qml/components/controls/SplitButton.qml
        qml/components/controls/SplitButtonRow.qml
        qml/components/controls/StyledCheckBox.qml
        qml/components/controls/StyledProgressBar.qml
        qml/components/controls/StyledRadioButton.qml
        qml/components/controls/StyledScrollBar.qml
        qml/components/controls/StyledSlider.qml
        qml/components/controls/StyledSpinBox.qml
        qml/components/controls/StyledSwitch.qml
        qml/components/controls/StyledTextField.qml
        qml/components/controls/TextButton.qml
        qml/components/controls/TextFieldBase.qml
        qml/components/controls/ToggleRow.qml
        qml/components/effects/ColouredIcon.qml
        qml/components/effects/Colouriser.qml
        qml/components/effects/Elevation.qml
        qml/components/effects/Mask.qml
        qml/components/filedialog/CurrentItem.qml
        qml/components/filedialog/DialogButtons.qml
        qml/components/filedialog/FileDialog.qml
        qml/components/filedialog/FolderContents.qml
        qml/components/filedialog/HeaderBar.qml
        qml/components/filedialog/Sidebar.qml
        qml/components/filedialog/Sizes.qml
        qml/components/images/CachingIconImage.qml
        qml/components/images/CachingImage.qml
        qml/components/images/FadeImage.qml
        qml/components/misc/CustomShortcut.qml
        qml/components/misc/Ref.qml
        qml/components/widgets/CoverArt.qml
        qml/components/widgets/ExtraIndicator.qml
        qml/components/widgets/WavyTopRect.qml
        qml/dialogs/ScreenChooserDialog.qml
        qml/dialogs/ScreenshotDialog.qml
        qml/dialogs/AppChooserDialog.qml
        qml/dialogs/AccessDialog.qml
        qml/dialogs/AccountDialog.qml
        qml/dialogs/DynamicLauncherDialog.qml
        qml/dialogs/WallpaperDialog.qml
    RESOURCES
        assets/fonts/GoogleSansFlex.ttf
        assets/fonts/MaterialSymbolsRounded.ttf
        assets/material_icons.txt
        assets/wormhole.svg
)

target_link_libraries(wormhole_app PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickEffects
    Qt6::Concurrent
    Qt6::Svg
    Qt6::DBus
    Qt6::Network
    Qt6::Multimedia
    PkgConfig::Gio
    PkgConfig::Glib
    PkgConfig::PipeWire
    PkgConfig::WaylandClient
)

# Install targets and assets
install(TARGETS wormhole_app
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES assets/portal/wormhole.portal
    DESTINATION ${CMAKE_INSTALL_DATADIR}/xdg-desktop-portal/portals
)

set(WORMHOLE_BIN "${CMAKE_INSTALL_FULL_BINDIR}/wormhole")

configure_file(
    assets/portal/org.freedesktop.impl.portal.Desktop.wormhole.service.in
    "${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.impl.portal.Desktop.wormhole.service"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.impl.portal.Desktop.wormhole.service"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services
)

install(FILES assets/portal/portals.conf
    DESTINATION ${CMAKE_INSTALL_DATADIR}/xdg-desktop-portal
    RENAME wormhole-portals.conf
)

# Create symlink xdg-desktop-portal-wormhole -> wormhole
install(CODE "
    execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink
        wormhole
        \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/xdg-desktop-portal-wormhole\"
    )
")

# Detect init system to decide service files to install
set(INIT_SYSTEM "auto" CACHE STRING "Init system to install service files for (auto, openrc, systemd, none)")

if(INIT_SYSTEM STREQUAL "auto")
    if(EXISTS "/sbin/openrc" OR EXISTS "/usr/bin/openrc" OR EXISTS "/run/openrc")
        set(DETECTED_INIT "openrc")
    elseif(EXISTS "/usr/lib/systemd" OR EXISTS "/lib/systemd" OR EXISTS "/etc/systemd")
        set(DETECTED_INIT "systemd")
    else()
        set(DETECTED_INIT "none")
    endif()
else()
    set(DETECTED_INIT "${INIT_SYSTEM}")
endif()

message(STATUS "Configuring service files for init system: ${DETECTED_INIT}")

if(DETECTED_INIT STREQUAL "openrc")
    configure_file(
        assets/init/xdg-desktop-portal-wormhole.initd.in
        "${CMAKE_CURRENT_BINARY_DIR}/xdg-desktop-portal-wormhole.initd"
        @ONLY
    )
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/xdg-desktop-portal-wormhole.initd"
        DESTINATION /etc/init.d
        RENAME xdg-desktop-portal-wormhole
    )
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/xdg-desktop-portal-wormhole.initd"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/wormhole/openrc
        RENAME xdg-desktop-portal-wormhole
    )
elseif(DETECTED_INIT STREQUAL "systemd")
    configure_file(
        assets/init/xdg-desktop-portal-wormhole.service.in
        "${CMAKE_CURRENT_BINARY_DIR}/xdg-desktop-portal-wormhole.service"
        @ONLY
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/xdg-desktop-portal-wormhole.service"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/user
    )
endif()

enable_testing()

add_executable(test_restorestore
    tests/test_restorestore.cpp
    src/portal/restorestore.cpp
    src/portal/restorestore.hpp
)

target_include_directories(test_restorestore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(test_restorestore PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME test_restorestore COMMAND test_restorestore)
