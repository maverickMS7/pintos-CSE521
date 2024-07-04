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
#include "userprog/process.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "threads/synch.h"


static void syscall_handler (struct intr_frame *);
//void halt (void);
void exit (int status);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
void close (int fd);
int filesize(int fd);
int write (int fd, const void *buffer, unsigned length);
int read (int fd,  void *buffer, unsigned length);
pid_t exec(const char *cmd_line);
//int wait (pid_t pid);
void seek(int fd, unsigned position);
unsigned tell (int fd);
/* Get up to three arguments from a programs stack (they directly follow the system call argument). */
void check_args(struct intr_frame *f, int *args, int num_of_args);
static struct file *find_file_by_file_descriptor(int fd);
int add_file_to_file_descriptor_table(struct file *file);
int remove_file_from_file_descriptor_table (int fd);
//void check_buffer (void *buff_to_check, unsigned size);
void check_addr (const void *vaddr);
const int STDIN = 0;
const int STDOUT = 1;
//int fdTable = 128;
int fdIdx = 2;			   // an index of an open spot in fdTable

/* Lock is in charge of ensuring that only one process can access the file system at one time. */
struct lock sys_lock;

int FDCOUNT_LIMIT = 128; // TODO: review


struct thread_file
{
    struct list_elem file_elem;
    struct file *file_addr;
    int fd;
};

void
syscall_init (void)
{

    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
    /* We will introduce locks to avoid race conditions */
    lock_init(&sys_lock);
}


/* syscall_handler() handles the system calls, if invoked by a user program */
/* In x86-32, eax will hold the result of the system calls. */
/* Followed steps in the following link
 * https://static1.squarespace.com/static/5b18aa0955b02c1de94e4412/t/5b85fad2f950b7b16b7a2ed6/1535507195196/Pintos+Guide
 * */
static void
syscall_handler (struct intr_frame *f)
{
    /* The syscall arguments might be an invalid address. If that is the case, exit immediately */
    check_addr((const void *) f->esp);
    int *args = (int *)f->esp + 1 ;

    switch (*(int*) f->esp){
//
//        case SYS_HALT: {
//            halt();
//            break;
//        }

//        case SYS_WAIT:{
//            check_args(f, &args[0], 1);
//            f->eax = wait((pid_t) args[0]);
//            break;
//        }

        case SYS_EXIT: {
//            check_args(f, &args[0], 2);
            exit (*((int *)f->esp + 1));
            break;
        }

            /* create */
        case SYS_CREATE: {
            check_args(f, &args[0], 2);
            if ((char*) args[0] == NULL) {exit (-1);}
            f->eax = create((const char *) args[0], (unsigned) args[1]);
            break;
        }
            /* remove */
        case SYS_REMOVE: {
            check_args(f, &args[0], 1);
            f->eax = remove((const char *) args[0]);
            break;
        }

        case SYS_OPEN:{
            check_args(f, &args[0], 1);
            if ((char*) args[0] == NULL) {exit (-1);}
            f->eax = open((const char *) args[0]);
            break;
        }

        case SYS_CLOSE:{
            check_args(f, &args[0], 1);
            close(args[0]);
            break;
        }
        case SYS_FILESIZE:{
            check_args(f, &args[0], 1);
            f->eax = filesize(args[0]);
            break;
        }

        case SYS_WRITE: {
            check_args(f, &args[0], 3);
            int *ptr = (int *)f->esp;
            int fd = *(ptr + 1);
            void *buffer = (void *) (*(ptr + 2));
            unsigned size = *((unsigned *)ptr + 3);
//          check_buffer((void *) args[1], args2[2]);
            f->eax = write(fd, buffer, size);
            break;
        }

        case SYS_READ:{
            check_args(f, &args[0], 3);
//            check_buffer ((void*) args[1], args[2]);
            f->eax = read(args[0], (void *) args[1], (unsigned) args[2]);
            break;
        }

        case SYS_EXEC:{
            check_args(f, &args[0], 1);
            f->eax = exec((const char *) args[0]);
            break;
        }

        case SYS_SEEK:{
            check_args(f, &args[0], 2);
            seek(args[0], (unsigned) args[1]);
            break;
        }

        case SYS_TELL:{
            check_args(f, &args[0], 1);
            f->eax = tell(args[0]);
            break;
        }
    }

//    printf ("system call!\n");
//    thread_exit ();
}

//void halt (void){
//    shutdown_power_off();
//}



