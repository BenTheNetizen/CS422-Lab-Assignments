#include <lib/elf.h>
#include <lib/debug.h>
#include <lib/gcc.h>
#include <lib/seg.h>
#include <lib/trap.h>
#include <lib/x86.h>

#include "import.h"

extern tf_t uctx_pool[NUM_IDS];
extern char STACK_LOC[NUM_IDS][PAGESIZE];

void proc_start_user(void)
{
    unsigned int cur_pid = get_curid();
    tss_switch(cur_pid);
    set_pdir_base(cur_pid);

    trap_return((void *) &uctx_pool[cur_pid]);
}

unsigned int proc_create(void *elf_addr, unsigned int quota)
{
    unsigned int pid, id;

    id = get_curid();
    pid = thread_spawn((void *) proc_start_user, id, quota);

    if (pid != NUM_IDS) {
        elf_load(elf_addr, pid);

        uctx_pool[pid].es = CPU_GDT_UDATA | 3;
        uctx_pool[pid].ds = CPU_GDT_UDATA | 3;
        uctx_pool[pid].cs = CPU_GDT_UCODE | 3;
        uctx_pool[pid].ss = CPU_GDT_UDATA | 3;
        uctx_pool[pid].esp = VM_USERHI;
        uctx_pool[pid].eflags = FL_IF;
        uctx_pool[pid].eip = elf_entry(elf_addr);
    }

    return pid;
}


// copies the current process, allocating half of quota to child
unsigned int proc_fork() 
{
    /*
        STEPS:
        1. get process id
        2. use process' container to fetch the quota
        3. update child and parent container fields
        4. copy page directory and page table entries for child process

    */
   unsigned int pid, chid, quota, child_quota, usage, parent_quota;

    pid = get_curid();
    parent_quota = container_get_quota(id);
    usage = container_get_usage(id);
    child_quota = parent_quota - usage / 2;

    // thread_spawn handles the container stuff
    // proc_start_user may not be the correct entry point
    chid = thread_spawn((void *) proc_start_user, id, child_quota);

    if (chid != NUM_IDS) {
        // copy page directory and page table entries for child process
        if (!copy_pdir_and_ptbl(pid, chid)) {
            dprintf("proc_fork: copy_pdir_and_ptbl failed");
        }

        uctx_pool[chid].es = uctx_pool[pid].es;
        uctx_pool[chid].ds = uctx_pool[pid].ds;
        uctx_pool[chid].cs = uctx_pool[pid].cs;
        uctx_pool[chid].ss = uctx_pool[pid].ss;
        uctx_pool[chid].esp = uctx_pool[pid].esp;
        uctx_pool[chid].eflags = uctx_pool[pid].eflags;
        uctx_pool[chid].eip = uctx_pool[pid].eip;
        
        // set child's return value to 0 and parent's return to the child's pid
        // is this necessary?
        uctx_pool[chid].eax = 0;
        uctx_pool[pid].eax = chid;
    }

    return chid;
}