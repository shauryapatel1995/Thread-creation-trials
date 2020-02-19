#ifndef SOMECODE_H_
#define SOMECODE_H_
#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <ucontext.h>
#include <setjmp.h>
#include <stdlib.h>

#define n 100
int run_threads_activated = 0; 

pthread_mutex_t lock;
int counter = 0;
int run_counter = 0; 
struct Thread {
	void *(*start_routine) (void*); 
	void *arg; 
};

struct Thread threads[n];

int run_threads() {
	// process to run threads, it waits if there are no threads to run. 
	//printf("Executing run threads\n");
	jmp_buf buf;
        if(setjmp(buf) == 0) {
	//	printf("Entered to execute the funtcion\n");
	    	struct Thread thread = threads[run_counter];
		thread.start_routine(thread.arg);
		run_counter++;
		if(run_counter == counter) {
			counter = 0; 
			run_counter = 0; 
			longjmp(buf, 1);
		} else {
			run_counter++;
			printf("Run counter is %d\n", run_counter);
			longjmp(buf, 0);
		}
	       	
	 } else {
	  // 	printf("Set jump returnd\n");
         }

        //printf("Returning from pthread\n");
	return 0;	
}


int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *),void *arg) {


	printf("In our pthread create\n");
	// maintain a queue here to get the total number of executions. 
	
	*thread = 10;	
	//printf("Value of pthread is %d\n", *thread);

	pthread_mutex_lock(&lock);
	struct Thread* new_thread = (struct Thread*)malloc(sizeof(struct Thread)); 
	new_thread->start_routine = start_routine; 
	new_thread->arg = arg;	
	threads[counter] = *new_thread;
	counter++;
	printf("Counter is %d", counter);
	if(!run_threads_activated)
		run_threads_activated = 1;

	if(run_threads_activated) {
	//	printf("Entered run threads\n");
		void  **child_stack = (void **) malloc(16384);
		int a = clone(run_threads, child_stack, CLONE_VM|CLONE_FILES, NULL);
	}
	pthread_mutex_unlock(&lock);
	return 1;
}

#endif
