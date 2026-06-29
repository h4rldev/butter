{
  description = "The dev flake for butter, a vulkan renderer.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    bread.url = "git+https://codeberg.org/h4rl/bread";
    htils.url = "github:h4rldev/htils";
  };

  outputs = {
    self,
    flake-utils,
    nixpkgs,
    bread,
    htils,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs {inherit system;};
      pversion = "0.1.0";
    in {
      packages.butter-wayland-release = pkgs.stdenv.mkDerivation {
        pname = "butter-wayland";
        version = pversion;

        src = ./.;

        nativeBuildInputs = [
          pkgs.just
          pkgs.gcc
          pkgs.vulkan-headers
          pkgs.wayland

          htils.packages.${system}.htils-threadsafe
        ];

        buildPhase = ''
          runHook preBuild

          sed -i 's|#!/usr/bin/env bash|#!${pkgs.bash}/bin/bash|' justfile
          just release wayland

          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall

          mkdir -p $out/lib
          mkdir -p $out/include/butter

          cp lib/libbutter-wayland-release.so $out/lib
          cp -r include/butter/* $out/include/butter

          runHook postInstall
        '';
      };

      packages.butter-wayland-release-static = pkgs.stdenv.mkDerivation {
        pname = "butter-wayland-static";
        version = pversion;

        src = ./.;

        nativeBuildInputs = [
          pkgs.just
          pkgs.gcc
          pkgs.vulkan-headers
          pkgs.wayland

          htils.packages.${system}.htils-threadsafe
        ];

        buildPhase = ''
          runHook preBuild

          sed -i 's|#!/usr/bin/env bash|#!${pkgs.bash}/bin/bash|' justfile
          just release wayland false static

          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall

          mkdir -p $out/lib
          mkdir -p $out/include/butter

          cp lib/libbutter-wayland-release.a $out/lib
          cp -r include/butter/* $out/include/butter

          runHook postInstall
        '';
      };

      packages.butter-wayland-debug = pkgs.stdenv.mkDerivation {
        pname = "butter-wayland-debug";
        version = pversion;

        src = ./.;

        nativeBuildInputs = [
          pkgs.just
          pkgs.gcc
          pkgs.vulkan-headers
          pkgs.wayland

          htils.packages.${system}.htils-debug-threadsafe
        ];

        buildPhase = ''
          runHook preBuild

          sed -i 's|#!/usr/bin/env bash|#!${pkgs.bash}/bin/bash|' justfile
          just debug wayland

          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall

          mkdir -p $out/lib
          mkdir -p $out/include/butter

          cp lib/libbutter-wayland-debug.a $out/lib
          cp -r include/butter/* $out/include/butter

          runHook postInstall
        '';
      };

      packages.butter-x11-release = pkgs.stdenv.mkDerivation {
        pname = "butter-x11";
        version = pversion;

        src = ./.;

        nativeBuildInputs = [
          pkgs.just
          pkgs.gcc
          pkgs.vulkan-headers
          pkgs.libxcb

          htils.packages.${system}.htils-threadsafe
        ];

        buildPhase = ''
          runHook preBuild

          sed -i 's|#!/usr/bin/env bash|#!${pkgs.bash}/bin/bash|' justfile
          just release x11

          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall

          mkdir -p $out/lib
          mkdir -p $out/include/butter

          cp lib/libbutter-x11-release.so $out/lib
          cp -r include/butter/* $out/include/butter

          runHook postInstall
        '';
      };

      packages.butter-x11-release-static = pkgs.stdenv.mkDerivation {
        pname = "butter-x11-static";
        version = pversion;

        src = ./.;

        nativeBuildInputs = [
          pkgs.just
          pkgs.gcc
          pkgs.vulkan-headers
          pkgs.libxcb

          htils.packages.${system}.htils-threadsafe
        ];

        buildPhase = ''
          runHook preBuild

          sed -i 's|#!/usr/bin/env bash|#!${pkgs.bash}/bin/bash|' justfile
          just release x11 false static

          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall

          mkdir -p $out/lib
          mkdir -p $out/include/butter

          cp lib/libbutter-x11-release.a $out/lib
          cp -r include/butter/* $out/include/butter

          runHook postInstall
        '';
      };

      packages.butter-x11-debug = pkgs.stdenv.mkDerivation {
        pname = "butter-x11-debug";
        version = pversion;

        src = ./.;

        nativeBuildInputs = [
          pkgs.just
          pkgs.gcc
          pkgs.vulkan-headers
          pkgs.libxcb

          htils.packages.${system}.htils-debug-threadsafe
        ];

        buildPhase = ''
          runHook preBuild

          sed -i 's|#!/usr/bin/env bash|#!${pkgs.bash}/bin/bash|' justfile
          just debug x11

          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall

          mkdir -p $out/lib
          mkdir -p $out/include/butter

          cp lib/libbutter-x11-debug.a $out/lib
          cp -r include/butter/* $out/include/butter

          runHook postInstall
        '';
      };

      devShells.default = pkgs.mkShell {
        name = "butter-dev";

        packages = with pkgs; [
          clang-tools
          just
          nixd
          bear
          vulkan-tools
          shaderc
          tokei
        ];

        nativeBuildInputs = with pkgs; [
          mold
        ];

        buildInputs = with pkgs; [
          htils.packages.${system}.htils-threadsafe
          bread.packages.${system}.bread-wayland-release
          bread.packages.${system}.bread-x11-release

          vulkan-headers
          vulkan-validation-layers
          vulkan-loader
          libxcb-wm
          libxcb
          libxkbcommon
          wayland
        ];

        shellHook = ''
          export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${pkgs.libxcb-wm}/lib"
          export NIX_LDFLAGS="-rpath ${pkgs.libxcb-wm}/lib  $NIX_LDFLAGS"
          export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d''${VK_LAYER_PATH:+:}$VK_LAYER_PATH"
          export LSAN_OPTIONS="suppressions=lsan.supp"
          export ASAN_OPTIONS="suppressions=asan.supp:halt_on_error=0:abort_on_error=0"
        '';
      };
    });
}
