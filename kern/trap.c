#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>
#include <inc/string.h>

#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/env.h>
#include <kern/syscall.h>
#include <kern/sched.h>
#include <kern/kclock.h>
#include <kern/picirq.h>

static struct Taskstate ts;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = {
        sizeof(idt) - 1, (uint32_t) idt
};


static const char *trapname(int trapno)
{
        static const char * const excnames[] = {
                "Divide error",
                "Debug",
                "Non-Maskable Interrupt",
                "Breakpoint",
                "Overflow",
                "BOUND Range Exceeded",
                "Invalid Opcode",
                "Device Not Available",
                "Double Fault",
                "Coprocessor Segment Overrun",
                "Invalid TSS",
                "Segment Not Present",
                "Stack Fault",
                "General Protection",
                "Page Fault",
                "(unknown trap)",
                "x87 FPU Floating-Point Error",
                "Alignment Check",
                "Machine-Check",
                "SIMD Floating-Point Exception"
        };

        if (trapno < sizeof(excnames)/sizeof(excnames[0]))
                return excnames[trapno];
        if (trapno == T_SYSCALL)
                return "System call";
        if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16)
                return "Hardware Interrupt";
        return "(unknown trap)";
}

#define SUNUSDBG 1
        void
idt_init(void)
{
        extern struct Segdesc gdt[];
        extern void divide_entry();
        extern void debug_entry();
        extern void nmi_entry();
        extern void brkpt_entry();
        extern void oflow_entry();
        extern void bound_entry();
        extern void illop_entry();
        extern void device_entry();
        extern void dblflt_entry();

        extern void tts_entry();
        extern void segnp_entry();
        extern void stack_entry();
        extern void gpflt_entry();
        extern void pgflt_entry();

        extern void fperr_entry();
        extern void align_entry();
        extern void mchk_entry();
        extern void simderr_entry();

        extern void syscall_entry();
        extern void irq_timer_entry();
        extern void irq_kbd_entry();
        extern void irq_serial_entry();
        extern void irq_spurious_entry();
        extern void irq_error_entry();

        // LAB 3: Your code here.

        SETGATE(idt[T_DIVIDE], 0, GD_KT, divide_entry, 0);
        SETGATE(idt[T_DEBUG], 0, GD_KT, debug_entry, 0);
        SETGATE(idt[T_NMI], 0, GD_KT, nmi_entry, 0);
        SETGATE(idt[T_BRKPT], 0, GD_KT, brkpt_entry, 3);
        SETGATE(idt[T_OFLOW], 0, GD_KT, oflow_entry, 3);
        SETGATE(idt[T_BOUND], 0, GD_KT, bound_entry, 3);
        SETGATE(idt[T_ILLOP], 0, GD_KT, illop_entry, 3);
        SETGATE(idt[T_DEVICE], 0, GD_KT, device_entry, 3);
        SETGATE(idt[T_DBLFLT], 0, GD_KT, dblflt_entry, 3);
        SETGATE(idt[T_TSS], 0, GD_KT, tts_entry, 0);
        SETGATE(idt[T_STACK], 0, GD_KT, stack_entry, 0);
        SETGATE(idt[T_GPFLT], 0, GD_KT, gpflt_entry, 0);
        SETGATE(idt[T_PGFLT], 0, GD_KT, pgflt_entry, 3);
        SETGATE(idt[T_FPERR], 0, GD_KT, fperr_entry, 0);
        SETGATE(idt[T_ALIGN], 0, GD_KT, align_entry, 0);
        SETGATE(idt[T_MCHK], 0, GD_KT, mchk_entry, 0);
        SETGATE(idt[T_SIMDERR], 0, GD_KT, simderr_entry, 0);
        SETGATE(idt[T_SYSCALL],0, GD_KT, syscall_entry, 3);
        /* lab4 irqs */
        SETGATE(idt[IRQ_OFFSET + IRQ_TIMER], 0, GD_KT, irq_timer_entry, 0);
        /* lab4 irqs */

        // Setup a TSS so that we get the right stack
        // when we trap to the kernel.
        ts.ts_esp0 = KSTACKTOP;
        ts.ts_ss0 = GD_KD;

        // Initialize the TSS field of the gdt.
        gdt[GD_TSS >> 3] = SEG16(STS_T32A, (uint32_t) (&ts),
                        sizeof(struct Taskstate), 0);
        gdt[GD_TSS >> 3].sd_s = 0;

        // Load the TSS
        ltr(GD_TSS);

        // Load the IDT
        asm volatile("lidt idt_pd");
}

        void
