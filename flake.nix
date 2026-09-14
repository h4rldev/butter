{
  description = "The dev flake for butter, a vulkan renderer.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    conjure.url = "git+ssh://git@codeberg.org/h4rl/conjure";
    bread.url = "git+ssh://git@codeberg.org/h4rl/bread";
    htils.url = "github:h4rldev/htils";
  };

  outputs = {
    self,
    flake-utils,
    nixpkgs,
    bread,
    htils,
    conjure,
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
          conjure.packages.${system}.default
          pkgs.gcc
          pkgs.vulkan-loader
          pkgs.vulkan-headers
          pkgs.wayland

          htils.packages.${system}.htils-threadsafe
        ];

        buildPhase = ''
          runHook preBuild

          conjure as wayland-release build

          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall

          mkdir -p $out/lib/pkgconfig
          mkdir -p $out/include/butter

          mkdir -p $out/lib/pkgconfig
          sed -e "s|^prefix=.*|prefix=$out|" -e "s|^libdir=.*|libdir=$out/lib|" lib/wayland-release/pkgconfig/butter-wayland.pc > $out/lib/pkgconfig/butter-wayland.pc

          cp lib/wayland-release/libbutter-wayland.so $out/lib
          cp -r include/butter/* $out/include/butter

          runHook postInstall
        '';
      };

      packages.butter-wayland-release-static = pkgs.stdenv.mkDerivation {
        pname = "butter-wayland-static";
        version = pversion;

        src = ./.;

        nativeBuildInputs = [
          conjure.packages.${system}.default
          pkgs.gcc
          pkgs.vulkan-headers
          pkgs.wayland

          htils.packages.${system}.htils-threadsafe
        ];

        buildPhase = ''
          runHook preBuild

          conjure as wayland-release-static build

          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall

          mkdir -p $out/lib/pkgconfig
          mkdir -p $out/include/butter
          sed -e "s|^prefix=.*|prefix=$out|" -e "s|^libdir=.*|libdir=$out/lib|" lib/wayland-release-static/pkgconfig/butter-wayland-static.pc > $out/lib/pkgconfig/butter-wayland-static.pc

          cp lib/wayland-release-static/libbutter-wayland.a $out/lib
          cp -r include/butter/* $out/include/butter

          runHook postInstall
        '';
      };

      packages.butter-wayland-debug = pkgs.stdenv.mkDerivation {
        pname = "butter-wayland-debug";
        version = pversion;

        src = ./.;

        nativeBuildInputs = [
          conjure.packages.${system}.default
          pkgs.gcc
          pkgs.vulkan-headers
          pkgs.wayland

          htils.packages.${system}.htils-debug-threadsafe
        ];

        buildPhase = ''
          runHook preBuild

          conjure as wayland-debug build

          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall

          mkdir -p $out/lib/pkgconfig
          mkdir -p $out/include/butter

          sed -e "s|^prefix=.*|prefix=$out|" -e "s|^libdir=.*|libdir=$out/lib|" lib/wayland-debug/pkgconfig/butter-wayland-debug.pc > $out/lib/pkgconfig/butter-wayland-debug.pc
          cp lib/wayland-debug/libbutter-wayland-debug.a $out/lib
          cp -r include/butter/* $out/include/butter

          runHook postInstall
        '';
      };

      packages.butter-x11-release = pkgs.stdenv.mkDerivation {
        pname = "butter-x11";
        version = pversion;

        src = ./.;

        nativeBuildInputs = [
          conjure.packages.${system}.default
          pkgs.gcc
          pkgs.vulkan-loader
          pkgs.vulkan-headers
          pkgs.libxcb

          htils.packages.${system}.htils-threadsafe
        ];

        buildPhase = ''
          runHook preBuild

          conjure as x11-release build

          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall

          mkdir -p $out/lib/pkgconfig
          mkdir -p $out/include/butter

          sed -e "s|^prefix=.*|prefix=$out|" -e "s|^libdir=.*|libdir=$out/lib|" lib/x11-release/pkgconfig/butter-x11.pc > $out/lib/pkgconfig/butter-x11.pc
          cp lib/x11-release/libbutter-x11.so $out/lib
          cp -r include/butter/* $out/include/butter

          runHook postInstall
        '';
      };

      packages.butter-x11-release-static = pkgs.stdenv.mkDerivation {
        pname = "butter-x11-static";
        version = pversion;

        src = ./.;

        nativeBuildInputs = [
          conjure.packages.${system}.default
          pkgs.gcc
          pkgs.vulkan-headers
          pkgs.libxcb

          htils.packages.${system}.htils-threadsafe
        ];

        buildPhase = ''
          runHook preBuild

          conjure as x11-release-static build

          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall

          mkdir -p $out/lib/pkgconfig
          mkdir -p $out/include/butter

          sed -e "s|^prefix=.*|prefix=$out|" -e "s|^libdir=.*|libdir=$out/lib|" lib/x11-release-static/pkgconfig/butter-x11-static.pc > $out/lib/pkgconfig/butter-x11-static.pc
          cp lib/x11-release-static/libbutter-x11.a $out/lib
          cp -r include/butter/* $out/include/butter

          runHook postInstall
        '';
      };

      packages.butter-x11-debug = pkgs.stdenv.mkDerivation {
        pname = "butter-x11-debug";
        version = pversion;

        src = ./.;

        nativeBuildInputs = [
          conjure.packages.${system}.default
          pkgs.gcc
          pkgs.vulkan-headers
          pkgs.libxcb

          htils.packages.${system}.htils-debug-threadsafe
        ];

        buildPhase = ''
          runHook preBuild

          conjure as x11-debug build

          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall

          mkdir -p $out/lib/pkgconfig
          mkdir -p $out/include/butter

          sed -e "s|^prefix=.*|prefix=$out|" -e "s|^libdir=.*|libdir=$out/lib|" lib/x11-debug/pkgconfig/butter-x11-debug.pc > $out/lib/pkgconfig/butter-x11-debug.pc
          cp lib/x11-debug/libbutter-x11-debug.a $out/lib
          cp -r include/butter/* $out/include/butter

          runHook postInstall
        '';
      };

      devShells.default = pkgs.mkShell {
        name = "butter-dev";

        packages = with pkgs; [
          clang-tools
          nixd
          bear
          vulkan-tools
          shaderc
          tokei
          conjure.packages.${system}.default
        ];

        nativeBuildInputs = with pkgs; [
          mold
        ];

        buildInputs = with pkgs; [
          htils.packages.${system}.htils-threadsafe
          bread.packages.${system}.bread-wayland-release
          bread.packages.${system}.bread-x11-release
          bread.packages.${system}.bread-wayland-debug
          bread.packages.${system}.bread-x11-debug

          vulkan-headers
          vulkan-validation-layers
          vulkan-loader
          libxcb-wm
          libxcb-cursor
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
