#include <lib/gcc.h>
#include <lib/x86.h>
#include <lib/debug.h>

#include "import.h"


// unsigned int *PDirPool[NUM_IDS][1024] gcc_aligned(PAGESIZE);

void copy_pdir(unsigned int from_proc, unsigned int to_proc)
{
    unsigned int from_pde;
    for (unsigned int i = 0; i < 1024; i++) {
        from_pde = get_pdir_entry(from_proc, i);
        // we need to insert the page directory entry into the new process' page directory
        set_pdir_entry(to_price, i, from_pde);
        // should we copy the page tables of each page directory entry?
    }
}


void copy_ptbl(unsigned int from_proc, unsigned int to_proc, unsigned int pde_index, unsigned int perm)
{
    unsigned int from_pte;
    for (unsigned int i = 0; i < 1024; i++) {
        // we need to copy over all the pages of the table into the new process' page table at page directory entry pde_index
        from_pte = get_ptbl_entry(from_proc, pde_index, i);
        if (from_pte != 0 && (from_pte & PTE_U) != 0) {
            // we need to copy the page
            unsigned int from_page_index = from_pte >> 12;
            unsigned int to_page_index = alloc_page();
            copy_page(from_page_index, to_page_index);
            set_ptbl_entry(to_proc, pde_index, i, to_page_index, perm);
        }
    }
}