void exit (int status){
    struct thread *cur = thread_current();
//    cur->exit_status = status;
    printf("%s: exit(%d)\n", cur->name, status);
    thread_exit ();
}

/* Used in SYS_CREATE */
bool create (const char *file, unsigned initial_size){
    lock_acquire(&sys_lock);
    bool result = filesys_create(file, initial_size);
    lock_release(&sys_lock);
    return result;
}

/* Used in SYS_REMOVE */
bool remove (const char *file){
    lock_acquire(&sys_lock);
    bool result = filesys_remove(file);
    lock_release(&sys_lock);
    return result;
}

/* called from SYS_OPEN stem call */
int open (const char *file){
    lock_acquire(&sys_lock);
    struct file *temp_f = filesys_open(file);

    //Opens the file with the given NAME. returns the new file if successful or a null pointer otherwise.
    if (temp_f == NULL){
        lock_release(&sys_lock);
        return -1;
    }
    int fd = add_file_to_file_descriptor_table(temp_f);

    // If File Descriptor Table (FDT) is full
    if (fd == -1){
        file_close(temp_f);
    }
    lock_release(&sys_lock);
    return fd;
}


/*  invoked by SYS_CLOSE system call */
void close (int fd){
    if(fd == 0 || fd == 1){
        return;
    }
    lock_acquire(&sys_lock);
    struct file *temp_f = find_file_by_file_descriptor(fd);;

    if (temp_f == NULL){
        return;
    }

    remove_file_from_file_descriptor_table(fd);
    lock_release(&sys_lock);
    file_close(temp_f);
}


/*  invoked by SYS_FILESIZE system call */
int filesize(int fd){
    int size;
    struct file *temp_f = find_file_by_file_descriptor(fd);
    lock_acquire(&sys_lock);
    if (temp_f == NULL){
        lock_release(&sys_lock);
        return -1;
    }

    size = file_length(temp_f);
    lock_release(&sys_lock);
    return size;
}

/* Definition from Linux man page:   ssize_t write(int fd, const void *buf, size_t count); */
/*  Writes size bytes from buffer to the open file fd. We will check if fd= 1, then we will proceed to write */
int write (int fd, const void *buffer, unsigned length){
    lock_acquire(&sys_lock);
    int write_result;

  struct file *temp_f;
    if (fd != 0 && fd != 1) {
        temp_f = find_file_by_file_descriptor(fd);
        if(temp_f == NULL){
            lock_release(&sys_lock);
            return 0;
        }
    }

//    if (fd == NULL){
//        lock_release(&sys_lock);
//        return 0;
//    }

    if(fd == STDOUT){
//        if(cur->stdout_count == 0){
//            NOT_REACHED();
//            remove_file_from_file_descriptor_table(fd);
//            write_result = -1;
//            lock_release(&sys_lock);
//        } else {
            putbuf(buffer, length);
            write_result = length;
//        }
    } else if (fd == STDIN) {
        write_result = -1;
    } else {
        write_result = file_write (temp_f, buffer, length);
    }
  lock_release (&sys_lock);
  return write_result;
}


/* Reads size bytes from the file open as fd into buffer. Returns the number of bytes actually
read (0 at end of file), or -1 if the file could not be read (due to a condition other than end of
file). Fd 0 reads from the keyboard using input_getc().
 Ref; https://static1.squarespace.com/static/5b18aa0955b02c1de94e4412/t/5b85fad2f950b7b16b7a2ed6/1535507195196/Pintos+Guide
 */

int read (int fd,  void *buffer, unsigned length){
    int read_result;
    lock_acquire(&sys_lock);
    struct file *temp_f;

    if (fd!= 0 && fd != 1){
        temp_f = find_file_by_file_descriptor(fd);
        if (temp_f == NULL){
            lock_release(&sys_lock);
            return -1;
        }
    }

    if(fd == 0){
//        if (cur->stdin_count==0){
//            NOT_REACHED();
//            remove_file_from_file_descriptor_table(fd);
//            read_result = -1;
//        } else {
            unsigned i;
            unsigned char *buf = buffer;
            for (i =0; i < length; i++){
                char c = input_getc();
                *buf++ = c;
                if (c == '\0'){break;}
            }
            lock_release(&sys_lock);
            read_result = i;
//        }
    } else if (fd == 1){
        lock_release(&sys_lock);
        read_result = -1;
    } else {
        read_result = file_read(temp_f, buffer, length);
        lock_release(&sys_lock);
    }
    return read_result;
}

