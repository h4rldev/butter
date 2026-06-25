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
    in {
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
