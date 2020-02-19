#define _GNU_SOURCE 
#include "threads.h"
#include <sched.h>

int variable = 10;
int method1() {
	variable = 42;
	printf("Something\n");
	return 0;
}

int method2() {
	printf("Second method calling\n");
	return 1; 
}

int main() {
	pthread_t thread_id1;
	pthread_t thread_id2; 

	pthread_create(&thread_id1, NULL, method1, NULL);
	pthread_create(&thread_id2, NULL, method2, NULL);
	
	sleep(1);
	
	printf("The variable was %d\n", variable);
	//pthread_join(thread_id1,NULL);
	//thread.sleep(10000);
}
