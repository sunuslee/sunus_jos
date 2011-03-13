
#include <inc/lib.h>

char *argv0;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: <message>", then causes a breakpoint exception,
 * which causes JOS to enter the JOS kernel monitor.
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);

	// Print the panic message
	if (argv0)
		cprintf("%s: ", argv0);
	cprintf("user panic in %s at %s:%d: ", binaryname, file, line);
	vcprintf(fmt, ap);
	cprintf("\n");

	// Cause a breakpoint exception
	while (1)
		asm volatile("int3");
}


#define ERROR 1
#define FINISHED 2
#define IN 3
#define OUT 4
#define WATCH 5
static const char * const dbg_infos[10] = 
{
        NULL,
        "ERROR",
        "FINISHED",
        "IN",
        "OUT",
        "WATCH",
        "PANIC"
};
void
_sunusdbg(int on,int act,const char *func, const char *file, int line, const char *fmt, ...)
{
        va_list ap;
        if(on)
        {
                cprintf("SUNUS : %s %d %s %s\n",file,line,func,dbg_infos[act]);
                va_start(ap, fmt);
                vcprintf(fmt, ap);
                cprintf("\n");
                if(act == 6)
                        panic("stop!\n");
        }
}

