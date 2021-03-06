#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "vm/page.h"
#include "threads/thread.h"
#include <stdbool.h>

void syscall_init (void);
void halt (void);
void exit (int status);
tid_t exec (const char *cmd_lime);
int wait (tid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
static inline void check_user_sp(const void* sp);
int mmap (int fd, void *upage);
void munmap (int mid);
bool load_from_exec (struct spte *spte);
bool chdir (const char *dir);
bool mkdir (const char *dir);
bool readdir (int fd, const char* name);

// bool readdir (int fd, char name[READDIR_MAX_LEN + 1]);
bool isdir (int fd);
int inumber (int fd);

#endif /* userprog/syscall.h */
