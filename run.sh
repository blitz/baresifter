#!/usr/bin/env bash

set -e -u

KERNEL=src/baresifter.x86_64.elf

if [ $# -gt 0 ]; then
    KERNEL=$1
    shift
fi

# Qemu only boots 32-bit ELFs
if file "$KERNEL" | grep -q "ELF 64-bit"; then
    COPIED_KERNEL=$(mktemp)
    trap "rm -rf $COPIED_KERNEL" EXIT

    objcopy -O elf32-i386 "$KERNEL" "$COPIED_KERNEL"
    KERNEL=$COPIED_KERNEL
fi

qemu-system-x86_64 \
     -enable-kvm -cpu host \
     -no-reboot \
     -display none -vga none -debugcon stdio \
     -kernel "$KERNEL" \
     -append "$*"
