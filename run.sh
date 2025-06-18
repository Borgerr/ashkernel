#!/bin/env bash

set -xue

QEMU=qemu-system-riscv32    # QEMU command for riscv32

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

