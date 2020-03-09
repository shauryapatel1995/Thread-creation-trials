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
using namespace std;

/*
We need a scheduler and a main. 
We need a thread control block. 
We need a queue of processes.
Need to provide basic functions for thread execution. - Create, Exit and Pre-empt
*/

int thread_counter = 0;

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
        
    cout << "Handling the thing!\n";
    cout << "Current thread: " << current_thread->thread_id << "\n" ;
    swapcontext(&(current_thread->saved_context),&sched);
    } else {
        cout << "Oh Fuck!\n";
    }
    
    
} 

void init_scheduler() {
    cout << "Initializing scheduler\n";

    // Create scheduler context
    ucontext_t uc;
    getcontext(&sched);
    sched.uc_stack.ss_sp = malloc(1024*64);
    sched.uc_stack.ss_size = sz;
    sched.uc_stack.ss_flags = 0;
    printf("Got context\n");
    sched.uc_link = &main_context;

    
    //Create signal handler 
    struct sigaction sa;
    // Create the new stack
    stack.ss_flags = 0;
    stack.ss_size = 1024*64;
    stack.ss_sp = malloc( 1024*512 );
    cout << "Got stack!\n";

    //Set up separate stack for signal handling
    sigaltstack( &stack, 0 );

    sa.sa_handler = &THREAD_YIELD;
    sa.sa_flags = SA_ONSTACK;
    sigemptyset( &sa.sa_mask );
    sigaction( SIGALRM, &sa, 0 );
    
    
    initscheduler = 1;
    cout << "INITIALIZAION COMPLETE!\n";
}

thread_exit() {
    // Remove thread from the queue. 
    using_queue.lock();
    cout << "\nRemoving from queue!\n";
    threads.pop();
    cout << "Queue size is: " << threads.size() << "\n";
    using_queue.unlock();
}

void scheduler() {

    //Need to setup the current process
    using_queue.lock();
    current_thread = threads.front();
    cout << "Current thread set: "<< current_thread->thread_id << "\n";
    using_queue.unlock();    
    
    //Initialize scheduler 
    if(!initscheduler) {
        init_scheduler();    
    }  
    

    //Loop through scheduler
    while(threads.size() != 0) {
            
        using_queue.lock();
        threads.pop();
        threads.push(current_thread);
        current_thread = threads.front();
        using_queue.unlock();
        cout << current_thread->thread_id;
        cout << "Current thread being scheduled is: " << current_thread->thread_id << "\n";
        

        cout << "Swapping context!";
        // Setup timer for preemption
        struct itimerval timer; 
        timer.it_value.tv_sec = 0;
        timer.it_value.tv_usec = 50000;
        //timer.it_interval = timer.it_value;
    
        setitimer(ITIMER_REAL, &timer, NULL);
        swapcontext(&sched, &(current_thread->saved_context));   
        thread_exit();
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
    cout << "What are we passing!: " << arg << "\n";
    makecontext(&(thread->saved_context), start_routine,1, arg);
    
    
    using_queue.lock();
    //Push to thread queue. 
    threads.push(thread);
    cout << "Queue size is: " << threads.size() << "\n";
    using_queue.unlock();
    
    // Start scheduler on a new thread
    if(threads.size() == 1 && scheduler_activated == 0) {
        
        cout << "Starting scheduler\n";
        scheduler_activated = 1;
        std::thread thread1(scheduler);
        thread1.detach();
    
    } else {
        
        //Notify scheduler thread about thread
        cout << "Is this blocking?\n";
        new_thread.notify_all();
    }
        
}

void some_function(int counter) {
    cout << "Entered function with counter value: " << counter << "\n";
    
    int i;
    int a = 0;
    while(1) {
        cout << "Trying to run\n";    
        
        for( i=0; i < 100; i++) {
            a += i;
        }

        cout << "Executing function!: " << counter << " Sum value is " << a << "\n";
        if (a % 10 == 0) {
            cout << "Returning!\n";
            return;
        }
            

    }
    
}

int main() {
    cout << "Hello, Threads!\n";
    

    getcontext(&main_context); 
	main_context.uc_stack.ss_sp = malloc(1024);
	main_context.uc_stack.ss_size = sz;
	main_context.uc_stack.ss_flags = 0;
	printf("Got context\n");
	main_context.uc_link = &sched;

   

    //swapcontext(&uc, &sched);
    
    thread_create(some_function, 1);
    
    thread_create(some_function, 2);
    
    thread_create(some_function, 3);
    
    thread_create(some_function, 4);

    std::this_thread::sleep_for(100s);
    

}
