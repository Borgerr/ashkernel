ENTRY(start)

SECTIONS {
    . = 0x1000000;

    .text :{
        KEEP(*(.text.start));
        *(.text .text.*);
    }

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    .data : ALIGN(4) {
        *(.data .data.*);
    }

    .bss : ALIGN(4) {
        *(.bss .bss.* .sbss .sbss.*);

        . = ALIGN(16);
        . += 64 * 1024; /* 64KB */
        __stack_top = .;

        /*
         * While I'd love to have executables of arbitrary sizes,
         * we currently don't have paging implemented very well
         * and user programs are not lazily loaded into memory.
         * XXX: come back to this. very likely to be an improvement to work on.
         */
        ASSERT(. < 0x1800000, "executable is too large!");
    }
}
