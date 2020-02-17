#include <pthread.h>
#include <stdio.h>
#include <sched.h>

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *),void *arg) {
	printf("In out pthread create");
	*thread = rand();
	void  **child_stack = (void **) malloc(16384);
	int a = clone(start_routine, child_stack, CLONE_VM|CLONE_FILES, NULL);
	
	return thread;
}
