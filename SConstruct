# -*- Mode: Python -*-

bare_env = Environment(CXX="clang++",
                       CC="clang",
                       AS="nasm",
                       LINK="ld",
                       CCFLAGS="-Wall -O2 -g -pipe -march=haswell -ffreestanding",
                       CXXFLAGS="-std=c++14",
                       CFLAGS="-std=c11",
                       ASFLAGS="-g -F dwarf -O5 -felf64",
                       LINKFLAGS="-g -T standalone.lds -N")

baresifter = bare_env.Program(target="baresifter.elf64", source = ["start.asm", "main.cpp"])
Depends(baresifter, "standalone.lds")

Command("baresifter.elf32", baresifter, "objcopy -O elf32-i386 $SOURCE $TARGET")
