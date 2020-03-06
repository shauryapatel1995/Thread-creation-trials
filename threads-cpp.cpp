//Implementation of user-level threads in C++
#include <iostream>
#include <queue>
#include <setjmp.h>
#include <thread>
#include <condition_variable>
#include <signal.h>
#include <unistd.h>
using namespace std;

/*
We need a scheduler and a main. 
We need a thread control block. 
We need a queue of processes.
Need to provide basic functions for thread execution. - Create, Exit and Pre-empt
*/

int thread_counter = 0;
struct TCB {

    int thread_id;
    //Pointer to function
    void *(*func) (void*); 
    
    // Args to a function
	void *arg; 

    //We also need a stackframe
    int* stack_bottom;

    //Saving the context to return to.
	jmp_buf saved_context;

    int context_changed = 0;
};

typedef struct TCB Thread;

// Queue of threads
std::queue<Thread*> threads;

//Scheduler context
jmp_buf sched;

//Main thread context
jmp_buf main_buf;

Thread* current_thread;

int scheduler_activated = 0;

condition_variable new_thread;
std::mutex m;
std::unique_lock<std::mutex> lk(m);
std::mutex using_queue;
bool ready = false;
stack_t stack;

int initscheduler = 0;



void THREAD_YIELD() { 
    cout << "Handling the thing!\n";
    if (setjmp(current_thread->saved_context) == 0) {
        cout << "Changed context!\n";
        longjmp(sched, 1);
    }
} 

void scheduler() {

    if(!initscheduler) {
        cout << "Initializing scheduler\n";
        struct sigaction sa;
    // Create the new stack
        stack.ss_flags = 0;
        stack.ss_size = 1024*64;
        stack.ss_sp = malloc( 1024*64 );
        cout << "Got stack!";
        sigaltstack( &stack, 0 );

        sa.sa_handler = &THREAD_YIELD;
        sa.sa_flags = SA_ONSTACK;
        sigemptyset( &sa.sa_mask );
        sigaction( SIGALRM, &sa, 0 );
        
        initscheduler = 1;
        ualarm(500, 500);
    }   
        
    //Scheduler just schedules the current top thread
    cout << "Scheduler\n";
    if(setjmp(sched) == 0) {
        using_queue.lock();
        cout << "Scheduling process!\n";
        current_thread = threads.front();
        using_queue.unlock();
        current_thread->func(current_thread->arg);
    } else {
        cout << "Back to scheduler!\n";
        using_queue.lock();
        threads.pop();
        threads.push(current_thread);
        current_thread = threads.front();
        using_queue.unlock();
        
        if(current_thread->context_changed) {
            cout << "Trying to restore previous context!\n";
            longjmp(current_thread->saved_context,1);
        } else {
            current_thread->func(current_thread->arg);
        }   
    }
            
    } 

/*#define thread_yield() {\
    //Store the context for the thread here
    if(setjmp(current_thread->saved_context) == 0) {\
        current_thread->context_changed = 1;\
        longjmp(sched, 1);\
    }\
} */

int thread_create(void *(*start_routine) (void *), void* arg) {
	Thread *thread = new Thread();
    thread->thread_id = thread_counter++;
    thread->func = start_routine;
    thread->arg = arg;
    
    
    using_queue.lock();
    //Push to thread queue. 
    threads.push(thread);
    cout << "Queue size is: " << threads.size() << "\n";
    using_queue.unlock();
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

void some_function() {
    cout << "Entered function\n";
    int counter = 1;
    int i;

    while(1) {
        cout << "Trying to run\n";    
        
        for( i=0; i < 10; i++) {
            cout << "Executing function!: " << counter << "\n";
        }
    
    }
    
}

int main() {
    cout << "Hello, Threads!\n";
    
    
    thread_create(some_function, 1);
    
    thread_create(some_function, 2);
    
    thread_create(some_function, 3);
    
    thread_create(some_function, 4);

    std::this_thread::sleep_for(10s);
    

}
