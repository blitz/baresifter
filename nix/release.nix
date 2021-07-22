{ sources ? import ./sources.nix
, nixpkgs ? sources.nixpkgs
, pkgs ? import nixpkgs { }
}:
let
  baresifter = pkgs.callPackage ./build.nix { };

  baresifter-run = pkgs.runCommandNoCC "baresifter-run"
    {
      nativeBuildInputs = [ pkgs.makeWrapper ];
    } ''
    mkdir -p $out/bin

    install -m 0755 ${../tools/baresifter-run} $out/bin/baresifter-run
    patchShebangs $out/bin/baresifter-run

    wrapProgram $out/bin/baresifter-run \
      --prefix PATH : ${pkgs.lib.makeBinPath (with pkgs; [ qemu file binutils-unwrapped ])}
  '';

  testcase = { mode, binary }: pkgs.runCommandNoCC "test-qemu-tcg"
    {
      nativeBuildInputs = [ baresifter-run ];
    } ''
    timeout 120 baresifter-run ${mode} ${baresifter}/share/baresifter/${binary} \
      stop_after=100 | tee out.log

    grep -Fq ">>> Done" out.log || echo "Test did not complete successfully."
    cp out.log $out
  '';
in
{
  inherit baresifter baresifter-run;

  test-x86_64-tcg = testcase { mode = "tcg"; binary = "baresifter.x86_64.elf"; };
  test-x86_32-tcg = testcase { mode = "tcg"; binary = "baresifter.x86_32.elf"; };
}
