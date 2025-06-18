typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;	// pointers are 32b

extern char __bss[], __bss_end[], __stack_top[];	// taken from kernel.ld

void *memset(void *buf, char c, size_t size)
{
	/*
	 * set membuf at `void *buf` to `char c`
	 * buf assumed to be of size `size`
	 */
	uint8_t *current = (uint8_t *) buf;
	for (size_t i = 0; i < size; i++)
		*current++ = c;
	return buf;
}

void kernel_main(void)
{
	memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

	for (;;) ;
}

// XXX: specific to RISC-V
// with potential hardware ports, move.
__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void)
{
	/*
	 * kernel.ld points to here as start (section attribute)
	 * __attribute__((naked)) removes prelude and epilog for function
	 */
	__asm__ __volatile__(
		"mv sp, %[stack_top]\n"	// set stack pointer
		"j kernel_main\n"	// jump to kernel main
		:
		: [stack_top] "r" (__stack_top)	// %[stack_top] in asm == __stack_top in C
	);
}

