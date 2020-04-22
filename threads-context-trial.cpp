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
#include <set>

typedef std::chrono::high_resolution_clock Clock;
using namespace std;

/*
We need a scheduler and a main. 
We need a thread control block. 
We need a queue of processes.
Need to provide basic functions for thread execution. - Create, Exit and Pre-empt
*/

int thread_counter = 0;


//std::unordered_map<std::thread::id, double> thread_times;

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
set<int> idSet;

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
bool signalset = false;
volatile int initscheduler = 0;


void scheduler();

void THREAD_YIELD(int signum) { 
    
    //This will change the context back to scheduler now. 
    if(signum == SIGALRM) {
        
    cout << "Swapping context!\n";
    cout << "Current thread inside swap context: " << current_thread->thread_id << "\n" ;
    
    signalset = true;
    swapcontext(&(current_thread->saved_context),&sched);
    
    } else {
        cout << "Oh Fuck!\n";
    }
    
    
} 

void joinAllThreads() {
    while(true){
        
        if(!threads.empty())
            continue;
        else {
            cout << "Queue is empty now!\n";
            break;
        }
    }
}

void join_thread(int id) {
    while(true) {
        // cout << "Size of set: " << idSet.size() << "\n";
        if(idSet.count(id) > 0)
            continue;
        else {
            cout << "Joining id: " << id;
            break;
        }
        
    }
}

int getCurrentID(){
	return current_thread->thread_id;
}
void init_scheduler() {
    //cout << "Initializing scheduler\n";

    // Create scheduler context
    ucontext_t uc;
    getcontext(&sched);
    sched.uc_stack.ss_sp = malloc(65536*64);
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
    stack.ss_size = 4096*64;
    stack.ss_sp = malloc( 4096*512 );
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
    cout << "Thread being removed is " << current_thread->thread_id << "\n";
    idSet.erase(current_thread->thread_id);
    cout << "Size of set: " << idSet.size() << "\n";
    free(current_thread);
    current_thread = nullptr;
    cout << "Queue size is: " << threads.size() << "\n";
    
    return;
}

void scheduler() {

    //Need to setup the current process
    
    // current_thread = threads.front();
    //cout << "Current thread set: "<< current_thread->thread_id << "\n";
    
    
    //Initialize scheduler 
    if(!initscheduler) {
        init_scheduler();    
    }  
    

    //Loop through scheduler
    while(1) {

        
        cout << "Queue size in scheduler is: " << threads.size() << "\n";
        
        using_queue.lock();
        
        if(current_thread != nullptr) {
            cout << "Pushing thread: " << current_thread->thread_id << "\n";
            threads.push(current_thread);
        }
            
        
        current_thread = threads.front();
        

        threads.pop();

        cout << "Queue size in scheduler is: " << threads.size() << "\n";

        using_queue.unlock();
        
        cout << "Current thread being scheduled is: " << current_thread->thread_id << "\n";
        
        current_thread->saved_context.uc_link = &sched;
       
        struct itimerval timer; 
        //timer.it_value.tv_sec = 0;
        //timer.it_value.tv_usec = 50000;
    
        setitimer(ITIMER_REAL, &timer, NULL);
        
        swapcontext(&sched, &(current_thread->saved_context));   
        
        
        //TODO: Check if we have come from the threads context or the interuppt context
        // If interuppt continue if not then we call thread_exit
        
        cout << "Is signal set? " << signalset << "\n";
        if(signalset) {
            cout << "Here after swap \n";
            cout << "Seems to be working\n";
            signalset = false;
        } else {
            cout << "Exiting thread!\n";
            setitimer(ITIMER_REAL, NULL, NULL);
                     thread_exit();
        //     cout << "Thread being removed is " << current_thread->thread_id << "\n";
        //     free(current_thread);
        //     current_thread = nullptr;
        //     cout << current_thread << "\n";
        //     cout << "Queue size is: " << threads.size() << "\n";
         }
            
        
        
        if(threads.size() == 0){ 
            setitimer(ITIMER_REAL, NULL, NULL);
            cout << "Threads pending are: " << threads.size() << "\n";
             //return;
        // I don't think this is possible because its on an older thread.
            cout << "Returning";
            break;
         }
            
    }
} 

int thread_create(void *(*start_routine) (void *), void* arg) {

	cout << "Creating thread!";    
        
	Thread *thread = new Thread();
    thread->thread_id = thread_counter++;
    thread->func = start_routine;
    thread->arg = arg;
    
    
    getcontext(&(thread->saved_context));

    thread->saved_context.uc_stack.ss_sp = malloc(4096*64);
    thread->saved_context.uc_stack.ss_size = sz;
    thread->saved_context.uc_stack.ss_flags = 0;

    
	thread->saved_context.uc_link = &sched;
    //cout << "What are we passing!: " << arg << "\n";
    makecontext(&(thread->saved_context), start_routine,1, arg);
    
    
    using_queue.lock();
    //Push to thread queue. 
    threads.push(thread);
    idSet.insert(thread->thread_id);
    //cout << "Queue size is: " << threads.size() << "\n";
    using_queue.unlock();
    
    // Start scheduler on a new thread
    if(threads.size() == 1 && scheduler_activated == 0) {
        
        //cout << "Starting scheduler\n";
        std::thread thread1(scheduler);
        scheduler_activated = 1;
        thread1.detach();
        
    
    } else {
        
        // Notify scheduler thread about thread
        // cout << "Is this blocking?\n";
        // new_thread.notify_all();
    }
       return thread->thread_id; 
}

/*void foo() {
	//cout << "Thread started executing : " << std::this_thread::get_id() << "\n";
	auto t1 = Clock::now();
	//cout << "Value of t1 is: " << t1 << "\n";
	int i;
	int out = 0;
	//Just run a loop 1000 times 
    int a = 0;
	
	for(i = 0; i < 100000; i++){
		a += i;
	}
	printf("a is %d\n",a);
	
	auto t2 = Clock::now();
	//cout << "Thread finished executing : " << std::this_thread::get_id() << " " << std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count() << "\n";
	//thread_times.insert(std::pair<std::thread::id, double>(std::this_thread::get_id(),std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()));
    cout << std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count() << "\n";
    // thread_exit();
}


int main() {
    cout << "Hello, Threads!\n";
    

    getcontext(&main_context); 
	main_context.uc_stack.ss_sp = malloc(1024);
	main_context.uc_stack.ss_size = sz;
	main_context.uc_stack.ss_flags = 0;
	//printf("Got context\n");
	

   
    auto t1 = Clock::now();
	
	cout << "Starting thread\n";

    for(int i =0; i < 100; i++)
        thread_create(foo, nullptr);
	
	cout << "Finished loop!\n";
    
   

    int sum = 0;
    std::this_thread::sleep_for(5s);
    cout << sum;
    cout << "Finished main!\n";
}*/
