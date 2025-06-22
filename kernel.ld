ENTRY(boot)

SECTIONS {
    . = 0x80200000; /* base address */
    /* . symbol is "current address", increments as data is placed, e.g. *(.text) */

    .text :{   /* .text.boot placed at beginning */
        KEEP(*(.text.boot));
        *(.text .text.*);
    }

    /* every other section placed in order after .text */

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    .data : ALIGN(4) {
        *(.data .data.*);
    }

    .bss : ALIGN(4) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    /* kernel stack */
    . = ALIGN(4);   /* address pointer 4B aligned */
    . += 128 * 1024; /* 128KB */
    __stack_top = .;

    /*
     * free memory for the kernel to allocate
     * TODO: rely on UEFI/BIOS/OpenSBI etc for determining available memory regions
     * after init
     */
    . = ALIGN(4096);
    __free_ram = .;
    . += 64 * 1024 * 1024;  /* 64MB, arbitrary, FIXME: come back to this and add more, possibly change for VM */
    __free_ram_end = .;
}

/*
 * if this shit stops working straight up just look on github
 * there are so many nerds trying to post examples it's not even funny
 */

