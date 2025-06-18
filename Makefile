# Makefile

# Variables
CC = clang
QEMU = qemu-system-riscv32
CFLAGS = -std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fno-stack-protector -ffreestanding -nostdlib
LDFLAGS = -Wl,-Tkernel.ld -Wl,-Map=kernel.map
OUTPUT = kernel.elf
SRC = kernel.c

# Default target
all: elf

# Build the ELF binary
elf: $(SRC) kernel.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUTPUT) $(SRC)

# Run the ELF binary with QEMU
run: elf
	$(QEMU) \
		-machine virt \
		-bios default \
		-serial mon:stdio \
		--no-reboot \
		-nographic \
		-kernel kernel.elf

# Clean build artifacts
clean:
	rm -f $(OUTPUT) kernel.map

.PHONY: all elf run clean

