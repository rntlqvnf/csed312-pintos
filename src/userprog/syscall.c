#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);
void validate_addr(const void* vaddr);

void halt();
void exit(int status);
pid_t exec(const char* cmd_line);
int wait(pid_t pid);
bool create(const char* file, unsigned initial_size);
bool remove(const char* file);
int open(const char* file);
int filesize(int fd);
int write(int fd, const void* buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
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
      halt();
      break;
    case SYS_EXIT:
      validate_addr(esp+1);
      exit(*(esp+1));
      break;
    case SYS_EXEC:
      validate_addr(esp+1);
      validate_addr(*(esp+1));
      f->eax = exec(*(esp+1));
      break;
    case SYS_WAIT:
      validate_addr(esp+1);
      f->eax = wait(*(esp+1));
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
      f->eax = write(*(esp+1), *(esp+2), *(esp+3));
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
validate_addr(const void* vaddr)
{
  int i;
  for(i=0; i<4; i++)
  {  
    if(is_kernel_vaddr(vaddr+i))
    {
      exit(-1);
      return;
    }

    if(!pagedir_get_page(thread_current()->pagedir, vaddr+i)) //page fault
    {
      exit(-1);
      return;
    }
  }
}

void
halt()
{
  shutdown_power_off();
  NOT_REACHED();
}

void
exit(int status)
{
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_exit();
  NOT_REACHED();
}

pid_t 
exec(const char* cmd_line)
{
  return process_execute(cmd_line);
}

int 
wait(pid_t pid)
{
  return process_wait(pid);
}

int 
write(int fd, const void* buffer, unsigned size)
{
  if(fd == 1)
  {
    putbuf(buffer, size);
    return size;
  }
  else
  {
    // TODO: File write
    return -1;
  }
}