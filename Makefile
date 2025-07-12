CC = clang
QEMU = qemu-system-riscv32
GDB = gdb
OBJCOPY = /usr/bin/llvm-objcopy

CFLAGS = -std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf \
         -fno-stack-protector -ffreestanding -nostdlib

# Kernel build
KERNEL_LDFLAGS = -Wl,-Tkernel.ld -Wl,-Map=kernel.map
KERNEL_SRC = sys/*.c common.c shell.bin.o
KERNEL_ELF = kernel.elf

# User build
# TODO: come back to this ofc
USER_LDFLAGS = -Wl,-Tuserspace.ld -Wl,-Map=shell.map
USER_SRC = apps/*.c common.c user/*.c
USER_ELF = shell.elf
USER_BIN = shell.bin
USER_BIN_O = shell.bin.o

# Default target
all: $(KERNEL_ELF)

# Kernel ELF depends on shell binary object (user program embedded)
$(KERNEL_ELF): $(KERNEL_SRC) kernel.ld $(USER_BIN_O)
	$(CC) $(CFLAGS) $(KERNEL_LDFLAGS) -o $@ $(KERNEL_SRC)

# Build user ELF, binary, and binary object
$(USER_BIN_O): $(USER_ELF)
	$(OBJCOPY) --set-section-flags .bss=alloc,contents -O binary $< $(USER_BIN)
	$(OBJCOPY) -Ibinary -Oelf32-littleriscv $(USER_BIN) $@

$(USER_ELF): $(USER_SRC) userspace.ld
	$(CC) $(CFLAGS) $(USER_LDFLAGS) -o $@ $(USER_SRC)

# Targets
app: $(USER_ELF) $(USER_BIN_O)
kern_elf: $(KERNEL_ELF)

run-user: all
	$(QEMU) \
		-machine virt \
		-bios default \
		-serial mon:stdio \
		--no-reboot \
		-nographic \
		-kernel $(KERNEL_ELF)

debug-user: all
	# https://www.qemu.org/docs/master/system/gdb.html
	# TODO: should figure out how to kill background on make kill
	$(QEMU) -s -S \
		-machine virt \
		-bios default \
		-serial mon:stdio \
		--no-reboot \
		-nographic \
		-kernel $(KERNEL_ELF) \
		&

	$(GDB) $(KERNEL_ELF) -ex "add-symbol-file shell.elf 0x1000000" -ex "target remote localhost:1234"	# TODO: come back to this

run-no-user: kern_elf
	$(QEMU) \
		-machine virt \
		-bios default \
		-serial mon:stdio \
		--no-reboot \
		-nographic \
		-kernel $(KERNEL_ELF)

clean:
	rm -f *.bin *.bin.o *.elf *.map

.PHONY: all clean app kern_elf run-user run-no-user

