
// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800
#define SUNUSDBG 0
//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
        static void
pgfault(struct UTrapframe *utf)
{
        void *addr = (void *) utf->utf_fault_va;
        uint32_t err = utf->utf_err;
        int r;

        // Check that the faulting access was (1) a write, and (2) to a
        // copy-on-write page.  If not, panic.
        // Hint:
        //   Use the read-only page table mappings at vpt
        //   (see <inc/memlayout.h>).

        // LAB 4: Your code here.
        // JAN 19,sunus*
           pte_t pte;
           pte = vpt[VPN(addr)];
           if((err & FEC_WR) && (pte & PTE_COW))
                   ;
           else
                   panic("pgfault arguments error:\n");
        // Allocate a new page, map it at a temporary location (PFTEMP),
        // copy the data from the old page to the new page, then move the new
        // page to the old page's address.
        // Hint:
        //   You should make three system calls.
        //   No need to explicitly delete the old page's mapping.

        // LAB 4: Your code here.
        // JAN 19,sunus
        if((r = sys_page_alloc(0, PFTEMP, PTE_U|PTE_P|PTE_W)) < 0)
                panic("SUNUS : sys_page_alloc error!:\n");
        memmove((void *)PFTEMP, (void *)ROUNDDOWN(addr,PGSIZE), PGSIZE);
        if((r = sys_page_map(0, PFTEMP, 0, (void *)ROUNDDOWN(addr, PGSIZE), PTE_U|PTE_P|PTE_W)) < 0)
                panic("SUNUS : sys_page_map error!:\n");
        if((r = sys_page_unmap(0, PFTEMP)) < 0)  // I don't think it is necessary , just in case
                panic("SUNUS : sys_page_unmap error!:\n");
       
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
// 
        static int
duppage(envid_t envid, unsigned pn)
{
        int r;
        char ch;
        // LAB 4: Your code here.
        pte_t pte;
        void *addr = (void *)(pn * PGSIZE);
        pte = (vpt[pn]);
//        sunus_dbg(SUNUSDBG, 3,duppage);
//        do
//        {
//                cprintf("addr = %08x,pn = %d,envid = %d,pte = %08x\n",(uint32_t)addr,pn,envid,(uint32_t)pte);
//        }while((ch = getchar()) != 'c');
        if(((pte & PTE_W) != 0) || ((pte & PTE_COW) != 0))
        {
                if((r = sys_page_map(0, addr, envid, addr, PTE_P|PTE_U|PTE_COW)) < 0)
                {
                        panic("SUNUS : sys_page_map(0, addr, envid, addr, PTE_P|PTE_U|PTE_COW) error : pn = %d",pn);
                }
                if((r = sys_page_map(0, addr, 0, addr, PTE_P|PTE_U|PTE_COW)) < 0 )
                        panic("SUNUS : sys_page_map(0, addr, 0, addr,PTE_P|PTE_U|PTE_COW) error : pn = %d\n",pn);
        }
        else
        {
                if((r = sys_page_map(0, addr, envid, addr, PTE_P|PTE_U)) < 0)
                        panic("SUNUS : sys_page_map(0, addr, envid, addr, PTE_P|PTE_U) : pn = %d\n",pn);

        }
        sunus_dbg(SUNUSDBG, 4, duppage,"I AM 95");
        return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "env" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
        envid_t
fork(void)
{
        // LAB 4: Your code here.
        // JAN 20,2011 SUNUS
        extern void _pgfault_upcall(void);
        /*
           extern unsigned char  end[];
           extern volatile pte_t vpt[];     // VA of "virtual page table"
           extern volatile pde_t vpd[];     // VA of current page directory
         */
        envid_t newenv;
        int r;
        int i,j,pn;
        char ch;
        pte_t pte;
        set_pgfault_handler(pgfault);
        if((newenv = sys_exofork()) < 0)
                panic("SUNUS : sys_exofork error!:\n");
//        sunus_dbg(SUNUSDBG, 2, sys_exofork);
        if(newenv == 0 )
        {
                env = &envs[ENVX(sys_getenvid())];
                return 0;
        }
        else
        {
                j = 0;
                for(i = 0; i < PDX(UTOP) ; i++)
                {
                        if(((pte_t *)vpd)[i] & PTE_P)
                        {
                                //   cprintf("i = %d,vpd[i] = %08x\n",i,(uint32_t)vpd[i]);
                                for(j = 0 ; j < 1024 ; j++)
                                {
                                        pn = (i << 10) + j;
                                        if( pn == 0xeebff)
                                                continue;
                                        pte = vpt[pn];
                                        if(pte & PTE_P)
                                        {
//                                                do
//                                                {
//                                                cprintf("i = %d,j = %d,vpd[%d] = %08x, pte = %08x\n",i,j,i,(uint32_t)vpd[i],pte);
//                                                }while((ch = getchar()) != 'c');
                                                if((r = duppage(newenv, pn)) < 0)
                                                        cprintf("SUNUS : duppage error! newenv = %d i * 1024 + jjjj = %d\n");
                                        }
                                }
                        }
                }
        }
//        do
//        {
//                cprintf("ROUND END!\n");
//        }while((ch = getchar()) != 'c');
        if((r = sys_page_alloc(newenv, (void *)(UXSTACKTOP - PGSIZE), PTE_P|PTE_U|PTE_W)) < 0)
                panic("SUNUS : sys_page_alloc error!");
        //        if ((r=sys_page_map(newenv, (void*)(UXSTACKTOP-PGSIZE), 0, (void*)UTEMP, PTE_U | PTE_P | PTE_W)) < 0) 
        //        {
        //                panic("sys_page_map fail: %e ", r); 
        //        }         
        //        memmove((void*)UTEMP, (void*)(UXSTACKTOP-PGSIZE), PGSIZE);
        //        sys_page_unmap(0, UTEMP);
        sunus_dbg(SUNUSDBG, 2, sys_page_alloc,"NORMAL");
        if((r = sys_env_set_pgfault_upcall(newenv, _pgfault_upcall) < 0))
        {
                cprintf("SUNUS : sys_env_set_pgfault_upcall error!newenv = %d\n",newenv);
                panic("%e\n",r);
        }
        sunus_dbg(SUNUSDBG, 2, sys_env_set_pgfault_upcall,"NORMAL");
        if((r = sys_env_set_status(newenv, ENV_RUNNABLE)) < 0)
                panic("SUNUS : sys_env_set_status error!\n");
        sunus_dbg(SUNUSDBG, 2, sys_env_set_status,"NORMAL");
        return newenv;
}
// Challenge!
        int
sfork(void)
{
        panic("sfork not implemented");
        return -E_INVAL;
}
