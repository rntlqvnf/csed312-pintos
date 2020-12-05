#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stdbool.h>
#include "threads/synch.h"
#include "vm/page.h"

struct frame
    {
        void *kpage;
        struct page* page;
        bool is_loaded;
        struct list_elem elem;
    };

void frame_init (void);
struct frame* frame_allocate(struct page* page);
void frame_remove(void *kpage);
void frame_remove_and_free_page(void *kpage)
struct frame * frame_find_by_kpage(void *kpage);

#endif
