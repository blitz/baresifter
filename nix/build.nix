{ stdenv, scons, nasm, gnused }:

let
  version = "0.0.0";
in
stdenv.mkDerivation {
  pname = "baresifter";
  inherit version;

  src = ../src;
  hardeningDisable = [ "all" ];

  nativeBuildInputs = [ scons nasm gnused ];

  BARESIFTER_VERSION = "Nix ${version}";
}
