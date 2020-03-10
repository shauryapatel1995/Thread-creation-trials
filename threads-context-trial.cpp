//Implementation of user-level threads in C++
#include <iostream>
#include <queue>
#include <setjmp.h>
#include <thread>
#include <condition_variable>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <ucontext.h>
#include <unordered_map>
#include <sched.h>

typedef std::chrono::high_resolution_clock Clock;
using namespace std;

/*
We need a scheduler and a main. 
We need a thread control block. 
We need a queue of processes.
Need to provide basic functions for thread execution. - Create, Exit and Pre-empt
*/

int thread_counter = 0;


std::unordered_map<std::thread::id, double> thread_times;

size_t sz = 0x10000;
struct TCB {

    int thread_id;
    //Pointer to function
    void *(*func) (void*); 
    
    // Args to a function
	void *arg; 

    //We also need a stackframe
    int* stack_bottom;

    //Saving the context to return to.
	ucontext_t saved_context;

    int context_changed = 0;
};

typedef struct TCB Thread;

// Queue of threads
std::queue<Thread*> threads;

//Context for main and scheduler. 
ucontext_t sched, main_context; 

Thread* current_thread;

int scheduler_activated = 0;

condition_variable new_thread;
std::mutex m;
std::unique_lock<std::mutex> lk(m);
std::mutex using_queue;
bool ready = false;
stack_t stack;

volatile int initscheduler = 0;


void scheduler();

void THREAD_YIELD(int signum) { 
    
    //This will change the context back to scheduler now. 
    if(signum == SIGALRM) {
        
    //cout << "Handling the thing!\n";
    //cout << "Current thread: " << current_thread->thread_id << "\n" ;
    swapcontext(&(current_thread->saved_context),&sched);
    } else {
        //cout << "Oh Fuck!\n";
    }
    
    
} 

void init_scheduler() {
    //cout << "Initializing scheduler\n";

    // Create scheduler context
    ucontext_t uc;
    getcontext(&sched);
    sched.uc_stack.ss_sp = malloc(1024*64);
    sched.uc_stack.ss_size = sz;
    sched.uc_stack.ss_flags = 0;
    //printf("Got context\n");
    sched.uc_link = &main_context;

    
    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    // CPU_SET(2, &mask);
    int result = sched_setaffinity(0, sizeof(mask), &mask);
    
    //Create signal handler 
    struct sigaction sa;
    // Create the new stack
    stack.ss_flags = 0;
    stack.ss_size = 1024*64;
    stack.ss_sp = malloc( 1024*512 );
    //cout << "Got stack!\n";

    //Set up separate stack for signal handling
    sigaltstack( &stack, 0 );

    sa.sa_handler = &THREAD_YIELD;
    sa.sa_flags = SA_ONSTACK;
    sigemptyset( &sa.sa_mask );
    sigaction( SIGALRM, &sa, 0 );
    
    // struct itimerval timer; 
    // timer.it_value.tv_sec = 0;
    // timer.it_value.tv_usec = 50000;
    // timer.it_interval = timer.it_value;
    
    // setitimer(ITIMER_REAL, &timer, NULL);
    
    initscheduler = 1;
    //cout << "INITIALIZAION COMPLETE!\n";
}

thread_exit() {
    // Remove thread from the queue. 
    
    //cout << "\nRemoving from queue!\n";
    //threads.pop();
    free(current_thread);
    current_thread = nullptr;
    //cout << "Queue size is: " << threads.size() << "\n";
    
    return;
}

