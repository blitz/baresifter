let
  sources = import ./nix/sources.nix;
  pkgs = import sources.nixpkgs {};
in
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    # Building
    scons
    gnused
    git
    nasm

    # Dependency Management
    niv

    # Running tests
    qemu
  ];
}
