#pragma once

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