print_trapframe(struct Trapframe *tf)
{
        cprintf("TRAP frame at %p\n", tf);
        print_regs(&tf->tf_regs);
        cprintf("  es   0x----%04x\n", tf->tf_es);
        cprintf("  ds   0x----%04x\n", tf->tf_ds);
        cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
        cprintf("  err  0x%08x\n", tf->tf_err);
        cprintf("  eip  0x%08x\n", tf->tf_eip);
        cprintf("  cs   0x----%04x\n", tf->tf_cs);
        cprintf("  flag 0x%08x\n", tf->tf_eflags);
        cprintf("  esp  0x%08x\n", tf->tf_esp);
        cprintf("  ss   0x----%04x\n", tf->tf_ss);
}

        void
print_regs(struct PushRegs *regs)
{
        cprintf("  edi  0x%08x\n", regs->reg_edi);
        cprintf("  esi  0x%08x\n", regs->reg_esi);
        cprintf("  ebp  0x%08x\n", regs->reg_ebp);
        cprintf("  oesp 0x%08x\n", regs->reg_oesp);
        cprintf("  ebx  0x%08x\n", regs->reg_ebx);
        cprintf("  edx  0x%08x\n", regs->reg_edx);
        cprintf("  ecx  0x%08x\n", regs->reg_ecx);
        cprintf("  eax  0x%08x\n", regs->reg_eax);
}

        static void
trap_dispatch(struct Trapframe *tf)
{
        // Handle processor exceptions.
        // LAB 3: Your code here.
        // dec 15,2010 sunus
        switch(tf->tf_trapno)
        {
                case T_PGFLT : 
                        {
                                page_fault_handler(tf);
                                break;
                        }
                case T_DEBUG :
                        cprintf("encounter a breakpoint!\n"); /*fall through*/
                case T_BRKPT :
                        {
                                monitor(tf);
                                break;
                        }
                case T_SYSCALL :
                        {
                                (tf->tf_regs).reg_eax = 
                                        syscall(
                                                        (tf->tf_regs).reg_eax,
                                                        (tf->tf_regs).reg_edx,
                                                        (tf->tf_regs).reg_ecx,
                                                        (tf->tf_regs).reg_ebx,
                                                        (tf->tf_regs).reg_edi,
                                                        (tf->tf_regs).reg_esi);
                                return ;
                        }
                default : ; /* do nothing */
        }

        // Handle clock interrupts.
        // LAB 4: Your code here.

        // JAN 30,2011,SUNUS
        if(tf->tf_trapno == IRQ_OFFSET + IRQ_TIMER)
        {
                sched_yield();
                return;
        }
        // Handle spurious interrupts
        // The hardware sometimes raises these because of noise on the
        // IRQ line or other reasons. We don't care.
        if (tf->tf_trapno == IRQ_OFFSET + IRQ_SPURIOUS) {
                cprintf("Spurious interrupt on irq 7\n");
                print_trapframe(tf);
                return;
        }


        // Unexpected trap: The user process or the kernel has a bug.
        print_trapframe(tf);
        if (tf->tf_cs == GD_KT)
                panic("unhandled trap in kernel");
        else {
                env_destroy(curenv);
                return;
        }
}

        void
trap(struct Trapframe *tf)
{
        // The environment may have set DF and some versions
        // of GCC rely on DF being clear
        asm volatile("cld" ::: "cc");

        // Check that interrupts are disabled.  If this assertion
        // fails, DO NOT be tempted to fix it by inserting a "cli" in
        // the interrupt path.

        /* temp fix */
        /*
         if((read_eflags() & FL_IF))
         {
        //       cprintf("NOW ESP = %08x\n",(uint32_t)(read_eflags()));
                 asm volatile("cli");
        //       cprintf("AFTER ESP = %08x\n",(uint32_t)(read_eflags()));
         }
        */
        assert(!(read_eflags() & FL_IF));

        if ((tf->tf_cs & 3) == 3) {
                // Trapped from user mode.
                // Copy trap frame (which is currently on the stack)
                // into 'curenv->env_tf', so that running the environment
                // will restart at the trap point.
                assert(curenv);
                curenv->env_tf = *tf;
                // The trapframe on the stack should be ignored from here on.
                tf = &curenv->env_tf;
        }

        // Dispatch based on what type of trap occurred
        trap_dispatch(tf);

        // If we made it to this point, then no other environment was
        // scheduled, so we should return to the current environment
        // if doing so makes sense.
        if (curenv && curenv->env_status == ENV_RUNNABLE)
                env_run(curenv);
        else
                sched_yield();
}
        void
