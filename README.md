# Wormhole 🌌

**Wormhole** is a native, modern, Material 3 Expressive implementation of the `xdg-desktop-portal` backend designed for Wayland & Hyprland desktop environments.

It provides a unified D-Bus portal daemon (`xdg-desktop-portal-wormhole`) and Material 3 Expressive UI dialogs for all desktop portal interfaces, integrating with Caelestia shell styling and Atlas file manager.

---

## Features

- **ScreenCast & Screen Sharing (`org.freedesktop.impl.portal.ScreenCast`)**:
  - Material 3 Expressive dialog for sharing displays or individual application windows.
  - Hyprland IPC integration for live monitor and window discovery with application icons and workspace tags.
  - PipeWire stream node generation with hardware-accelerated / Wayland screencopy capture.
  - Persistent restore token support and cursor visibility toggles (embedded, hidden, metadata).
- **Screenshot & Color Picker (`org.freedesktop.impl.portal.Screenshot`)**:
  - Interactive region crop selector with live dimensions and magnifier.
  - Window and fullscreen snapshot modes.
  - Precision pixel color picker tool (`PickColor`).
- **FileChooser Portal (`org.freedesktop.impl.portal.FileChooser`)**:
  - Seamless delegation to `atlas --picker` for Material 3 file and folder selection, saving, and multi-file handling.
- **Application Chooser (`org.freedesktop.impl.portal.AppChooser`)**:
  - Searchable Material 3 app chooser filtering system desktop entries by MIME type or URL protocol scheme.
- **Access & Permissions (`org.freedesktop.impl.portal.Access`)**:
  - Material 3 security permission prompt for camera, microphone, and location.
- **Dynamic Launcher (`org.freedesktop.impl.portal.DynamicLauncher`)**:
  - Web application and desktop shortcut installer.
- **Wallpaper (`org.freedesktop.impl.portal.Wallpaper`)**:
  - Wallpaper preview and target selection (Desktop, Lockscreen, or Both).
- **Settings (`org.freedesktop.impl.portal.Settings`)**:
  - Appearance and theme settings provider (`color-scheme`, `accent-color`, `contrast`).
- **Inhibition (`org.freedesktop.impl.portal.Inhibit`)**:
  - Sleep, idle, and notification inhibition tracking.

---

## Build & Installation

### Requirements
- **C++20** compiler
- **CMake** ≥ 3.19
- **Qt 6.5+** (`Core`, `Gui`, `Qml`, `Quick`, `QuickControls2`, `QuickEffects`, `DBus`, `Concurrent`, `Svg`, `Network`)
- **PipeWire** (`libpipewire-0.3`)
- **Wayland Client** (`wayland-client`)
- **Atlas** (runtime dependency for file choosing)

### Build Commands
```bash
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
sudo cmake --install build
```

---

## Usage

### Run as D-Bus Portal Daemon
```bash
wormhole --daemon
```

### Direct CLI Dialog Testing
```bash
# Screen share chooser
wormhole --screencast --app-id "Discord"

# Interactive screenshot
wormhole --screenshot --interactive

# Color picker
wormhole --pick-color

# Application chooser
wormhole --appchooser --mime "image/png"

# Access permission prompt
wormhole --access --title "Camera Permission" --app-id "org.mozilla.Firefox"

# Dynamic launcher prompt
wormhole --dynamic-launcher --name "GitHub" --url "https://github.com"

# Wallpaper preview
wormhole --wallpaper --url "file:///usr/share/backgrounds/default.png"
```

---

## License
GPL-3.0 License.
