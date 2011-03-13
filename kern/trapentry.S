/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* The TRAPHANDLER macro defines a globally-visible function for handling
 * a trap.  It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 */ 
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
TRAPHANDLER_NOEC(divide_entry,T_DIVIDE);
TRAPHANDLER_NOEC(debug_entry,T_DEBUG);
TRAPHANDLER_NOEC(nmi_entry,T_NMI);
TRAPHANDLER_NOEC(brkpt_entry,T_BRKPT);
TRAPHANDLER_NOEC(oflow_entry,T_OFLOW);
TRAPHANDLER_NOEC(bound_entry,T_BOUND);
TRAPHANDLER_NOEC(illop_entry,T_ILLOP);
TRAPHANDLER_NOEC(device_entry,T_DEVICE);
TRAPHANDLER(dblflt_entry,T_DBLFLT);
TRAPHANDLER(tts_entry,T_TSS);
TRAPHANDLER(segnp_entry,T_SEGNP);
TRAPHANDLER(stack_entry,T_STACK);
TRAPHANDLER(gpflt_entry,T_GPFLT);
TRAPHANDLER(pgflt_entry,T_PGFLT);
TRAPHANDLER_NOEC(fperr_entry,T_FPERR);
TRAPHANDLER(align_entry,T_ALIGN);
TRAPHANDLER_NOEC(mchk_entry,T_MCHK);
TRAPHANDLER_NOEC(simderr_entry,T_SIMDERR);
TRAPHANDLER_NOEC(syscall_entry,T_SYSCALL);
/*
 *DEC 13,sunus
 */

/*lab4 IRQs */
TRAPHANDLER_NOEC(irq_timer_entry,IRQ_OFFSET + IRQ_TIMER);
TRAPHANDLER_NOEC(irq_kbd_entry,IRQ_OFFSET + IRQ_KBD);
TRAPHANDLER_NOEC(irq_serial_entry,IRQ_OFFSET + IRQ_SERIAL);
TRAPHANDLER_NOEC(irq_spurious_entry,IRQ_OFFSET + IRQ_SPURIOUS);
TRAPHANDLER_NOEC(irq_ide_entry,IRQ_OFFSET + IRQ_IDE);
TRAPHANDLER_NOEC(irq_error_entry,IRQ_OFFSET + IRQ_ERROR);
/*lab4 IRQs */
_alltraps:
pushl %ds
pushl %es
pushal 

movl $GD_KD,%ax
movl %ax,%ds
movl %ax,%es

pushl %esp
call trap

popl %esp
popal			/* I don't know if call will ever return !,just in case*/
popl %es
popl %ds
addl $8, %esp
iret