page_fault_handler(struct Trapframe *tf)
{
        uint32_t fault_va;

        // Read processor's CR2 register to find the faulting address
        fault_va = rcr2();

        // Handle kernel-mode page faults.

        // LAB 3: Your code here.
        //cprintf("SUNUS : tf eip = %08x\n",tf->tf_eip);
        //sunus_dbg(0,3,page_fault_handler);
        if((tf->tf_cs & 0x3) != 0x3)
                panic("page_fault_handler @ %08x\n",fault_va);
        // We've already handled kernel-mode exceptions, so if we get here,
        // the page fault happened in user mode.

        // Call the environment's page fault upcall, if one exists.  Set up a
        // page fault stack frame on the user exception stack (below
        // UXSTACKTOP), then branch to curenv->env_pgfault_upcall.
        //
        // The page fault upcall might cause another page fault, in which case
        // we branch to the page fault upcall recursively, pushing another
        // page fault stack frame on top of the user exception stack.
        //
        // The trap handler needs one word of scratch space at the top of the
        // trap-time stack in order to return.  In the non-recursive case, we
        // don't have to worry about this because the top of the regular user
        // stack is free.  In the recursive case, this means we have to leave
        // an extra word between the current top of the exception stack and
        // the new stack frame because the exception stack _is_ the trap-time
        // stack.
        //
        // If there's no page fault upcall, the environment didn't allocate a
        // page for its exception stack or can't write to it, or the exception
        // stack overflows, then destroy the environment that caused the fault.
        // Note that the grade script assumes you will first check for the page
        // fault upcall and print the "user fault va" message below if there is
        // none.  The remaining three checks can be combined into a single test.
        //
        // Hints:
        //   user_mem_assert() and env_run() are useful here.
        //   To change what the user environment runs, modify 'curenv->env_tf'
        //   (the 'tf' variable points at 'curenv->env_tf').

        // LAB 4: Your code here.
        // JAN,10,SUNUS
        if(curenv->env_pgfault_upcall)
        {
                struct UTrapframe utf;
                utf.utf_fault_va =fault_va;
                utf.utf_err = tf->tf_err;
                utf.utf_regs = tf->tf_regs;
                utf.utf_eip = tf->tf_eip;
                utf.utf_eip = tf->tf_eip;
                utf.utf_eflags = tf->tf_eflags;
                utf.utf_esp = tf->tf_esp;
                if(tf->tf_esp >= (UXSTACKTOP - PGSIZE) && tf->tf_esp <= UXSTACKTOP) /* check if it's in another PGFLT */
                {
                        //LINE 277-279
                        tf->tf_esp -= 4;
                        user_mem_assert(curenv, (const void *)(tf->tf_esp - sizeof(utf) - 4), sizeof(utf), PTE_W|PTE_U);// -4 is the extra 32bit
                }
                else
                {
                        tf->tf_esp = UXSTACKTOP;
                        user_mem_assert(curenv, (const void *)(tf->tf_esp - sizeof(utf)), sizeof(utf), PTE_W|PTE_U);
                }
                tf->tf_esp -= sizeof(utf);
                if(tf->tf_esp < (UXSTACKTOP - PGSIZE))
                {
                        cprintf("tf->tf_esp < UXSTACKTOP - PGSIZE!\n");
                        env_destroy(curenv);
                        return;
                }
                memmove((void *)(tf->tf_esp), &utf, sizeof(struct UTrapframe));
                tf->tf_eip = (uintptr_t)curenv->env_pgfault_upcall;
                env_run(curenv);
        }
        // Destroy the environment that caused the fault.
        cprintf("[%08x] user fault va %08x ip %08x\n",curenv->env_id, fault_va, tf->tf_eip);
        print_trapframe(tf);
        env_destroy(curenv);
}


