/***********************************************************************
* SCHEDULER.C
* 
* This file contains methods to schedule jobs in the JAEOS operating 
* system. It removes jobs from the ready queue, sets the interval timer,
* and performs the load state operation on them to set the current 
* processor state to the state of that process. This begins process 
* execution.
* 
* It also contains a method that copies the 22 words in one state area
* in memory into another state area for easier state changing.
*
* Written by Jake Wagner
* Last Updated: 4-20-16
***********************************************************************/

#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"
#include "../e/asl.e"

#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "../e/interrupts.e"
#include "../e/initial.e"

#include "/usr/include/uarm/libuarm.h"

void debugA(int a){
	int b;
	b = a;
}

void debugB(int b){
	int c;
	c = b;
}

/***********************Global Definitions*****************************/

/*No Global Definitions*/

/*************************Main Functions*******************************/

/***********************************************************************
 *Function that takes a job off of the ready queue and executes it. If
 *there are no jobs left on the ready queue, the CPU halts. If it
 *cannot get a new job, if there are processes blocked by I/0, the CPU
 *waits. If there are no processes blocked, the CPU panics.
 *RETURNS: N/a
 **********************************************************************/
void getNewJob(){
	
	/*Declare a new job*/
	pcb_PTR newJob = NULL;

	/*Get a new job from the ready queue*/
	newJob = removeProcQ(&(readyQueue));
	
	/*If there were no jobs on the ready queue...*/
	if(newJob == NULL){
		
		currentProcess = NULL;
		
		/*If there are no more processes to execute...*/
		if(processCount == 0){
			HALT();
		}	
		else if(processCount > 0){
			/*If there are no processes blocked by I/O...*/
			if(softBlockCount == 0){	
										
				PANIC();
			}

			/*If there are processes blocked by I/0...*/
			if(softBlockCount > 0){
				
				setTIMER(timeLeft);
				intTimerFlag = TRUE;
				
				/*Enable interrupts in the processor*/
				setSTATUS(getSTATUS() & ALLINTENABLED);
				WAIT();
			}
		}
	}
	
	else{
		/*Process job*/
		processJob(newJob);
	}
}

/*************************Helper Functions*****************************/

/***********************************************************************
 *Function that takes a new job, sets it to the current process, checks
 *to see if a the timer should be the end of a quantum or the interval
 *timer, then proceeds to perform a load state operation.
 *RETURNS: N/a
 **********************************************************************/
void processJob(pcb_PTR newJob){
		
	/*Set the current process to the new job*/
	currentProcess = newJob;
	
	/*Store starting TOD*/
	STCK(startTOD);
	
	if(timeLeft < 0) timeLeft = 0;
	
	/*If there is less than than one quantum left on the clock...*/
	if(timeLeft < QUANTUM){			
		
		/*Set the new job's timer to be the remaining interval time*/
		setTIMER(timeLeft);
		intTimerFlag = TRUE;
	}
	else{
		/*Set the new job's timer to be a full quantum*/
		setTIMER(QUANTUM);
	}
	
	/*Load the new job*/
	LDST(&(newJob->p_s));
	
}

/***********************************************************************
 *Function that copies the source state into a target state.
 *RETURNS: N/a
 **********************************************************************/
void moveState(state_t* source, state_t* target){
	
	int i;
	
	/*Copy the 22 registers*/
	for(i = 0; i < STATEREGNUM; i++){
		target->s_reg[i] = source->s_reg[i];
	} 
}
