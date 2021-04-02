# Baresifter

![Tests](https://github.com/blitz/baresifter/actions/workflows/test.yml/badge.svg)
[![stability-experimental](https://img.shields.io/badge/stability-experimental-orange.svg)](https://github.com/emersion/stability-badges#experimental)
![GitHub](https://img.shields.io/github/license/blitz/baresifter.svg)
![GitHub commit activity](https://img.shields.io/github/commit-activity/m/blitz/baresifter)

Baresifter is a 64-bit x86 instruction set fuzzer modeled
after [Sandsifter](https://github.com/xoreaxeaxeax/sandsifter). In contrast to
Sandsifter, Baresifter is intended to run bare-metal without any operating
system.

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
E <exc> O <capstone-instruction-id> <status> | <instruction hex bytes>
```

`exc` is the CPU exception that was triggered, when baresifter tried to execute
the instruction. Exception 1 (#DB) indicates that an instruction was
successfully executed. The `capstone-instruction-id` is an integer that
represents the instruction that Capstone decoded. A zero in this field means
that Capstone could not decode the instruction. `status` is currently one of
`BUG` (indicating a capstone bug), `UNKN` (indicating an undocumented
instruction), or `OK` (nothing interesting was found).

A concrete example looks like this:

```
E 0E O 0008 OK   | 00 14 6D 00 00 00 00
E 01 O 0000 UNKN | 0F 0D 3E
E 01 O 010A BUG  | 66 E9 00 00 00 00
```

The first line is an instruction that decoded successfully and generated a page
fault when executing (exception 0xE). Capstone knows this instruction.

The second line is an undocumented instruction, i.e. the CPU executed it
successfully (or at least didn't throw an undefined opcode exception), but
Capstone has no idea what it is.

The third line is
a [Capstone bug](https://github.com/aquynh/capstone/pull/776). Here both the CPU
and Capstone both decoded an instruction, the CPU was able to execute it, but
Capstone and the CPU disagree on the length of that instruction.
