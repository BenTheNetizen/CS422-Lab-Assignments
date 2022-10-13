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

// void copy_page_tables(unsigned int parent_index, unsigned int child_index){
//     for (pde_index = VM_USERLO_PDE; pde_index < VM_USERHI_PDE; pde_index++) {
//         unsigned int vaddr = pde_index*PDIRSIZE;
//         alloc_ptbl(child_index, vaddr);

//         for (pte_index = 0; pte_index < 1024; pte_index++) {
//             unsigned int pte = get_ptbl_entry(parent_index, pde_index, pte_index);
//             unsigned int pte_addr = PTE_ADDR(pte);
//             set_ptbl_entry(parent_index, pde_index, pte_index, pte_addr, PT_PERM_FORK);
//             set_ptbl_entry(child_index, pde_index, pte_index, pte_addr, PT_PERM_FORK);
//         }
//     }
// }

// void copy_on_write(unsigned int pid, unsigned int vaddr){
//     unsigned int from_pte = get_ptbl_entry_by_va(pid, vaddr);
//     unsigned int from_addr = PTE_ADDR(from_pte);
//     alloc_page(pid, vaddr, PTE_P | PTE_U | PTE_W);
//     unsigned int to_pte = get_pdir_entry_by_va(pid, vaddr);
//     unsigned int to_addr = PTE_ADDR(to_pte);

//     memcpy(to_addr, from_addr, PAGESIZE);
// }

unsigned int copy_pdir_and_ptbl(unsigned int from_proc, unsigned int to_proc)
{
    unsigned int vaddr, pte, page_index, parent_page_index, parent_pde;
    for (unsigned int pde_index = VM_USERLO_PDE; pde_index < VM_USERHI_PDE; pde_index++) {
        vaddr = pde_index << 22; // we want increment the page directory index by 1 (since it is encoded in the vaddr)
        parent_pde = get_pdir_entry_by_va(from_proc, vaddr);
        // if parent_pde != 0, then the parent has a page table at this index, so child needs one too
        if (parent_pde != 0 && (parent_pde & PTE_P) != 0) {
            dprintf("copy_dir_and_ptbl: running alloc_ptbl for proc %d at vaddr %x\n", to_proc, vaddr);
            if (!alloc_ptbl(to_proc, vaddr)) {
                dprintf("copy_pdir_and_ptbl: alloc_ptbl failed	\n");
                return 0;
            }
        }
        // go thru each page table entry and have the child match the parent's
        for (unsigned int pte_index = 0; pte_index < 1024; pte_index++) {
            // gets the ptbl_entry of the parent process
            pte = get_ptbl_entry(from_proc, pde_index, pte_index);
            page_index = ADDR_MASK(pte) / PAGESIZE;
            // we want parent and child's pte to point to the same page
            set_ptbl_entry(from_proc, pde_index, pte_index, page_index, PT_PERM_FORK); 
            set_ptbl_entry(to_proc, pde_index, pte_index, page_index, PT_PERM_FORK); // correct
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
    unsigned int pte = get_ptbl_entry_by_va(pid, vaddr);
    unsigned int page_index = container_alloc(pid);
    if (page_index == 0) {
        dprintf("copy_on_write: container_alloc failed	\n");
        return 0;
    }
    unsigned int *from_page_addr = (unsigned int *) ADDR_MASK(pte);
    unsigned int *new_page_addr = (unsigned int *) (page_index << 12);
    for (unsigned int i = 0; i < PAGESIZE; i++) {
        new_page_addr[i] = from_page_addr[i];
    }

    // set the pte to point to the new page
    set_ptbl_entry_by_va(pid, vaddr, page_index, PT_PERM_PTU);
    return 1;
}