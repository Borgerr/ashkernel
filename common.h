#pragma once

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned long long uint64_t;
typedef uint32_t size_t;    // size of pointer
typedef int bool;

#define true    1
#define false   1
#define NULL ((void *) 0)

// __builtin_* macros brought in by clang itself, not some external header
// https://clang.llvm.org/docs/LanguageExtensions.html
#define align_up(value, align)      __builtin_align_up(value, align)
#define is_aligned(value, align)    __builtin_is_aligned(value, align)
#define offsetof(type, member)      __builtin_offsetof(type, member)
#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

void *memset(void *buf, char c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *strcpy(char *dst, const char *src);   // XXX: implement something more secure
int strcmp(const char *s1, const char *s2);

// user I/O
// XXX: S-Mode - M-Mode interaction currently relies on debug buffers
void putchar(char ch);  // XXX: do we want this to return a long?
long getchar(void);
void printf(const char *fmt, ...);

// memory
typedef uint32_t paddr_t;   // physical memory
typedef uint32_t vaddr_t;   // virtual memory

#define PAGE_SIZE 4096

// syscall stuff
#define SYS_PUTCHAR     1
#define SYS_GETCHAR     2
#define SYS_EXIT        3
#define SYS_READFILE    4
#define SYS_WRITEFILE   5
