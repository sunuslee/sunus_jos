// buggy hello world -- unmapped pointer passed to kernel
// kernel should destroy user environment in response

#include <inc/lib.h>

void
umain(void)
{
//    	cprintf("hello address = %p\n",(char *)1);
//	sys_cputs((char*)1, 1);
        cprintf("i am cprintf!:\n");
        sys_cputs((char *)"i am sys_cputs", 32);
}

