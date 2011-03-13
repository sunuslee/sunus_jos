/* See COPYRIGHT for copyright information. */

#ifndef JOS_INC_ASSERT_H
#define JOS_INC_ASSERT_H

#include <inc/stdio.h>

void _warn(const char*, int, const char*, ...);
void _panic(const char*, int, const char*, ...) __attribute__((noreturn));
void _sunusdbg(int DBG, int ACT,const char *func, const char *file,int line, const char *fmt, ...);
#define warn(...) _warn(__FILE__, __LINE__, __VA_ARGS__)
#define panic(...) _panic(__FILE__, __LINE__, __VA_ARGS__)
#define sunus_dbg(DBG,ACT,func,...)  _sunusdbg(DBG,ACT,#func,__FILE__,__LINE__, __VA_ARGS__)
#define assert(x)		\
	do { if (!(x)) panic("assertion failed: %s", #x); } while (0)
/* SUNUS DEBUG MARCO */

#define DEBUG(on,value) do {                                                    \
                        if(!on)                                                 \
                                break;                                          \
                        cprintf("DEBUG INFO : the %s is %08x\n",#value,(uint32_t)(value)); \
                        if(getchar() == 'c')                            \
                                break;                                  \
                        }while(1)

// static_assert(x) will generate a compile-time error if 'x' is false.
#define static_assert(x)	switch (x) case 0: case (x):

#endif /* !JOS_INC_ASSERT_H */
