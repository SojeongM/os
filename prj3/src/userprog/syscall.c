#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <stdbool.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/input.h"
#include "lib/user/syscall.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/off_t.h"
#include "threads/synch.h"

//struct lock file_locking;
struct file{
	struct inode *inode;
	off_t pos;
	bool deny_write;
};

static void syscall_handler (struct intr_frame *);
int wait(tid_t pid);
tid_t exec(const char *c);

void
syscall_init (void) 
{
  lock_init(&file_locking);
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
	for(int i=3;i<128;i++){
		if(a->fd[i]!=NULL){
			close(i);
		}
	}
	struct thread *b;
	struct list_elem *e;
	b=NULL;
	e=NULL;
	for(e=list_begin(&thread_current()->child_thr);e!=list_end(&thread_current()->child_thr);e=list_next(e)){
		b=list_entry(e, struct thread, child_thr_elem);
		process_wait(b->tid);
	}
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
	char name[128];
	int k=0;
	while(c[k]!=' '&&c[k]!='\0'){
		name[k]=c[k];
		k++;
	}
	name[k]='\0';
	struct file *ff=filesys_open(name);
	if(ff=NULL){
		return -1;
	}
	tid_t pp=process_execute(c);
	return pp;	
}

int wait(tid_t pid){
	return process_wait(pid);
}

int read(int f, void *buffer, unsigned s){
	checking_ad(buffer);
	int k=-1;
	int m=-1;
	lock_acquire(&file_locking);
	if(f==0){
		for(m=0;m<s;m++){
			if(input_getc()=='\0'){
				break;
			}
		}
		k=m;
	//	lock_release(&file_locking);
	//	return k;
	}
	else if(f>2){
		if(thread_current()->fd[f]==NULL){
			lock_release(&file_locking);
			exit(-1);
		}
		
		k=file_read(thread_current()->fd[f], buffer, s);
			
	}
	lock_release(&file_locking);
	return k;
}

int write(int f, const void* buf, unsigned s){
//	checking_ad(buf);
///	if(f != 1){
//		return -1;
//	}
	int ss=-1;
	lock_acquire(&file_locking);
	if(f==1){

		putbuf(buf, s);
		ss=s;
	}
	else if(f>2){
		if(thread_current()->fd[f]==NULL){
			lock_release(&file_locking);
			exit(-1);
		}
		if(thread_current()->fd[f]->deny_write){
			file_deny_write(thread_current()->fd[f]);
		}
//		lock_release(&file_lock);
		ss= file_write(thread_current()->fd[f], buf, s);
	}
	lock_release(&file_locking);
	return ss;
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

bool create(const char *f, unsigned size){
	if(f==NULL){
		exit(-1);
	}
	checking_ad(f);
	return filesys_create(f, size);
}

bool remove(const char *f){
	if(f==NULL){
		exit(-1);
	}
	checking_ad(f);
	return filesys_remove(f);
}

int open(const char *f){
	if(f==NULL){
		exit(-1);
	}
//	checking_ad(f);
	int s=-1;
	struct file *ff=filesys_open(f);
	lock_acquire(&file_locking);
	if(ff==NULL){
		s=-1;
		lock_release(&file_locking);
		return s;
	}
	else{
		
		for(int m=3;m<128;m++){
			if(thread_current()->fd[m]==NULL){
				if(strcmp(thread_current()->name, f)==0){
					file_deny_write(ff);
				}
			
				thread_current()->fd[m]=ff;
				s=m;
				break;
				
			}	
		}	
	}
	lock_release(&file_locking);
	return s;
}

int filesize(int fd){
	if(thread_current()->fd[fd]==NULL){
		exit(-1);
	}
	int re;
	re=file_length(thread_current()->fd[fd]);
	return re;
}

void seek (int fd, unsigned p){
	if(thread_current()->fd[fd]==NULL){
		exit(-1);
	}
	file_seek(thread_current()->fd[fd], p);
}

unsigned tell(int f){
	if(thread_current()->fd[f]==NULL){
		exit(-1);
	}
	return file_tell(thread_current()->fd[f]);

}

void close(int f){
	if(thread_current()->fd[f]==NULL){
		exit(-1);
	}
	struct file *ff=thread_current()->fd[f];
	thread_current()->fd[f]=NULL;
	return file_close(ff);
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
	case SYS_CREATE:
		checking_ad(f->esp+16);
		checking_ad(f->esp+20);
		f->eax=create((const char*)*(uint32_t*)(f->esp+16),(unsigned)*(uint32_t*)(f->esp+20));
		break;
	case SYS_REMOVE:
		checking_ad(f->esp+4);
		f->eax=remove((const char*)*(uint32_t*)(f->esp+4));
		break;
	case SYS_OPEN:
		checking_ad(f->esp+4);
		f->eax=open((const char*)*(uint32_t*)(f->esp+4));
		break;
	case SYS_FILESIZE:
		checking_ad(f->esp+4);
		f->eax=filesize((int*)*(uint32_t*)(f->esp+4));
		break;
	case SYS_SEEK:
		checking_ad(f->esp+16);
		checking_ad(f->esp+20);
		seek((int*)*(uint32_t*)(f->esp+16), (unsigned)*(uint32_t*)(f->esp+20));
		break;
	case SYS_TELL:
		checking_ad(f->esp+4);
		f->eax=tell((int*)*(uint32_t*)(f->esp+4));
		break;
	case SYS_CLOSE:
		checking_ad(f->esp+4);
		close((int*)*(uint32_t*)(f->esp+4));
		break;
  }
 // printf ("system call!\n");
//  thread_exit ();
}
