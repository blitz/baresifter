# -*- Mode: Python -*-

import os

capstone_defines = [
    "CAPSTONE_DIET",            # No string mnemonics
    "CAPSTONE_HAS_X86",
    "CAPSTONE_X86_ATT_DISABLE",
]

bare_env = Environment(CXX=os.environ.get("CXX", "clang++"),
                       CC=os.environ.get("CC", "clang"),
                       LINK=os.environ.get("CXX", "clang++"),
                       AR=os.environ.get("AR", "llvm-ar"),
                       AS="nasm",
                       CCFLAGS="-Wall -O2 -flto -g -pipe -march=haswell -ffreestanding -nostdinc -mno-red-zone -mno-avx -mno-avx2 -fno-asynchronous-unwind-tables",
                       CXXFLAGS="-std=c++17 -fno-threadsafe-statics -fno-rtti -fno-exceptions -nostdinc++",
                       ASFLAGS="-g -F dwarf -O5 -felf64",
                       CPPPATH=["#include", "#capstone/include"],
                       CPPDEFINES=capstone_defines,
                       LINKFLAGS="-flto -nostdlib -g -Xlinker -n -Xlinker -T -Xlinker")

capstone_src = [
    "capstone/cs.c",
    "capstone/MCInst.c",
    "capstone/MCInstrDesc.c",
    "capstone/MCRegisterInfo.c",
    "capstone/SStream.c",
    "capstone/utils.c",
    "capstone/arch/X86/X86Disassembler.c",
    "capstone/arch/X86/X86DisassemblerDecoder.c",
    "capstone/arch/X86/X86IntelInstPrinter.c",
    "capstone/arch/X86/X86Mapping.c",
    "capstone/arch/X86/X86Module.c",
]

musl_src = Glob("musl/*.c")

version_inc = Command("version.inc", [], "git describe --always --dirty | sed -E 's/^(.*)$/\"\\1\"/' > $TARGET")
AlwaysBuild(version_inc)

baresifter = bare_env.Program(target="baresifter.elf64",
                              source = ["standalone.lds"] + Glob("*.asm") + Glob("*.cpp")
                                       + capstone_src + musl_src)

Command("baresifter.elf32", baresifter, "objcopy -O elf32-i386 $SOURCE $TARGET")
