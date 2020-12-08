#ifndef VM_SWAP_H
#define VM_SWAP_H

void swap_init(void);
bool swap_in(void *kpage, size_t sector);
size_t swap_out(void *kpage);

#endif