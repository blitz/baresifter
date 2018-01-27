# -*- Mode: Python -*-

bare_env = Environment(CXX="clang++",
                       AS="nasm",
                       LINK="clang++",
                       CCFLAGS="-Wall -Os -flto -g -pipe -march=haswell -ffreestanding -mno-red-zone -mno-mmx -mno-sse -mno-avx -mno-avx2 -fno-asynchronous-unwind-tables",
                       CXXFLAGS="-std=c++17 -fno-threadsafe-statics -fno-rtti -fno-exceptions",
                       ASFLAGS="-g -F dwarf -O5 -felf64",
                       LINKFLAGS="-flto -nostdlib -g -Xlinker -T")

version_inc = Command("version.inc", [], "git describe --always --dirty | sed -E 's/^(.*)$/\"\\1\"/' > $TARGET")
AlwaysBuild(version_inc)

baresifter = bare_env.Program(target="baresifter.elf64",
                              source = ["standalone.lds"] + Glob("*.asm") + Glob("*.cpp"))

Command("baresifter.elf32", baresifter, "objcopy -O elf32-i386 $SOURCE $TARGET")
