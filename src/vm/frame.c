#include "vm/frame.h"
#include <stdio.h>
#include <list.h>
#include "vm/page.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

static struct list frames;

static struct lock frames_lock;

void
frame_init (void)
{
    lock_init(&frames_lock);
    list_init(&frames);
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
    {
        //TODO: Eviction
    }
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

    lock_release(&frames_lock);
    return new_frame;
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
