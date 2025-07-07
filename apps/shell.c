#include "../user/user.h"    // TODO: figure out a better organization for this file structure

void main(void)
{
    *((volatile int *) 0x80200000) = 0x1234;    // should trap
    for (;;) ;
}
