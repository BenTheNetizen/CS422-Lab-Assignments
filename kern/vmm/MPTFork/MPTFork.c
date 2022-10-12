#include <lib/gcc.h>
#include <lib/x86.h>
#include <lib/debug.h>

#include "import.h"

#define PAGESIZE      4096
#define PDIRSIZE      (PAGESIZE * 1024)
#define VM_USERLO     0x40000000
#define VM_USERHI     0xF0000000
#define VM_USERLO_PDE (VM_USERLO / PDIRSIZE)
#define VM_USERHI_PDE (VM_USERHI / PDIRSIZE)
#define VM_USERLO_PAGE_INDEX (VM_USERLO / PAGESIZE)
#define VM_USERHI_PAGE_INDEX (VM_USERHI / PAGESIZE)
#define PT_PERM_FORK (PTE_P | PTE_U | PTE_COW)
#define PT_PERM_PTU (PTE_P | PTE_W | PTE_U)
#define ADDR_MASK(x) ((unsigned int) x & 0xfffff000)

// unsigned int *PDirPool[NUM_IDS][1024] gcc_aligned(PAGESIZE);

unsigned int copy_pdir_and_ptbl(unsigned int from_proc, unsigned int to_proc)
{
    unsigned int from_pde;
    unsigned int page_index
    unsigned int from_pte;
    unsigned int vaddr;
    // HOW DO WE DO THIS WITHIN THE BOUNDS OF USER SPACE?
    // copy only what's in user space
    for (unsigned int i = 0; i < 1024; i++) {
        // construct virtual address from pde index
        page_index = container_alloc(to_proc);
        if (page_index == 0) {
            dprintf("copy_pdir_and_ptbl: container_alloc failed	\n");
            return 0;
        }
        set_pdir_entry(to_proc, i, page_index);
        for (unsigned int j = 0; j < 1024; j++) {
            from_pte = get_ptbl_entry(from_proc, i, j); // this is an address with permission bits
            page_index = ADDR_MASK(from_pte) / PAGESIZE;
            set_ptbl_entry(to_proc, i, j, page_index, PT_PERM_FORK);
            set_ptbl_entry(from_proc, i, j, page_index, PT_PERM_FORK);
        }
    }

    return 1;
}

unsigned int copy_on_write(unsigned int pid, unsigned int vaddr) {
    /*
        STEPS:
        1. Create a copy of the page that the vaddr points to (use get_ptbl_entry_by_vaddr)
        2. Create a new page
        3. Copy over the contents of the page to the new page
        4. Set the permission bits of the page to be read/write
    */
    unsigned int pte = get_ptbl_entry_by_vaddr(pid, vaddr);
    unsigned int page_index = container_alloc(pid);
    if (page_index == 0) {
        dprintf("copy_on_write: container_alloc failed	\n");
        return 0;
    }
    unsigned int *from_page_addr = (unsigned int *) GET_PAGE_ADDR(pte);
    unsigned int *new_page_addr = (unsigned int *) (page_index << 12);
    for (unsigned int i = 0; i < PAGESIZE; i++) {
        new_page_addr[i] = from_page_addr[i];
    }

    // set the pte to point to the new page
    set_ptbl_entry_by_vaddr(pid, vaddr, page_index, PT_PERM_PTU);
    return 1;
}