{
  description = "Wormhole - Material 3 Expressive XDG Desktop Portal daemon & dialogs for Wayland";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = {
    self,
    nixpkgs,
    ...
  }: let
    forAllSystems = fn:
      nixpkgs.lib.genAttrs nixpkgs.lib.platforms.linux (
        system: fn nixpkgs.legacyPackages.${system}
      );
  in {
    packages = forAllSystems (pkgs: rec {
      wormhole = pkgs.callPackage ./nix {};
      default = wormhole;
    });

    devShells = forAllSystems (pkgs: {
      default = pkgs.mkShell {
        packages = with pkgs; [
          cmake
          ninja
          pkg-config
          wayland-scanner
          wayland-protocols
          pipewire
          glib
          qt6.qtbase
          qt6.qtdeclarative
          qt6.qtsvg
          qt6.qtmultimedia
          qt6.wrapQtAppsHook
          grim
        ];
      };
    });

    apps = forAllSystems (pkgs: {
      default = {
        type = "app";
        program = "${self.packages.${pkgs.stdenv.hostPlatform.system}.default}/bin/wormhole";
        meta.description = "Launch Wormhole Material 3 XDG desktop portal daemon";
      };
      wormhole = {
        type = "app";
        program = "${self.packages.${pkgs.stdenv.hostPlatform.system}.default}/bin/wormhole";
        meta.description = "Launch Wormhole Material 3 XDG desktop portal daemon";
      };
    });

    homeManagerModules.default = { config, lib, pkgs, ... }:
      with lib;
      let
        pkg = self.packages.${pkgs.stdenv.hostPlatform.system}.default;
        cfg = config.programs.wormhole;
      in {
        options.programs.wormhole = {
          enable = mkEnableOption "Wormhole Material 3 Expressive XDG Desktop Portal daemon";
          package = mkOption {
            type = types.package;
            default = pkg;
            description = "The wormhole package to use.";
          };
        };

        config = mkIf cfg.enable {
          home.packages = [ cfg.package ];
        };
      };
  };
}
