#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "prioque.h"
#include <limits.h>

#define INFINITY 20000
#define NUM_OF_QUEUE 3


//Intializing structs for Behaviors, Process and ReadyQ
typedef struct ProcessBehavior {
    unsigned long  cpuburst;
    unsigned long  ioburst;
    int repeat;
}ProcessBehavior;

typedef struct Process{
    int pid;
    unsigned long arrival_time;
    unsigned long return_from_IO_time;
    unsigned long cpu_charge_against_quantum;
    unsigned long current_cpuburst;
    int current_q_level;
    unsigned long total_cpu;
    unsigned long total_IO;
    unsigned long ioremaining;
    int good;
    int bad;
    Queue behaviors;
}Process;

typedef struct _ReadyQ{
    int bad;
    int good;
    int quantum;
    Queue  q;
}ReadyQ;


//global variables
int Clock = 0;
int IdleClock = 0;
ReadyQ Q[NUM_OF_QUEUE];
Queue HighQ;
Queue MedQ;
Queue LowQ;
Queue ArrivalQ;
Queue FinishedQ;
Process* IdleBehavior;
int currentProcess = 0;
Queue IOQ;

//Intializing the priorities
void intializing_Priority_queues(void){

//High Priority Queue
Q[0].bad = 1;
Q[0].good = INT_MAX;
Q[0].quantum = 10;
Q[0].q = HighQ;

//Medium Priority Queue
Q[1].bad = 1;
Q[1].good = INT_MAX;
Q[1].quantum = 30;
Q[1].q = MedQ;

//Low Priority Queue
Q[2].bad = INT_MAX;
Q[2].good = 1;
Q[2].quantum = 100;
Q[2].q = LowQ;

//Intializing the Queues 
init_queue(&Q[0].q, sizeof(Process),1, 0,0);
init_queue(&Q[1].q, sizeof(Process),1, 0,0);
init_queue(&Q[2].q, sizeof(Process),1, 0,0);
init_queue(&ArrivalQ, sizeof(Process),1, 0,0);
init_queue(&FinishedQ, sizeof(Process),1,0,0);
init_queue(&IOQ, sizeof(Process),1,0,0);
}



void init_process(Process *p){
    p->pid =0;
    p->arrival_time =0;
    p->return_from_IO_time=0;
    p->cpu_charge_against_quantum =0;
    p->current_q_level=0;
    p->total_cpu=0;
    p->total_IO=0;
    p->good=0;
    p->bad=0;
    init_queue(&(p->behaviors),sizeof(ProcessBehavior),1,0,0);
}


void read_process_descriptions(void){
    Process p;
    ProcessBehavior b;
    int pid = 0, first=1;
    unsigned long arrival;

    init_process(&p);
    arrival=0;
    while(scanf("%lu", &arrival) != EOF){
        scanf("%d %lu %lu %d",
        &pid,
        &b.cpuburst,
        &b.ioburst,
        &b.repeat);

        if(! first && p.pid != pid){
            add_to_queue(&ArrivalQ, &p, p.arrival_time);
            init_process(&p);
        }

        p.pid = pid;
        p.arrival_time=arrival;
        first=0;
        add_to_queue(&p.behaviors, &b, 1);
    }
    add_to_queue(&ArrivalQ, &p , p.arrival_time);
}

//it will check if promotion is possible and if the process need it
void process_promotion(Process *p){
    //if the process cpu charge is less than the quantum then increase the value of good and reintialize bad to 0
    if(p->cpu_charge_against_quantum < Q[p->current_q_level].quantum){
        p->good++;
        p->bad=0;
        //if the good value is higher than the current prioritys good value
        if(p->good >= Q[p->current_q_level].good){
            //and if the current 
            if(p->current_q_level > 0){
                p->current_q_level = p->current_q_level - 1;
            }

            //after reset the value of good as it was either promoted or sent back to the rear of the High Q
            p->good = 0;
        }
        
    }
}

//Will Check if demotion is needed for the process
void process_demotion(Process *p){
    //If cpu charge is == to quantum, it has exhausted the quantum and will increment the bad
    if(p->cpu_charge_against_quantum == Q[p->current_q_level].quantum){
        p->bad++;
        p->good=0;
        //after evaulating the bad evaluate if it has gone over the number of chances to go bad
        if(p->bad >= Q[p->current_q_level].bad){
            //if that is the case then check if it is possible to demote anymore 
            //if not then lower the priority of the queue
            if(p->current_q_level < NUM_OF_QUEUE - 1){
                p->current_q_level = p->current_q_level + 1;
            }

            //reset the process's bad value as it gets placed in a new queue
            p->bad = 0;
        }
    }
}

