// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>
#include <inc/env.h>
#include <kern/pmap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>


#define CMDBUF_SIZE	80	// enough for one VGA text line

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "kgb","show some debug infos",mon_kgb},
	{ "alloc_page","alloc_page",mon_alloc_page},
	{ "free_page","free_page",mon_free_page},
	{ "page_status","page_status",mon_page_status},
	{ "backtrace","backtrace regs",mon_backtrace},
	{ "continue","continue",mon_continue},
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

unsigned read_eip();

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start %08x (virt)  %08x (phys)\n", _start, _start - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		(end-_start+1023)/1024);
	return 0;
}


#define PRINT cprintf("ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n",ebp,eip,arg1,arg2,arg3)
#define PRINT_INFO cprintf("\t %s:%d: %s+%d\n ",eip_info.eip_file,eip_info.eip_line,eip_info.eip_fn_name,eip_info.eip_fn_namelen) 
int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{ 
    uint32_t ebp,eip,arg1,arg2,arg3,arg4,arg5;
    struct Eipdebuginfo eip_info;
    ebp = read_ebp();
    for( ; ; ebp = *(uint32_t*)ebp)
      {
	if(ebp == 0)
	    break;
	eip = *((uint32_t *)ebp + 1);
	arg1 =*((uint32_t *)ebp + 2);
	arg2 =*((uint32_t *)ebp + 3);
	arg3 =*((uint32_t *)ebp + 4);
	arg4 =*((uint32_t *)ebp + 5);
	arg5 =*((uint32_t *)ebp + 6);	
	debuginfo_eip(eip,&eip_info); 
	PRINT_INFO;
      }
    return 0;
}

/* Those functions below are coded by sunus */
int
mon_kgb(int argc, char **argv, struct Trapframe *tf)
{
    int i;
    for(i = 0 ; i < argc ; i++)
	cprintf("argv %d is %s\n",i,argv[i]);
    return -2;
}


int
mon_alloc_page(int argc, char **argv, struct Trapframe *tf)
{
    struct Page *newpage = NULL;
    if(page_alloc(&newpage) == 0)
      {
	cprintf("\t new page allocted:%08x\n",page2kva(newpage));
	newpage->pp_ref = 1;
      }
    else
	cprintf("page alloction failed\n");
    return 0;
}


int 
mon_free_page(int argc, char **argv, struct Trapframe *tf)
{
    uint32_t page_addr ;
    struct Page *free_page;
    if(!argv[1])
	return 0;
    else
	page_addr = (uint32_t)str2int(argv[1],16);
    
    if(page_addr%PGSIZE)
      {
	cprintf("illegal address!(has to be multiple of 4kb)\n");
	cprintf("do you mean : %08x or %08x ?\n",ROUNDDOWN(page_addr,PGSIZE),ROUNDUP(page_addr,PGSIZE));
      }
    else
      {
      free_page = (struct Page *)page_addr;
      free_page->pp_ref = 0;
      page_free(free_page);
      cprintf("page free succeeded\n");
      }
    return 0;
}
    
int
mon_page_status(int argc, char **argv, struct Trapframe *tf)
{
    uint32_t page_addr ;
    struct Page *p;
    if(!argv[1])
	return 0;
    else
	page_addr = (uint32_t)str2int(argv[1],16);
    
    if(page_addr%PGSIZE)
      {
	cprintf("illegal address!(has to be multiple of 4kb)\n");
	cprintf("do you mean : %08x or %08x ?\n",ROUNDDOWN(page_addr,PGSIZE),ROUNDUP(page_addr,PGSIZE));
      }
    else
      {
	p = (struct Page *)page_addr;
	cprintf("page is %s\n",(p->pp_ref ? "alloced" : "free"));
      }
    return 0;
}

int mon_continue(int argc, char **argv, struct Trapframe *tf) // if this is the first proc,then tf = NULL 
{
    extern struct Env *curenv;
    void env_run(struct Env *);
    if(tf)
      {
    	tf->tf_eflags &= 0XFFFFFEFF;
    	env_run(curenv);
      }
    return 0;
}

/*Those functions above are coded by sunus*/

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

// return EIP of caller.
// does not work if inlined.
// putting at the end of the file seems to prevent inlining.
unsigned
read_eip()
{
	uint32_t callerpc;
	__asm __volatile("movl 4(%%ebp), %0" : "=r" (callerpc));
	return callerpc;
}
