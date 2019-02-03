# -*- Mode: Python -*-

import itertools
import os

version_inc = Command("common/version.inc", [],
                      "git describe --always --dirty | sed -E 's/^(.*)$/\"\\1\"/' > $TARGET")
AlwaysBuild(version_inc)

capstone_defines = [
    "CAPSTONE_DIET",            # No string mnemonics
    "CAPSTONE_HAS_X86",
    "CAPSTONE_X86_ATT_DISABLE",
]

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

common_cc_flags = "-Wall -O2 -g -pipe -ffreestanding -nostdinc -mno-avx -mno-avx2 -fno-asynchronous-unwind-tables"
common_cxx_flags = "-std=c++14 -fno-threadsafe-statics -fno-rtti -fno-exceptions -nostdinc++"

common_bare_env = Environment(CXX=os.environ.get("CXX", "clang++"),
                              CC=os.environ.get("CC", "clang"),
                              LINK=os.environ.get("CXX", "clang++"),
                              AR=os.environ.get("AR", "llvm-ar"),
                              ENV = os.environ,
                              AS="nasm",
                              CCFLAGS=common_cc_flags,
                              CXXFLAGS=common_cxx_flags,
                              OBJSUFFIX=".${ARCH_NAME}.o",
                              PROGSUFFIX=".${ARCH_NAME}.elf",
                              ASFLAGS="-O5",
                              CPPPATH=["#$ARCH_NAME/include", "#common/include", "#include", "#capstone/include"],
                              CPPDEFINES=capstone_defines,
                              LINKFLAGS="-nostdlib -g -Xlinker -n -Xlinker -T -Xlinker")

bare64_env = common_bare_env.Clone(ARCH_NAME="x86_64")
bare64_env.Append(CCFLAGS=" -m64 -march=x86-64 -mno-red-zone",
                  ASFLAGS=" -felf64")

bare32_env = common_bare_env.Clone(ARCH_NAME="x86_32")
bare32_env.Append(CCFLAGS=" -m32 -march=i686",
                  ASFLAGS=" -felf32")

for env in [bare64_env, bare32_env]:
    source_directories = [".", "common", "musl", "$ARCH_NAME"]
    if env["ARCH_NAME"] == "x86_32":
        source_directories += ["compiler-rt"]

    source_files = [files
                    for directory in source_directories
                    for extension in ["c", "cpp", "asm"]
                    for files in env.Glob("{}/*.{}".format(directory, extension)) ]
    source_files += capstone_src

    baresifter = env.Program(target="baresifter", source = ["$ARCH_NAME/standalone.lds"] + source_files)
    if env["ARCH_NAME"] == "x86_64":
        Command("baresifter.elf32", baresifter, "objcopy -O elf32-i386 $SOURCE $TARGET")
