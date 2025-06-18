#!/bin/env bash

set -xue    # nice for bash scripts. quit on bad thing happening :)

QEMU=qemu-system-riscv32    # QEMU command for riscv32

# compile
CC=clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fno-stack-protector -ffreestanding -nostdlib"
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
    kernel.c

# basic "virtual" machine
# default firmware resolves as OpenSBI
# QEMU's stdout/stdin connected to VM's serial. mon: switches to QEMU monitor with Ctrl+A and C
# emulator stops without rebooting on VM crash
# QEMU starts without a GUI
$QEMU \
    -machine virt \
    -bios default \
    -serial mon:stdio \
    --no-reboot \
    -nographic

