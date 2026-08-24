cmake_minimum_required(VERSION 3.19)
project(wormhole VERSION 1.0.0 LANGUAGES CXX)

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
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PipeWire IMPORTED_TARGET libpipewire-0.3 REQUIRED)
pkg_check_modules(WaylandClient IMPORTED_TARGET wayland-client REQUIRED)

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
    src/screencast/hyprlandipc.hpp
    src/screencast/hyprlandipc.cpp
    src/screencast/sourcesmodel.hpp
    src/screencast/sourcesmodel.cpp
    src/screencast/pipewirestream.hpp
    src/screencast/pipewirestream.cpp
    src/screencast/screencopy.hpp
    src/screencast/screencopy.cpp
    src/screencast/previewprovider.hpp
    src/screencast/livescreenitem.hpp
    src/screencast/livescreenitem.cpp
    src/appchooser/appinfo.hpp
    src/appchooser/appinfo.cpp
    src/portal/request.hpp
    src/portal/request.cpp
    src/portal/session.hpp
    src/portal/session.cpp
    src/portal/filechooserportal.hpp
    src/portal/filechooserportal.cpp
    src/portal/screencastportal.hpp
    src/portal/screencastportal.cpp
    src/portal/screenshotportal.hpp
    src/portal/screenshotportal.cpp
    src/portal/appchooserportal.hpp
    src/portal/appchooserportal.cpp
    src/portal/accessportal.hpp
    src/portal/accessportal.cpp
    src/portal/accountportal.hpp
    src/portal/accountportal.cpp
    src/portal/dynamiclauncherportal.hpp
    src/portal/dynamiclauncherportal.cpp
    src/portal/wallpaperportal.hpp
    src/portal/wallpaperportal.cpp
    src/portal/inhibitportal.hpp
    src/portal/inhibitportal.cpp
    src/portal/settingsportal.hpp
    src/portal/settingsportal.cpp
    src/portal/portaldaemon.hpp
    src/portal/portaldaemon.cpp
)

set_target_properties(wormhole_app PROPERTIES
    OUTPUT_NAME "wormhole"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(wormhole_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/screencast
    ${CMAKE_CURRENT_SOURCE_DIR}/src/appchooser
    ${CMAKE_CURRENT_SOURCE_DIR}/src/portal
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
    assets/portal/org.freedesktop.impl.portal.desktop.wormhole.service.in
    "${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.impl.portal.desktop.wormhole.service"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.impl.portal.desktop.wormhole.service"
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
