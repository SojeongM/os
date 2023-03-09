#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/input.h"
#include "lib/user/syscall.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "userprog/process.h"
#include "threads/malloc.h"
//#include "filesys/filesys.h"
//#include "filesys/file.h"


static void syscall_handler (struct intr_frame *);
int wait(tid_t pid);
tid_t exec(const char *c);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


void checking_ad(void* t){
	if(!is_user_vaddr(t)){
		exit(-1);
	}
}
void halt(void){
	shutdown_power_off();
}

void exit(int i){
	struct thread *a=thread_current();
//	a->e_sta=i;
	char cmd[256];
	int k;
	k=0;
	while((a->name[k])!=' '&&(a->name[k])!='\0'){
		cmd[k]=(a->name)[k];
		k++;
	}
	cmd[k]='\0';
	printf("%s: exit(%d)\n", cmd, i);
	a->exit_s=i;
	thread_exit();
}

tid_t exec(const char *c){
//	printf("input %s\n", c);
/*	if(c==NULL){
		//printf("in exec\n\n");
		return -1;
	}
	char cmd[256];
	int k=0;
	while(cmd[k]!=' '&&cmd[k]!='\0'){
		cmd[k]=c[k];
		k++;
	}
	cmd[k]='\0';
	//printf("%s cmd\n\n", cmd);
	struct file *f=NULL;
	f=filesys_open(cmd);
	if(f==NULL){
		return -1;
	}
	*/
	tid_t pp=process_execute(c);
	return pp;	
}

int wait(tid_t pid){
	return process_wait(pid);
}

int read(int f, void *buffer, unsigned s){
	int k;
	if(f!=0){
		return -1;
	}
	for(k=0;k<s;k++){
		if(input_getc()=='\0'){
			break;
		}
	}
	return k;
}

int write(int f, const void* buf, unsigned s){
	if(f != 1){
		return -1;
	}
/*	struct thread *cur=thread_current();
	struct file *ff=process_get_file(f);
	if(ff==2){
		
		putbuf(buf, s);
		return s;
	}*/
//	printf("writing\n\n");
//	printf("%d\n", s);
	putbuf(buf, s);
	return s;
}

int fibonacci(int i){
	if(i<=1){
		return i;
	}
	int n1=0, n2=1;
	int j, next;
	for(j=0;j<i-1;j++){
		next=n2;
		n2=n1+n2;
		n1=next;
	}
	return n2;
}

int max_of_four_int(int n1, int n2, int n3, int n4){
	int s[4];
	int i, j, temp;
	s[0]=n1;
	s[1]=n2;
	s[2]=n3;
	s[3]=n4;
	for(i=0;i<3;i++){
		for(j=0;j<3-i;j++){
			if(s[j]>s[j+1]){
				temp=s[j];
				s[j]=s[j+1];
				s[j+1]=temp;
			}	
		}
	}
	return s[3];
}

static void
syscall_handler (struct intr_frame *f) 
{
//  printf("sys num : %d\n\n", *(uint32_t*)(f->esp));
//  int *argv=(int*)(f->esp), i;
  checking_ad(f->esp);
  switch(*(uint32_t *)(f->esp)){
	case SYS_HALT:
		halt();
		break;
	case SYS_EXIT:
//		printf("handler exit\n\n");
		checking_ad(f->esp+4);
		exit(*(uint32_t *)(f->esp+4));
		break;
	case SYS_EXEC:
	//	printf("exec!\n\n");
		checking_ad(f->esp+4);
		f->eax=exec((const char*)*(uint32_t *)(f->esp+4));
		break;

	case SYS_WAIT:	
		checking_ad(f->esp+4);
		f->eax=wait((tid_t)*(uint32_t *)(f->esp+4));
		break;
	case SYS_READ:
		checking_ad(f->esp+20);
		checking_ad(f->esp+24);
		checking_ad(f->esp+28);
		f->eax =read((int)*(uint32_t *)(f->esp+20), (void*)*(uint32_t *)(f->esp+24), (unsigned)*((uint32_t *)(f->esp+28)));
		break;
	case SYS_WRITE:
		f->eax =write((int)*(uint32_t *)(f->esp+20), (void*)*(uint32_t *)(f->esp+24), (unsigned)*((uint32_t *)(f->esp+28)));
		break;
	case SYS_FIBO:
		checking_ad(f->esp+4);
		f->eax=fibonacci((int)*(uint32_t*)(f->esp+4));
		break;
	case SYS_MAX:
		checking_ad(f->esp+28);
		checking_ad(f->esp+32);
		checking_ad(f->esp+36);
		checking_ad(f->esp+40);
		f->eax =max_of_four_int((int)*(uint32_t *)(f->esp+28), (int)*(uint32_t *)(f->esp+32), (int)*(uint32_t *)(f->esp+36), (int)*(uint32_t*)(f->esp+40));
		break;
  }
 // printf ("system call!\n");
//  thread_exit ();
}
