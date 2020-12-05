#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "devices/block.h"
#include "filesys/off_t.h"
#include "threads/synch.h"

struct page 
    {
        struct thread* thread;
        struct hash_elem elem; 

        struct frame* frame;
        void* upage;
        
        block_sector_t sector;
        
        struct file* file;
        bool writable;
        uint32_t read_bytes;
        uint32_t zero_bytes;
        off_t ofs;
    };

bool page_set_with_file(void* upage, struct file* file, off_t ofs, uint32_t read_bytes,  uint32_t zero_bytes, bool writable);
bool page_set_with_zero(void *upage);
bool page_load(void *upage);
void page_exit(void);
void page_destory(struct hash_elem *e, void *aux);
struct page* page_find_by_upage(void* upage);
    
hash_hash_func page_hash_func;
hash_less_func page_less_func;

#endif