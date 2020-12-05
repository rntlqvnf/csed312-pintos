#include "vm/page.h"
#include <hash.h>
#include <stdio.h>
#include "vm/frame.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"

bool
page_set_with_file(
    void* upage, struct file* file, off_t ofs, uint32_t read_bytes, 
    uint32_t zero_bytes, bool writable)
{
    if(page_find_by_upage(upage) != NULL)
        return false;

    struct page* new_page = malloc(sizeof(struct page));
    if(new_page != NULL)
    {
        new_page->upage = upage;
        new_page->file = file;
        new_page->ofs = file;
        new_page->read_bytes = read_bytes;
        new_page->zero_bytes = zero_bytes;
        new_page->writable = writable;
        new_page->sector = (block_sector_t) -1;
        new_page->thread = thread_current();
        new_page->frame = NULL;

        hash_insert(thread_current()->pages, &new_page->elem);
        return true;
    }
    else
    {
        return false;
    }
}

bool
page_set_with_zero(void *upage)
{
    if(page_find_by_upage(upage) != NULL)
        return false;

    struct page* new_page = malloc(sizeof(struct page));    
    if(new_page != NULL)
    {
        new_page->upage = upage;
        new_page->file = NULL;
        new_page->ofs = 0;
        new_page->read_bytes = 0;
        new_page->zero_bytes = 0;
        new_page->writable = true;
        new_page->sector = (block_sector_t) -1;
        new_page->thread = thread_current();
        new_page->frame = NULL;

        hash_insert(thread_current()->pages, &new_page->elem);
        return true;
    }
    else
    {
        return false;
    }
}

bool
page_load(void *upage)
{
    struct page* page_to_load = page_find_by_upage(upage);
    if (page_to_load == NULL || page_to_load->frame != NULL)
        return false;
    
    struct frame* new_frame = frame_allocate(page_to_load);
    if(new_frame == NULL)
        return false;
    
    if(page_to_load->sector != (block_sector_t) -1)
    {
        // TODO: swap
    }
    else if(page_to_load->file != NULL)
    {
        file_seek (page_to_load->file, page_to_load->ofs);
        if (file_read (page_to_load->file, new_frame->kpage, page_to_load->read_bytes) != (int) page_to_load->read_bytes)
        {
            return false;
        }
        memset (new_frame->kpage + page_to_load->read_bytes, 0, page_to_load->zero_bytes);
    }
    else
    {
      memset (page_to_load->frame->kpage, 0, PGSIZE);
    }

    //TODO: SET PAGEDIR
}

void
page_exit(void)
{
    struct hash* h = thread_current()->pages;
    if(h != NULL)
        hash_destroy(h, page_destory);
}

void
page_destory (struct hash_elem *e, void *aux UNUSED)
{
    struct page* p = hash_entry(e, struct page, elem);
    if(p->frame)
        frame_remove(p->frame);
    free(p);
}

struct page*
page_find_by_upage(void* upage)
{
    struct page page_to_find;
    struct hash_elem *e;

    page_to_find.upage = upage;
    e = hash_find (&thread_current()->pages, &page_to_find.elem);
    if(e != NULL)
        return hash_entry(e, struct page, elem);
    else
        return NULL;
}

unsigned
page_hash_func (const struct hash_elem *e, void *aux UNUSED)
{
  const struct page *p = hash_entry (e, struct page, elem);
  return hash_bytes (&p->upage, sizeof p->upage);
}

bool
page_less_func(const struct hash_elem *e1, const struct hash_elem *e2,void *aux UNUSED)
{
  const struct page *p1 = hash_entry (e1, struct page, elem);
  const struct page *p2 = hash_entry (e2, struct page, elem);

  return p1->upage < p2->upage;
}