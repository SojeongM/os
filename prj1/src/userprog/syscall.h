#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/user/syscall.h"

void syscall_init (void);
void checking_ad(void* t);
void halt(void);
void exit(int i);
int read(int f, void *buffer, unsigned s);
int write(int f, const void *buf, unsigned s);

int fibonacci(int i);
int max_of_four_int(int n1, int n2, int n3, int n4);

#endif /* userprog/syscall.h */
