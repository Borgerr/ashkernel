#pragma once

// from <stdarg.h>, but we can adopt here
// since they're ultimately provided by clang
#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

void printf(const char *fmt, ...);

