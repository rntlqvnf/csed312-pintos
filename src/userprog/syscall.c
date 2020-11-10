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

void syscall_halt();
void syscall_exit(int status);
pid_t syscall_exec(const char* cmd_line);
int syscall_wait(pid_t pid);
bool syscall_create(const char* file, unsigned initial_size);
bool syscall_remove(const char* file);
int syscall_open(const char* file);
int syscall_filesize(int fd);
int syscall_write(int fd, const void* buffer, unsigned size);
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
      break;
    case SYS_REMOVE:
      break;
    case SYS_OPEN:
      break;
    case SYS_FILESIZE:
      break;
    case SYS_READ:
      break;
    case SYS_WRITE:
      validate_addr(esp+1);
      validate_addr(esp+2);
      validate_addr(esp+3);
      f->eax = syscall_write(*(esp+1), *(esp+2), *(esp+3));
      break;
    case SYS_SEEK:
      break;
    case SYS_TELL:
      break;
    case SYS_CLOSE:
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
  thread_exit();
  NOT_REACHED();
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
  struct thread* t=thread_current();
  lock_acquire(&filesys_lock);
  if(fd == 1)
  {
    putbuf(buffer, size);
    lock_release(&filesys_lock);
    return size;
  }
  else if(fd>1)
  {
    // TODO: File write
    int output=(int) file_write(t->fd_table[fd], buffer, (off_t) size);
    lock_release(&filesys_lock);
    return output;
  }
  else
  {
    lock_release(&filesys_lock);
    return -1;
  }
  
}

void
validate_addr(const void* vaddr)
{
  int i;
  for(i=0; i<4; i++)
  { 
    if(validate_byte(vaddr+i) == false)
      exit(-1);
  }
}

bool validate_byte(const void* byte)
{
  if(is_kernel_vaddr(byte))
    return false;
  if(!pagedir_get_page(thread_current()->pagedir, byte)) //page fault
    return false;

  return true;
}

int syscall_open(const char* file)
{
  struct file* f=filesys_open(file);
  if(f==NULL)
  {
    return -1;
  }
  return process_add_file(f) ;
}

int syscallfilesize(int fd)
{
  if(thread_current()->fd_table[fd]==NULL)
  {
    return -1;
  }
  return (int) file_length(thread_current()->fd_table[fd]);
}

int syscall_read(int fd, const void* buffer, unsigned size)
{
  struct thread* t=thread_current();
  lock_acquire(&filesys_lock);
  int input;
  if(fd==0)
  {
    for(input=0; input<size; input++)
    {
      if(input_getc()==NULL)
      {
        break;
      }
    }

  }
  else if(fd>1)
  {
    input=(int) file_read(t->fd_table[fd], buffer, (off_t) size);
  }
  else
  {
    lock_release(&filesys_lock);
    return -1;
  }
  
  lock_release(&filesys_lock);
  return input;

}

void syscall_seek(int fd, unsigned position)
{
  struct thread* t=thread_current();
  if(t->fd_table[fd]==NULL)
  {
    return;
  }
  else
  {
    file_seek(t->fd_table[fd], (off_t) position);
  }
}

unsigned syscall_tell(int fd)
{
  struct thread* t=thread_current();
  if(t->fd_table[fd]==NULL)
  {
    return;
  }
  else
  {
    return (unsigned) file_tell(t->fd_table[fd]);
  }
}

void syscall_close(int fd)
{
  struct thread* t=thread_current();
  if(t->fd_table[fd]==NULL)
  {
    return;
  }
  else
  {
    file_close(t->fd_table[fd]);
    t->fd_table[fd]=NULL;
  }
}

bool syscall_create(const char* file, unsigned initial_size)
{
  return filesys_create(file, (off_t) initial_size);
}

bool syscall_remove(const char* file)
{
  return filesys_remove(file);
}