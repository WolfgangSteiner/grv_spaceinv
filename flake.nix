{
  description = "SDL2 development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { nixpkgs, ... }:
    let
      system = "x86_64-linux"; # Change if using another architecture
      pkgs = import nixpkgs { inherit system; };
    in {
      devShells.${system}.default = pkgs.mkShell {
        buildInputs = [
          pkgs.SDL2
          pkgs.SDL2_image
          pkgs.SDL2_mixer
          pkgs.SDL2_ttf
          pkgs.SDL2_net
          pkgs.gcc
          pkgs.clang
          pkgs.tinycc
          pkgs.ccache
        ];

        shellHook = ''
          echo "SDL2 development environment activated!"
          export LD_LIBRARY_PATH=build:lib/grv/build:$LD_LIBRARY_PATH
        '';
      };
    };
}
