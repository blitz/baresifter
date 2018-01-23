# -*- Mode: Python -*-

bare_env = Environment(CXX="clang++",
                       CC="clang",
                       AS="nasm",
                       LINK="ld",
                       CCFLAGS="-Wall -O2 -g -pipe -march=haswell -ffreestanding",
                       CXXFLAGS="-std=c++14",
                       CFLAGS="-std=c11",
                       ASFLAGS="-g -O5 -f elf64",
                       LINKFLAGS="-g -T standalone.lds -N")

baresifter = bare_env.Program(target="baresifter.elf", source = ["start.asm", "main.cpp"])
Depends(baresifter, "standalone.lds")
