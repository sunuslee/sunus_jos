// Called from entry.S to get us going.
// entry.S already took care of defining envs, pages, vpd, and vpt.

#include <inc/lib.h>

extern void umain(int argc, char **argv);

volatile struct Env *env;
char *binaryname = "(PROGRAM NAME UNKNOWN)";

#define S_DBG_PRINT_ENVID 0
        void
libmain(int argc, char **argv)
{
        // set env to point at our env structure in envs[].
        // LAB 3: Your code here.
        env = 0;
        // DEC 15,2010 sunus
        envid_t et = sys_getenvid();
#if S_DBG_PRINT_ENVID
        cprintf("Now curenvID = %d\n",ENVX(et));
#endif
        env = &envs[ENVX(et)];  
        // save the name of the program so that panic() can use it
        if (argc > 0)
                binaryname = argv[0];

        // call user main routine
        umain(argc, argv);

        // exit gracefully
        exit();
}

/* 
 * this is how sys_getenvid implemented 
 * envid_t sys_getenvid()
 *  	{
 * 		return curenv->env_id; // JUST 1 !!
 * 	}	
 */

