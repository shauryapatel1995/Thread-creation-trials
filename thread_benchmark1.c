#include<stdio.h>
#include "threads.h"

void some_function() {
	int a = 0; 
	int out = 0; 
	for(a = 0;a < 10; a++) {
		out += a;
	}	

	printf("%d\n",out);
}

int main() {

	pthread_t threadid1, threadid2, threadid3; 

	pthread_create(&threadid1,NULL, some_function, NULL); 
	pthread_create(&threadid2, NULL, some_function, NULL);
	pthread_create(&threadid3, NULL, some_function, NULL);

}
