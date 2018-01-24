# -*- Mode: Python -*-

bare_env = Environment(CXX="clang++",
                       AS="nasm",
                       LINK="clang++",
                       CCFLAGS="-Wall -Os -flto -g -pipe -march=haswell -ffreestanding -mno-red-zone -mno-mmx -mno-sse -mno-avx -mno-avx2",
                       CXXFLAGS="-std=c++14 -fno-threadsafe-statics -fno-rtti -fno-exceptions",
                       ASFLAGS="-g -F dwarf -O5 -felf64",
                       LINKFLAGS="-flto -nostdlib -g -Wl,-T,standalone.lds -Wl,-N")

baresifter = bare_env.Program(target="baresifter.elf64",
                              source = [
                                  "start.asm",
                                  "entry.asm",
                                  "main.cpp",
                                  "idt.cpp",
                                  "io.cpp",
                                  "util.cpp",
                                  "x86.cpp",
                              ])
Depends(baresifter, "standalone.lds")

Command("baresifter.elf32", baresifter, "objcopy -O elf32-i386 $SOURCE $TARGET")
