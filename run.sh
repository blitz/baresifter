#!/usr/bin/env bash
# Usage: [QEMU_MODE [KERNEL ARGS...]]

set -e -u

KERNEL=src/baresifter.x86_64.elf
QEMU_MODE=tcg

if [ $# -gt 0 ]; then
    QEMU_MODE=$1
    shift
fi

case $QEMU_MODE in
    tcg) QEMU_CPU_FLAGS="-cpu qemu64,+xsave -machine q35,accel=tcg" ;;
    kvm) QEMU_CPU_FLAGS="-cpu host -machine q35,accel=kvm" ;;
    *) echo "Unknown mode $QEMU_MODE" > /dev/stderr; exit 1 ;;
esac

echo "Running Qemu in $QEMU_MODE mode."
echo

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
     $QEMU_CPU_FLAGS \
     -no-reboot \
     -display none -vga none -debugcon stdio \
     -kernel "$KERNEL" \
     -append "$*"