void queue_new_arrivals() {
     //Adding processes that are waiting to be created. A process will be created when the timer reaches
    //the same number as the process's entry number.
    Process* readProcess;
    ProcessBehavior* b;
    if (ArrivalQ.queuelength > 0) {
      rewind_queue(&ArrivalQ);
      readProcess = pointer_to_current(&ArrivalQ);
      if (readProcess->arrival_time == Clock) {
        b = pointer_to_current(&readProcess->behaviors);
        readProcess->current_cpuburst = b->cpuburst;
        add_to_queue(&Q[0].q, readProcess,0);

        printf("CREATE: Process %d entered the ready queue at time %u.\n",
            readProcess->pid, Clock);
        delete_current(&ArrivalQ);
        //Runs the function again, just in case two processes had the same start time
        queue_new_arrivals();
  
      }
    }
}
//The y u no work priority process
void execute_highest_priority_process(void){
        Process* p;
        ProcessBehavior *b ;
        int level_priority = -1;
        
        

    //rechecking if Queues are empty and if they are start from top and work to bottom with queues
    if(! empty_queue(&Q[0].q))
        {
            level_priority = 0;
        }
    else if(! empty_queue(&Q[1].q))
        {
            level_priority = 1;
        }
    else if(! empty_queue(&Q[2].q))
        {
            level_priority = 2;
        }

//if the priority Queue is still empty then it will idle until process can start again
    if(level_priority < 0){
        IdleClock++;
    }
    
     else{
         //Rewind Both Queues(Process and ProcessBehaviors) to make sure they are pointing at the first element in the Queue's
        rewind_queue(&Q[level_priority].q);
        p = pointer_to_current(&Q[level_priority].q);
        rewind_queue(&p->behaviors);
        b = pointer_to_current(&p->behaviors);

        //Make sure current pid is not the same current pid if it is print the Queued
        
        //check if the process can start running
        if(currentProcess == 0){
            for(int i =0 ; i < 3 && currentProcess == 0; i++){
                if(&Q[i].q.queuelength > 0){
                    rewind_queue(&Q[i].q);
                    currentProcess = 1;
                    printf("RUN: Process %d started execution from level %d at time %u; wants to execute for %lu ticks.\n", p->pid, p->current_q_level+1, Clock, p->current_cpuburst);
                }
            }
        //Start running the queue and print the statement out as given
    //    printf("RUN: Process %d started execution from level %d at time %u; wants to execute for %lu ticks.\n", p->pid, p->current_q_level+1, Clock, p->current_cpuburst);
        }
        //Increment by one tick and decrement the cpu burst as we are attempting to finish the process
        p->cpu_charge_against_quantum++;
        p->total_cpu++;
        p->current_cpuburst--;

    //if cpu burst has finished then the process is done
    if(p->current_cpuburst == 0){
        if(b->repeat == 0){
            printf("FINISHED: Process %d finished at time %u. \n", p->pid, Clock+1);
            add_to_queue(&FinishedQ, p, 1);
        }
    
    else{
        //Check whether to promote
        process_promotion(p);
        //display ioremaining time for do_IO and decrement repeat
        p->ioremaining = b->ioburst;
        p->cpu_charge_against_quantum=0;
        b->repeat--;

        //print and add to IOQ
        printf("I/O: Process %d blocked for I/O at time %u. \n", p->pid, Clock+1);
        add_to_queue(&IOQ,p, 0);
        currentProcess = 0;

    }
    //Kill the element after done with moving it around to another Queue
    delete_current(&Q[level_priority].q);
    
    }
    //if the process has used all of the quantum then demote the process
    else if(p->cpu_charge_against_quantum == Q[level_priority].quantum){
    //demote check
    process_demotion(p);
    p->cpu_charge_against_quantum = 0;
    //send process down the new queue
    add_to_queue(&Q[p->current_q_level].q, p, 1);
    printf("QUEUED: Process %d queued at level %d at time %u. \n",p->pid,p->current_q_level+1, Clock+1);
    //ensure process has been deleted from old queue
    delete_current(&Q[level_priority].q);
    currentProcess = 0;



}
     }
}

void doIO(void){
    //Setting process and make sure its rewinded to first element
    Process* p;
    ProcessBehavior *b;
    rewind_queue(&IOQ);


    //go through the queue and go through IO process
    for(int i = 0; i < IOQ.queuelength; i++){
            
            //current set pointers for current IO queue
            p = pointer_to_current(&IOQ);
            b = pointer_to_current(&p->behaviors);
            
            p->ioremaining--;

            //if no more IO time send back to priority queue
            if(p->ioremaining == 0){
                printf("QUEUED:Process %d queued at level %d at time %u. \n",p->pid,p->current_q_level+1, Clock);
                p->current_cpuburst = b->cpuburst;
                p->total_IO += b->ioburst;
                p->total_cpu += p->total_IO;
                add_to_queue(&Q[p->current_q_level].q,p,0);
                delete_current(&IOQ);
            }

            //if it isnt done then go to next queue
            else{
                if(i != IOQ.queuelength -1)
                    next_element(&IOQ);
            }
    }

}

//Checks if the Queues are empty or if there is more to go through
int processes_exist(void){
    //integer to return either true or false checking if there is any more processes
    int process = 0;

    //Loop to check if queues are empty or not
    for(int i = 0; i < NUM_OF_QUEUE; i++){
        if(! empty_queue(&Q[i].q)){
            //If the queue is not empty set to 1 since there are still queues to go through
            process = 1;
        }
        else{

            //else if it is empty process are not available in current Queues
            process = 0;
        }
    }
    //Ensure that every single Queue is finished including Arrival and IOQ
    return process || !empty_queue(&IOQ) || !empty_queue(&ArrivalQ);
}


//The OOF report
void final_report(void){
Process* p;
    //Printing out the Clock and the IdleClock Time
    printf("\nScheduler shutdown at time %d\n", Clock);
    printf("Total CPU usage for all processes scheduled: \n\n");
    printf("<<Null>> Process Time is %d\n", IdleClock);

   //In this while loop it will loop through the entire Finished Queue and Display the time units for the process
    while(FinishedQ.queuelength != 0){
        rewind_queue(&FinishedQ);
        p = pointer_to_current(&FinishedQ);
        printf(" Process %d: %ld time units. \n" , p->pid, p->total_cpu);
        delete_current(&FinishedQ);
    }

}

//I tried
int main(int argc, char *argv[]){
    intializing_Priority_queues();
    read_process_descriptions();

    while(processes_exist()){
        Clock++;
        queue_new_arrivals();
        execute_highest_priority_process();
        doIO();
    }
    Clock++;
    final_report();
    return 0;
}

