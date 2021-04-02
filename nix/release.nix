{ sources ? import ./sources.nix
, nixpkgs ? sources.nixpkgs
, pkgs ? import nixpkgs {}}:
{
  baresifter = pkgs.callPackage ./build.nix {};
}
