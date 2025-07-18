#include "../common.h"
#include "../user/user.h"

// kind of works more like a terminal emulator
// but really currently just for verifying reading and writing characters
void main(void)
{
    while (1) {
        printf("-> ");
        char cmdline[128];      // XXX: input stored on the stack
                                // probably safe since we have a check for the length
        for (int i = 0;; i++) {
            char ch = getchar();
            putchar(ch);
            if (i == sizeof(cmdline) - 1) {
                printf("\n\nhold up youngster, you're yapping too much\n\n");
                break;
            } else if (ch == '\r') {
                printf("\n");
                cmdline[i] = '\0';
                break;
            } else {
                cmdline[i] = ch;
            }
        }

        if (strcmp(cmdline, "hello") == 0)
            printf("hey!\n");
        else if (strcmp(cmdline, "break shit") == 0)    // XXX: TESTING ONLY. Remove once you want shit to not break.
            break;
        else if (strcmp(cmdline, "readfile") == 0) {
            char buf[128];
            int len = readfile("hello.txt", buf, sizeof(buf));
            buf[len] = '\0';
            printf("%s\n", buf);
        } else if (strcmp(cmdline, "writefile") == 0) {
            writefile("hello.txt", "ashkernel, reporting in.\n", 26);
        }
        else
            printf("unrecognized command: %s\n", cmdline);
    }
    // user/user.c start (previous function in the backtrace)
    // calls the exit syscall after this one
}