void scheduler() {

    //Need to setup the current process
    
    current_thread = threads.front();
    //cout << "Current thread set: "<< current_thread->thread_id << "\n";
    
    
    //Initialize scheduler 
    if(!initscheduler) {
        init_scheduler();    
    }  
    

    //Loop through scheduler
    while(1) {
        //cout << "Well schedule it!\n";
        
        threads.pop();
        using_queue.lock();
        if(current_thread != nullptr)
            threads.push(current_thread);
        using_queue.unlock();
        current_thread = threads.front();
        
        
        
        
        //cout << current_thread->thread_id;
        //cout << "Current thread being scheduled is: " << current_thread->thread_id << "\n";
        //cout << "Queue size is: " << threads.size() << "\n";
        current_thread->saved_context.uc_link = &sched;
        //cout << "Swapping context!";
        // Setup timer for preemption
        struct itimerval timer; 
        timer.it_value.tv_sec = 0;
        timer.it_value.tv_usec = 50000;
    //    timer.it_interval = timer.it_value;
        
        setitimer(ITIMER_REAL, &timer, NULL);
        
        swapcontext(&sched, &(current_thread->saved_context));   
        
         if(threads.size() == 0){ 
             //cout << "Threads pending are: " << threads.size() << "\n";
             return;
         }
            
    }
} 

int thread_create(void *(*start_routine) (void *), void* arg) {

    
        
	Thread *thread = new Thread();
    thread->thread_id = thread_counter++;
    thread->func = start_routine;
    thread->arg = arg;
    
    
    getcontext(&(thread->saved_context));

    thread->saved_context.uc_stack.ss_sp = malloc(1024*64);
    thread->saved_context.uc_stack.ss_size = sz;
    thread->saved_context.uc_stack.ss_flags = 0;

    
	thread->saved_context.uc_link = &sched;
    //cout << "What are we passing!: " << arg << "\n";
    makecontext(&(thread->saved_context), start_routine,1, arg);
    
    
    using_queue.lock();
    //Push to thread queue. 
    threads.push(thread);
    //cout << "Queue size is: " << threads.size() << "\n";
    using_queue.unlock();
    
    // Start scheduler on a new thread
    if(threads.size() == 1 && scheduler_activated == 0) {
        
        //cout << "Starting scheduler\n";
        
        std::thread thread1(scheduler);
        scheduler_activated = 1;
        thread1.detach();
    
    } else {
        
        //Notify scheduler thread about thread
        //cout << "Is this blocking?\n";
        //new_thread.notify_all();
    }
        
}

void foo() {
	//cout << "Thread started executing : " << std::this_thread::get_id() << "\n";
	auto t1 = Clock::now();
	//cout << "Value of t1 is: " << t1 << "\n";
	int i;
	int out = 0;
	//Just run a loop 1000 times 
    int a = 0;
	
	for(i = 0; i < 1000; i++){
		a += i;
	}
	printf("a is %d\n",a);
	
	auto t2 = Clock::now();
	//cout << "Thread finished executing : " << std::this_thread::get_id() << " " << std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count() << "\n";
	//thread_times.insert(std::pair<std::thread::id, double>(std::this_thread::get_id(),std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()));
    cout << std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count() << "\n";
    thread_exit();
}


int main() {
    cout << "Hello, Threads!\n";
    

    getcontext(&main_context); 
	main_context.uc_stack.ss_sp = malloc(1024);
	main_context.uc_stack.ss_size = sz;
	main_context.uc_stack.ss_flags = 0;
	//printf("Got context\n");
	main_context.uc_link = &sched;

   
    auto t1 = Clock::now();
	
	cout << "Starting thread\n";

    for(int i =0; i < 100; i++)
        thread_create(foo, nullptr);
	
	// //cout << "ID of first is " << first.get_id() << "\n";
	// thread_create(foo, nullptr);
	// //cout << "ID of second is " << second.get_id() << "\n";
	// thread_create(foo, nullptr);
	// //cout << "ID of third is " << third.get_id() << "\n";
	// thread_create(foo, nullptr);
	// //cout << "ID of fourth is " << fourth.get_id() << "\n";
	// thread_create(foo, nullptr);
	// //cout << "ID of fifth is " << fifth.get_id() << "\n";

	// auto t2 = Clock::now();
	// first.join();
	// second.join();
	// third.join();
	// fourth.join();
	// fifth.join();
	
//	cout << "Duration of the thread execution is: " <<
//	       	std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()	
//		<< "\n";
//	printf("\nFinishing main\n");
	
	// for(auto i : thread_times) {
	// 	cout << i.first << " : " << i.second << "\n";
	// }	
    this_thread::sleep_for(100s);
    cout << "Finished main!\n";
}
