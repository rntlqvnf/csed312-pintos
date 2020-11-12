#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "process.h"

#include "../filesys/file.h"
#include "../filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
void validate_addr(const void* vaddr);
bool validate_byte(const void* byte);
void close_files();

void syscall_halt();
pid_t syscall_exec(const char* cmd_line);
int syscall_wait(pid_t pid);
bool syscall_create(const char* file, unsigned initial_size);
bool syscall_remove(const char* file);
int syscall_open(const char* file);
int syscall_filesize(int fd);
int syscall_write(int fd, const void* buffer, unsigned size);
int syscall_read(int fd, const void* buffer, unsigned size);
void syscall_seek(int fd, unsigned position);
unsigned syscall_tell(int fd);
void syscall_close(int fd);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int *esp = f->esp;

  validate_addr(esp);
  int syscall_num = *esp;
  switch (syscall_num)
  {
    case SYS_HALT:
      syscall_halt();
      break;
    case SYS_EXIT:
      validate_addr(esp+1);
      syscall_exit(*(esp+1));
      break;
    case SYS_EXEC:
      validate_addr(esp+1);
      validate_addr(*(esp+1));
      f->eax = syscall_exec(*(esp+1));
      break;
    case SYS_WAIT:
      validate_addr(esp+1);
      f->eax = syscall_wait(*(esp+1));
      break;
    case SYS_CREATE:
      validate_addr(esp+1);
      validate_addr(*(esp+1));
      validate_addr(esp+2);
      f->eax = syscall_create(*(esp+1), *(esp+2));
      break;
    case SYS_REMOVE:
      validate_addr(esp+1);
      validate_addr(*(esp+1));
      f->eax = syscall_remove(*(esp+1));
      break;
    case SYS_OPEN:
      validate_addr(esp+1);
      validate_addr(*(esp+1));
      f->eax = syscall_open(*(esp+1));
      break;
    case SYS_FILESIZE:
      validate_addr(esp+1);
      f->eax = syscall_filesize(*(esp+1));
      break;
    case SYS_READ:
      validate_addr(esp+1);
      validate_addr(esp+2);
      validate_addr(*(esp+2));
      validate_addr(esp+3);
      f->eax = syscall_read(*(esp+1), *(esp+2), *(esp+3));
      break;
    case SYS_WRITE:
      validate_addr(esp+1);
      validate_addr(esp+2);
      validate_addr(*(esp+2));
      validate_addr(esp+3);
      f->eax = syscall_write(*(esp+1), *(esp+2), *(esp+3));
      break;
    case SYS_SEEK:
      validate_addr(esp+1);
      validate_addr(esp+2);
      syscall_seek(*(esp+1), *(esp+2));
      break;
    case SYS_TELL:
      validate_addr(esp+1);
      f->eax = syscall_tell(*(esp+1));
      break;
    case SYS_CLOSE:
      validate_addr(esp+1);
      syscall_close(*(esp+1));
      break;
    default:
      printf("Not implemented \n");
      break;
  }
}

void
syscall_halt()
{
  shutdown_power_off();
  NOT_REACHED();
}

void
syscall_exit(int status)
{
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current()->exit_status = status;
  close_files();
  thread_exit();
  NOT_REACHED();
}

void
close_files()
{
  int i;
  for(i = 0; i<130; i++)
    syscall_close(i);
}

pid_t 
syscall_exec(const char* cmd_line)
{
  return process_execute(cmd_line);
}

int 
syscall_wait(pid_t pid)
{
  return process_wait(pid);
}

int 
syscall_write(int fd, const void* buffer, unsigned size)
{
  int result;
  lock_acquire(&filesys_lock);
  
  if(fd == 1)
  {
    putbuf(buffer, size);
    result = size;
  }
  else if(fd > 1)
  {
    if(thread_current()->fd_table[fd] == NULL)
      result = -1;
    else
      result = (int) file_write(thread_current()->fd_table[fd], buffer, (off_t) size);
  }
  else
    result = -1;

  lock_release(&filesys_lock);
  return result;
}

int 
syscall_read(int fd, const void* buffer, unsigned size)
{
  lock_acquire(&filesys_lock);
  int result;
	uint8_t* bf = (uint8_t *) buffer;
  if(fd == 0)
  {
    int i = 0;
    uint8_t b;
    for(i=0; i<size; i++)
    {
      if(b = input_getc() == NULL)
        break;
      else
        *bf++ = b;
    }
    result = i;
  }
  else if(fd > 1)
  {
    if(thread_current()->fd_table[fd] == NULL)
      result = -1;
    else
      result = (int) file_read(thread_current()->fd_table[fd], buffer, (off_t) size);
  }
  else
    result = -1;
  
  lock_release(&filesys_lock);
  return result;
}

int syscall_open(const char* file)
{
  lock_acquire(&filesys_lock);
  struct file* opened_file = filesys_open(file);
  int i;
  
  if(opened_file == NULL)
  {
    lock_release(&filesys_lock);
    return -1;
  }

  for(i = 2; i<130; i++)
  {
    if(thread_current()->fd_table[i] == NULL)
    {
      if(strcmp(thread_name(), file) == 0)
        file_deny_write(opened_file);

      thread_current()->fd_table[i] = opened_file;
      lock_release(&filesys_lock);
      return i;
    }
  }
  lock_release(&filesys_lock);
  return -1;
}

int syscall_filesize(int fd)
{
  if(thread_current()->fd_table[fd] == NULL)
    return -1;

  return (int) file_length(thread_current()->fd_table[fd]);
}

void syscall_seek(int fd, unsigned position)
{
  if(thread_current()->fd_table[fd] == NULL)
    return;
  else
    file_seek(thread_current()->fd_table[fd], (off_t) position);
}

unsigned syscall_tell(int fd)
{
  if(thread_current()->fd_table[fd] == NULL)
    return -1;
  else
    return (unsigned) file_tell(thread_current()->fd_table[fd]);
}

void syscall_close(int fd)
{
  file_close(thread_current()->fd_table[fd]);
  thread_current()->fd_table[fd] = NULL;
}

bool syscall_create(const char* file, unsigned initial_size)
{
  return filesys_create(file, (off_t) initial_size);
}

bool syscall_remove(const char* file)
{
  return filesys_remove(file);
}


void
validate_addr(const void* vaddr)
{
  int i;
  for(i=0; i<4; i++)
  { 
    if(validate_byte(vaddr+i) == false)
      syscall_exit(-1);
  }
}

bool 
validate_byte(const void* byte)
{
  if(is_kernel_vaddr(byte))
    return false;
  if(!pagedir_get_page(thread_current()->pagedir, byte)) //page fault
    return false;

  return true;
}