# Baresifter

![Tests](https://github.com/blitz/baresifter/actions/workflows/test.yml/badge.svg)
[![stability-experimental](https://img.shields.io/badge/stability-experimental-orange.svg)](https://github.com/emersion/stability-badges#experimental)
![GitHub](https://img.shields.io/github/license/blitz/baresifter.svg)
![GitHub commit activity](https://img.shields.io/github/commit-activity/m/blitz/baresifter)

Baresifter is a 64-bit x86 instruction set fuzzer modeled
after [Sandsifter](https://github.com/xoreaxeaxeax/sandsifter). In contrast to
Sandsifter, Baresifter is intended to run bare-metal without any operating
system. It's most easy to set up to run in a virtual machine.

When loaded, the main fuzzing logic runs in ring0 as a tiny kernel. To safely
execute arbitrary instructions, baresifter creates a single executable page in
ring3 user space. For every instruction candidate, baresifter writes the
instruction bytes to this user space page and attempts to execute it by exiting
to user space. It follows the same algorithm as outlined in the original
[Sandsifter paper](https://github.com/xoreaxeaxeax/sandsifter/blob/master/references/domas_breaking_the_x86_isa_wp.pdf) to find interesting instructions and guess instruction length.

## Building and running

The build is currently tested on NixOS and other Linux distributions
with Nix installed. If you haven't done so, [install
Nix](https://nixos.org/download.html).

Then enter a
[nix-shell](https://nixos.org/guides/nix-pills/developing-with-nix-shell.html)
and build baresifter:

```sh
% nix-shell
# Dependencies are fetched. This might take a bit.

nix-shell % scons -C src
```

Once you have built baresifter, you can run it in Qemu:

```sh
# Execute 1000 instructions in Qemu's full emulation mode.
nix-shell % baresifter-run tcg src/baresifter.x86_64.elf stop_after=1000
...

# Run forever with KVM enabled.
nix-shell % baresifter-run kvm src/baresifter.x86_64.elf
...
```

To run baresifter bare-metal, use either grub or
[syslinux](https://www.syslinux.org/wiki/index.php?title=Mboot.c32) and boot
`baresifter.elf32` as multiboot kernel. It will dump instruction traces on the
serial port. The serial port is hardcoded, so you might need to change that:
`git grep serial_output`.

## Interpreting results

Baresifter outputs data in a tabular format that looks like:

```
EXC <exc> <OK|??> | <instruction hex bytes>
```

`exc` is the CPU exception that was triggered, when baresifter tried
to execute the instruction. `OK` indicates the CPU decoded the
instruction, `??` means we probably hit a bug in Baresifter. The rest
are the instruction bytes in hexadecimal.

A concrete example looks like this:

```
EXC 0D OK | 0F 08
EXC 06 OK | 0F 0A
EXC 01 OK | 0F 0D 00
```

The first line is an instruction that decoded successfully and
generated a general protection exception (#GP / 0x0D) when
executing. In this case, it was a `invd` instruction. The second line
is an invalid opcode (0x06). The third line is an instruction that
decoded _and_ executed correctly by running into a #DB (0x01)
exception.

Earlier versions of Baresifter had a built-in disassembler to
disassemble on-the-fly and find interesting discrepancies. This was
removed in favor of offline analysis of the logs, but this offline
analysis does not exist yet.
