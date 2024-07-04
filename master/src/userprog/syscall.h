#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

/* Process identifier. */
typedef int pid_t;
#define PID_ERROR ((pid_t) -1)

void syscall_init (void);

//
//void halt (void) NO_RETURN;
void exit (int);
//bool create (const char *file, unsigned initial_size);
//bool remove (const char *file);
//
//void check_addr (const void *vaddr);

#endif /* userprog/syscall.h */



