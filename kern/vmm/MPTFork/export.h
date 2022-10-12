#ifndef _KERN_VMM_MPTFORK_H_
#define _KERN_VMM_MPTFORK_H_

#ifdef _KERN_

void copy_page_tables(unsigned int parent_index, unsigned int child_index);
void copy_on_write(unsigned int pid, unsigned int vaddr);

#endif  /* _KERN_ */

#endif  /* !_KERN_VMM_MPTFORK_H_ */
