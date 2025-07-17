# ashkernel (WIP)

This project aims to be a POSIX-compliant kernel.
Overall, I want to be able to boot into a fully functioning shell,
and have most, if not all GNU core-utils supported.
Maybe uutils if they're not too complicated to port.

## Usage and Prerequisites

This is currently only built and tested on Linux,
largely because I do most/all of my development there.
If you're wanting support on another platform, contact me, and we'll figure something out.

### Package Requirements

To fully run this project, you'll need whatever packages your system has for the following programs:

- `clang` (for fedora, this is `clang19`)
- `qemu-system-riscv32` (for fedora, this is `qemu-system-riscv-core`. may change depending on target architecture in the future)
- `gdb` (for fedora, this is `gdb`)
- `llvm-objcopy` (for fedora, this is `llvm`)
- `make` (for fedora, this is `make`)

### Running

Currently the only real way to run the kernel is with `make run-user`.
You can also use `make debug-user` for stepping through the source and instructions,
though I would recommend this only if you've had prior experience with debugging C or assembly.

## Goals

Goals, and reaching them, may be altered depending on my time availability.
Right now I'd like to keep this list relatively short, but may add another file
if I start getting loads of ideas I want to implement.

- [ ] Implement a basic file system over disk I/O
- [ ] Implement a UDP stack (TCP would also be nice, but more complicated)
- [ ] More sophisticated memory management beyond bump allocation
- [ ] Dynamically load processes
- [ ] Disk interrupt handling
- [ ] x86_64 support (maybe more generally, boot on real hardware)

