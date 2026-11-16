{
  lib,
  stdenv,
  cmake,
  ninja,
  pkg-config,
  wayland-scanner,
  qt6,
  glib,
  pipewire,
  wayland,
  wayland-protocols,
  grim,
}:
stdenv.mkDerivation (finalAttrs: {
  pname = "wormhole";
  version = "1.0.2";

  src = ./..;

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    wayland-scanner
    qt6.wrapQtAppsHook
  ];

  buildInputs = [
    qt6.qtbase
    qt6.qtdeclarative
    qt6.qtsvg
    qt6.qtmultimedia
    glib
    pipewire
    wayland
    wayland-protocols
  ];

  cmakeFlags = [
    "-DINIT_SYSTEM=systemd"
    "-DWORMHOLE_VERSION=1.0.2"
  ];

  postFixup = ''
    wrapProgram $out/bin/wormhole \
      --prefix PATH : ${lib.makeBinPath [ grim glib pipewire ]}
  '';

  meta = with lib; {
    description = "Material 3 Expressive XDG Desktop Portal daemon & dialogs for Wayland and Hyprland";
    homepage = "https://github.com/AstraSuite/Wormhole";
    license = licenses.gpl3Only;
    platforms = platforms.linux;
    mainProgram = "wormhole";
  };
})
