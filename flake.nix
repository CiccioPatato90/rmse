{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=23.11";
    flake-utils.url = "github:numtide/flake-utils";
    nur-kapack = {
      url = "github:oar-team/nur-kapack/master";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.flake-utils.follows = "flake-utils";
    };
    intervalset-flake = {
      url = "git+https://framagit.org/batsim/intervalset";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.nur-kapack.follows = "nur-kapack";
      inputs.flake-utils.follows = "flake-utils";
    };
    batprotocol-flake = {
      url = "git+https://framagit.org/batsim/batprotocol";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.nur-kapack.follows = "nur-kapack";
      inputs.flake-utils.follows = "flake-utils";
    };
    batsim-flake = {
      url = "git+https://framagit.org/batsim/batsim?ref=batprotocol";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.nur-kapack.follows = "nur-kapack";
      inputs.batprotocol.follows = "batprotocol-flake";
      inputs.intervalset.follows = "intervalset-flake";
      inputs.flake-utils.follows = "flake-utils";
    };
  };

  outputs = { self, nixpkgs, nur-kapack, intervalset-flake, flake-utils, batprotocol-flake, batsim-flake }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        py = pkgs.python3;
        pyPkgs = pkgs.python3Packages;
        kapack = nur-kapack.packages.${system};
        batprotopkgs = batprotocol-flake.packages-debug.${system};
        intervalsetpkgs = intervalset-flake.packages-debug.${system};
        batpkgs = batsim-flake.packages-debug.${system};
      in rec {
        devShells = rec {
          default = pkgs.mkShell {
            buildInputs = with pkgs; [
              # program deps
              batpkgs.batsim

              # libraries deps
              batprotopkgs.batprotocol-cpp
              intervalsetpkgs.intervalset
              nlohmann_json

              # build deps
              meson ninja pkg-config
            ];

            hardeningDisable = [ "fortify" ];
            shellHook = ''
              echo '⚠️ DO NOT USE THIS SHELL FOR A REAL EXPERIMENT! ⚠️'
              echo 'This shell is meant to get started with batsim (batprotocol version)'
              echo 'All softwares have been compiled in debug mode, which is extremely slow'
            '';
          };
        };
      }
    );
}
