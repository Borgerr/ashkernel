#pragma once
#include "../common.h"

/*
 * kernel panics!
 * wrapped in a `do while(0)` -- executes once and can more easily be deployed
 * gives source file name (__FILE__) and line (__LINE__)
 * macro allows for multiple arguments with ##__VA_ARGS__
 */
#define PANIC(fmt, ...)                                                         \
    do {                                                                        \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);   \
        while (1) {}                                                            \
    } while (0)                                                                 \

/*
 * --------------------------------------------------------------------------------
 * MEMORY MANAGEMENT
 * --------------------------------------------------------------------------------
 */

paddr_t alloc_pages(uint32_t n);    // paddr_t from common.h

/*
 * ----------------------------------------------------------------------------------
 * PROCESS OBJECTS
 * ----------------------------------------------------------------------------------
 */

#define PROCS_MAX       8
#define PROC_UNUSED     0
#define PROC_RUNNABLE   1

struct proc {
    int pid;
    enum proc_state { UNUSED, RUNNABLE } state;
    vaddr_t sp;
    uint32_t *page_table;
    uint8_t kern_stack[8192];   // user's GPRs, ret addr, etc, as well as kernel's vars
};

struct proc *init_proc(uint32_t);
void yield(void);

