#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
void check_addr (const void *vaddr);
void get_args(struct intr_frame *f, int *args, int num_of_args);

struct lock sys_lock;

void
syscall_init (void)
{

    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
    /* We will introduce locks to avoid race conditions */
    lock_init(&sys_lock);
}


/* syscall_handler() handles the system calls, if invoked by a user program */
/* In x86-32, eax will hold the result of the system calls. */
static void
syscall_handler (struct intr_frame *f UNUSED)
{
    /* The syscall arguments might be an invalid address. If that is the case, exit immediately */
    check_addr((const void *) f->esp);

    switch (*(int*) f->esp){
        case SYS_HALT: {
            halt();
            break;
        }
        case SYS_EXIT: {
            get_args(f, &arg[0], 2);
            exit(arg[0]);
            break;
        }

        /* create */
        case SYS_CREATE: {
            get_args(f, &args[0], 2);
            f-> eax = create((const char *) args[0], (unsigned) args[1]);
            break;
        }
        /* remove */
        case SYS_REMOVE: {
            get_args(f, &args[0], 1);
            f->eax = remove((const char *) arg[0]);
            break;
        }

    }

    printf ("system call!\n");
    thread_exit ();
}

void halt (void){
    shutdown_power_off();
}

/* Used in SYS_CREATE */

bool create (const char *file, unsigned initial_size)
{
    lock_acquire(&sys_lock);
    bool result = filesys_create(file, initial_size);
    lock_release(&sys_lock);
    return result;
}

/* Used in SYS_REMOVE */
bool remove (const char *file)
{
    lock_acquire(&sys_lock);
    bool result = filesys_remove(file);
    lock_release(&sys_lock);
    return result;
}

/* Function to receive the arguments and use for the system calls */
void get_args(struct intr_frame *f, int *args, int num_of_args){
    int i, *ptr;
    for (i = 0; i <  num_of_args; i++){
        check_addr((const void *) ptr);
        ptr++;
    }
}



void check_addr (const void *vaddr)
{
    /* In case of invalid user address pace or NULL value, the program will be
     * terminated */
    /*To-DO : The address should not go beyond the virtual address space */
    if(!is_user_vaddr(vaddr) || vaddr == NULL)
    {
        /* Terminate the program and free its resources */
        exit_proc(-1);
        return 0;
    }
}