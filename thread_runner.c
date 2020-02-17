#define _GNU_SOURCE 
#include "threads.h"
#include <sched.h>

int variable = 10;
int method1() {
	variable = 42;
	printf("Something");
	return 0;
}

int main() {
	pthread_t thread_id1;


	pthread_create(&thread_id1, NULL, method1, NULL);
	
	sleep(1);

	printf("The variable was %d\n", variable);
	//pthread_join(thread_id1,NULL);
	//thread.sleep(10000);
}
