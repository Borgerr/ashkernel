#include "user.h"

extern char __stack_top[];

__attribute__((noreturn)) void exit(void)
{
    syscall(SYS_EXIT, 0, 0, 0);
    for (;;)    
        printf("process didn't exit!!!\n");     // XXX: hopefully this isn't reached.
                                                // if it is, include a better log mechanism
                                                // or better yet, include the mech before it does
}

__attribute__((section(".text.start")))
__attribute__((naked))
void start(void)
{
    __asm__ __volatile__(
            "mv sp, %[stack_top] \n"
            "call main           \n"
            "call exit           \n"
            :: [stack_top] "r" (__stack_top)
    );
}

/*
 * --------------------------------------------------------------------------------
 * SYSCALLS
 * --------------------------------------------------------------------------------
 */

// XXX: specific to RISC-V
int syscall(int sysno, int arg0, int arg1, int arg2)
{
    register int a0 __asm__("a0") = arg0;
    register int a1 __asm__("a1") = arg1;
    register int a2 __asm__("a2") = arg2;
    register int a3 __asm__("a3") = sysno;

    __asm__ __volatile__("ecall"
            : "=r"(a0)
            : "r"(a0), "r"(a1), "r"(a2), "r"(a3)
            : "memory");
    return a0;
}

long getchar(void)
{
    return syscall(SYS_GETCHAR, 0, 0, 0);
}

void putchar(char c)
{
    syscall(SYS_PUTCHAR, c, 0, 0);
}

int readfile(const char *filename, char *buf, int len)
{
    // TODO: implement
    syscall(SYS_READFILE, (int)filename, (int)buf, (int)len);
}

int writefile(const char *filename, const char *buf, int len)
{
    // TODO: implement
    syscall(SYS_WRITEFILE, (int)filename, (int)buf, (int)len);
}

