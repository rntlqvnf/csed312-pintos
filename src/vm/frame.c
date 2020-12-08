#include "vm/frame.h"
#include <stdio.h>
#include <list.h>
#include <bitmap.h>
#include "vm/page.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"

static struct list frames;

static struct lock frames_lock;

static struct list_elem* frame_clock_points;

static inline bool
is_head(struct list_elem *elem)
{
    return elem != NULL && elem->prev == NULL && elem->next != NULL;
}

static inline bool
is_interior(struct list_elem *elem)
{
    return elem != NULL && elem->prev != NULL && elem->next != NULL;
}

void
frame_init (void)
{
    lock_init(&frames_lock);
    list_init(&frames);
    frame_clock_points = list_head(&frames);
}

/* Allocate frame, and register page with given page
    If no free space, evict and allocate. 
    If failed to allocate,  return NULL
    Otherwise, return allocated frame */
struct frame*
frame_allocate(struct page* page)
{
    lock_acquire(&frames_lock);

    struct frame* new_frame = NULL;
    void* kpage = palloc_get_page(PAL_USER);
    if(kpage == NULL) //No free page, eviction needs
        new_frame = frame_evict_and_reassign(page);
    else
    {
        new_frame = malloc(sizeof(struct frame));

        if(new_frame == NULL) 
        {
            palloc_free_page(kpage);
        }
        else
        {
            new_frame->kpage = kpage;
            new_frame->page = page;
        }
    }

    lock_release(&frames_lock);
    return new_frame;
}

struct frame*
frame_evict_and_reassign(struct page* page)
{
    ASSERT (lock_held_by_current_thread (&frames_lock));

    struct frame* frame = frame_to_evict();
    if(frame == NULL) return NULL;

    if(!frame_evict(frame)) return NULL;
    frame_page_reassign_and_remove_list(frame, page);
}

/* CLock Algorithm */
struct frame*
frame_to_evict(void) 
{
    ASSERT (lock_held_by_current_thread (&frames_lock));

    struct frame* frame = frame_clock_forward();
    while(pagedir_is_accessed (get_pagedir_of_frame(frame), frame->page->upage))
    {
        pagedir_set_accessed (get_pagedir_of_frame(frame), frame->page->upage, false);
        frame = frame_clock_forward();
    }

    return frame;
}

struct frame*
frame_clock_forward(void)
{
    ASSERT (lock_held_by_current_thread (&frames_lock));

    struct list_elem* elem;
    if(is_interior(elem)) elem = list_front(&frames);
    else elem = list_next(frame_clock_points);

    return elem;
}

bool
frame_evict(struct frame* frame)
{
    ASSERT (lock_held_by_current_thread (&frames_lock));

    struct page* page = frame->page;
    bool dirty = pagedir_is_dirty(get_pagedir_of_frame(frame), page->upage);
    
    page->prev_type = page->type;
    switch (page->type)
    {
    case PAGE_ZERO:
        if(!swap_frame(page, frame)) return false;
        break;
    
    case PAGE_MMAP:
        if(dirty)
        {
            //Do write back
        }
        break;
    
    case PAGE_FILE:
        if(page->writable && dirty)
            if(!swap_frame(page, frame)) return false;
        break;

    default:
        NOT_REACHED();
        break;
    }

    page->frame = NULL;
    pagedir_clear_page(get_pagedir_of_frame(frame), page->upage);
    return true;
}

bool
swap_frame(struct page* page, struct frame* frame)
{
    page->swap_index = swap_out(frame->kpage);
    if(page->swap_index == BITMAP_ERROR) 
        return false;
    else 
    {
        page->type = PAGE_SWAP;
        return true;
    }
}

void
frame_page_reassign_and_remove_list(struct frame* frame, struct page* page)
{
    frame->page = page;
    list_remove(&frame->elem);
}

/* Remove frame by kpage */
void
frame_remove_and_free_page(void *kpage)
{
    lock_acquire(&frames_lock);
    struct frame* frame_to_remove = frame_find_by_kpage(kpage);
    if(frame_to_remove != NULL)
    {
        list_remove(&frame_to_remove->elem);
        free(frame_to_remove);
        palloc_free_page(frame_to_remove->kpage);
    }
    lock_release(&frames_lock);
}

/* Remove frame by kpage without palloc_free_page */
void
frame_remove(void *kpage)
{
    lock_acquire(&frames_lock);
    struct frame* frame_to_remove = frame_find_by_kpage(kpage);
    if(frame_to_remove != NULL)
    {
        list_remove(&frame_to_remove->elem);
        free(frame_to_remove);
    }
    lock_release(&frames_lock);
}


/* Find frame in frames by kpage. If not found, return NULL 
    Be sure that before calling this method, please get lock */
struct frame *
frame_find_by_kpage(void *kpage)
{
    ASSERT (lock_held_by_current_thread (&frames_lock));
  
    struct list_elem * elem;
    for (elem = list_begin (&frames); elem != list_end (&frames);
        elem = list_next (elem))
        {
            struct frame *f = list_entry (elem, struct frame, elem);
            if (f->kpage == kpage)
                return f;
        }
    
    return NULL;
}

void
frame_push_back(struct frame* frame)
{
    list_push_back (&frames, &frame->elem);
}

uint32_t *
get_pagedir_of_frame(struct frame* frame)
{
    return frame->page->thread->pagedir;
}