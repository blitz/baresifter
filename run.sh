#!/bin/sh

set -e

scons -s -j$(nproc) baresifter.elf32
exec qemu-system-x86_64 -enable-kvm -cpu host -no-reboot -vga none -debugcon stdio -kernel baresifter.elf32
