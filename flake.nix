{
  description = "Debugger";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = {self, nixpkgs}:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      packages.${system}.default = pkgs.stdenv.mkDerivation {
        pname = "result";
        version = "0.1.0";
        src = ./.;

        nativeBuildInputs = [
            pkgs.cmake
            pkgs.gcc
            pkgs.pkg-config
            pkgs.ninja
        ];

        buildInputs = [
            pkgs.libdwarf
            pkgs.ncurses
            pkgs.zstd
        ];

        buildPhase = ''
          cmake --preset=DefaultDebug ..
          cmake --build . -- -j 8
        '';

        installPhase = ''
          mkdir -p $out/bin
          cp debugger $out/bin
        '';
      };

      devShells.${system}.default = pkgs.mkShell {
        buildInputs = [
            pkgs.cmake
	    pkgs.gcc
            pkgs.pkg-config
            pkgs.ninja
            pkgs.libdwarf
            pkgs.ncurses
            pkgs.zstd
        ];
        shellHook = ''
        '';
      };
    };

}
