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

      platformInputs = platform:
        if platform == "wayland"
        then [pkgs.wayland]
        else [pkgs.libxcb];

      mkButter = {
        name,
        profile,
        artifact,
        pc,
        platform,
        loader ? false,
        debug ? false,
      }: let
        artifactStem =
          pkgs.lib.removePrefix "lib"
          (pkgs.lib.removeSuffix ".so"
            (pkgs.lib.removeSuffix ".a" (baseNameOf artifact)));

        htilsPkg =
          if debug
          then htils.packages.${system}.htils-debug-threadsafe
          else htils.packages.${system}.htils-threadsafe;
      in
        pkgs.stdenv.mkDerivation {
          pname = name;
          version = pversion;

          src = ./.;

          nativeBuildInputs =
            [
              conjure.packages.${system}.default
              pkgs.gcc
              pkgs.mold
              pkgs.vulkan-headers
              htilsPkg
            ]
            ++ platformInputs platform
            ++ pkgs.lib.optional loader pkgs.vulkan-loader;

          buildPhase = ''
            runHook preBuild
            conjure as ${profile} build
            runHook postBuild
          '';

          installPhase = ''
            runHook preInstall
            mkdir -p $out/lib/pkgconfig
            mkdir -p $out/include/butter

            cp ${artifact} $out/lib
            sed -e "s|^prefix=.*|prefix=$out|" \
              -e "s|^libdir=.*|libdir=$out/lib|" \
              lib/${profile}/pkgconfig/${artifactStem}.pc \
              > $out/lib/pkgconfig/${pc}

            cp -r include/butter/* $out/include/butter

            runHook postInstall
          '';
        };
    in {
      packages = {
        butter-wayland-release = mkButter {
          name = "butter-wayland";
          profile = "wayland-release";
          artifact = "lib/wayland-release/libbutter-wayland.so";
          pc = "butter-wayland.pc";
          platform = "wayland";
          loader = true;
        };

        butter-wayland-release-static = mkButter {
          name = "butter-wayland-static";
          profile = "wayland-release-static";
          artifact = "lib/wayland-release-static/libbutter-wayland.a";
          pc = "butter-wayland-static.pc";
          platform = "wayland";
        };

        butter-wayland-debug = mkButter {
          name = "butter-wayland-debug";
          profile = "wayland-debug";
          artifact = "lib/wayland-debug/libbutter-wayland-debug.a";
          pc = "butter-wayland-debug.pc";
          platform = "wayland";
          debug = true;
        };

        butter-x11-release = mkButter {
          name = "butter-x11";
          profile = "x11-release";
          artifact = "lib/x11-release/libbutter-x11.so";
          pc = "butter-x11.pc";
          platform = "x11";
          loader = true;
        };

        butter-x11-release-static = mkButter {
          name = "butter-x11-static";
          profile = "x11-release-static";
          artifact = "lib/x11-release-static/libbutter-x11.a";
          pc = "butter-x11-static.pc";
          platform = "x11";
        };

        butter-x11-debug = mkButter {
          name = "butter-x11-debug";
          profile = "x11-debug";
          artifact = "lib/x11-debug/libbutter-x11-debug.a";
          pc = "butter-x11-debug.pc";
          platform = "x11";
          debug = true;
        };
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
