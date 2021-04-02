let
  sources = import ./nix/sources.nix;
  nixpkgs = sources.nixpkgs;
  pkgs = import nixpkgs {};

  local = import ./nix/release.nix { inherit sources nixpkgs pkgs; };
in

pkgs.mkShell {
  inputsFrom = [ local.baresifter ];

  nativeBuildInputs = with pkgs; [
    # Dependency Management
    niv

    # Running tests
    local.baresifter-run
  ];
}