/* Not sure about the exec () implementation */

pid_t exec(const char *cmd_line) {
     /* if cmd_line is NULL, return -1 */
    if (!cmd_line){ return  -1;}
    lock_acquire(&sys_lock);
    pid_t child_tid = process_execute(cmd_line);
    lock_release(&sys_lock);
    return child_tid;
}

//int wait (pid_t pid){
//    return process_wait(pid);
//}

/*
// * Changes the next byte to be read or written in open file fd to position, expressed in bytes from the beginning of the file.
// * (Thus, a position of 0 is the file's start.) A seek past the current end of a file is not an error. A later read obtains 0 bytes,
// * indicating end of file. A later write extends the file, filling any unwritten gap with zeros.
//
// * */
//
/* file_seek() is in filesys/file.c*/
void seek(int fd, unsigned position){
    lock_acquire(&sys_lock);
    unsigned size;
    struct file *temp_f = find_file_by_file_descriptor(fd);

    if (temp_f == NULL){
        lock_release(&sys_lock);
        return;
    }

    size = file_length(temp_f);
    /* Calibrating position */
    if (position > size){
        position = size;
    }
    file_seek(temp_f, position);
    lock_release(&sys_lock);
}

/*Returns the position of the next byte to be read or written in open file fd, expressed in bytes from the beginning
 * of the file.*/
unsigned tell (int fd){
    lock_acquire(&sys_lock);
    struct file *temp_f = find_file_by_file_descriptor(fd);
    if (temp_f == NULL){
        lock_release(&sys_lock);
        return -1;
    }
    /* file_tell() is in filesys/file.c*/
    unsigned offset = file_tell(temp_f);
    lock_release(&sys_lock);
    return offset;
}



/* Function to receive the arguments and use for the system calls */
void check_args(struct intr_frame *f, int *args, int num_of_args){
    int i, *ptr = args;
    for (i = 0; i <  num_of_args; i++){
        check_addr((const void *) ptr);
        ptr++;
    }
}
//
///* SEARCH, APPEND, REMOVE from file descriptor table */
///* It is necessary to check if the given fd is valid */
//
static struct file *find_file_by_file_descriptor(int fd){
    struct thread *cur = thread_current();

    if (fd < 0 || fd >= FDCOUNT_LIMIT)
        return NULL;

    return cur->fdTable[fd];
}

/* Adding file to file descriptor table */

int add_file_to_file_descriptor_table(struct file *file){
    struct thread *cur = thread_current();
    struct file **fdt = cur->fdTable;   /* File descriptor table */

    while ((fdIdx < FDCOUNT_LIMIT) && fdt[fdIdx])
        fdIdx++;

    if (fdIdx >= FDCOUNT_LIMIT)
        return -1;

    /* Updating the file descriptor table  */
    fdt[fdIdx] = file;

    return fdIdx;
}


/* Removing file from the file descriptor table */

int remove_file_from_file_descriptor_table (int fd){
    struct thread *cur = thread_current();

    if (fd < 0 || fd >= FDCOUNT_LIMIT)
        return;

    cur->fdTable[fd] = NULL;
}

//
////int process_add_file(struct file *f){
////    struct thread *cur = thread_current();
////    size_t fe_size = sizeof(struct file_elements);
////    struct file_elements *fe = malloc(fe_size);
////    fe->file = f;
////    fe->fd = cur->fd;
////    thread_current()->fd++;
////    list_push_back(&cur->file_list, &fe->elem);
////    return fe->fd;
////
////}
//
///* We must validate, each memory address in the bufer is in the valid user space */
//void check_buffer (void *buff_to_check, unsigned size){
//    unsigned i;
//    char *ptr = (char*) buff_to_check;
//    for (i = 0; i < size; i++){
//        check_addr((const void *) ptr);
//        ptr++;
//    }
//}
//
//
void check_addr (const void *vaddr){
    /* In case of invalid user address pace or NULL value, the program will be
     * terminated */
    /*To-DO : The address should not go beyond the virtual address space */
    if(!is_user_vaddr(vaddr) || vaddr == NULL)
    {
        /* Terminate the program and free its resources */
//        exit_proc(-1);
        return 0;
    }
}
//
//
///* process control
//halt    exit    wait    fork    exec
//*/
//
///* file management
// * create remove open close
// * read write  filesize
// * */
