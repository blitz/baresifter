{ sources ? import ./sources.nix
, nixpkgs ? sources.nixpkgs
, pkgs ? import nixpkgs { }
}:
rec {
  baresifter = pkgs.callPackage ./build.nix { };

  baresifter-run = pkgs.runCommandNoCC "baresifter-run" {
    nativeBuildInputs = [ pkgs.makeWrapper ];
  } ''
    mkdir -p $out/bin

    install -m 0755 ${../run.sh} $out/bin/baresifter-run
    patchShebangs $out/bin/baresifter-run

    wrapProgram $out/bin/baresifter-run \
      --prefix PATH : ${pkgs.lib.makeBinPath (with pkgs; [ qemu file binutils-unwrapped ])}
  '';

  test-qemu-tcg = pkgs.runCommandNoCC "test-qemu-tcg" {
    nativeBuildInputs = [ baresifter-run ];
  } ''
    timeout 120 baresifter-run tcg ${baresifter}/share/baresifter/baresifter.x86_64.elf \
      tcg stop_after=100 | tee out.log

    grep -Fq ">>> Done" out.log || echo "Test did not complete successfully."
    cp out.log $out
  '';

}
