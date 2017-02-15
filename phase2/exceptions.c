/***********************************************************************
* EXCEPTIONS.C
* 
* This file contains the methods to handle program trap's, transition
* lookaside buffer (TLB) trap exceptions and syscalls in the JAEOS 
* operating system. Program trap and TLB trap exceptions are handled by 
* either being passed up or killed.
* 
* System calls can occur in either user or system mode. If in user mode,
* if the Syscall is between 1(create new process) and 8(wait for I/O),
* it is represented as a program trap exception and handled as such. If
* in system mode and the syscall is between 1 and 8, it handles each as
* they are supposed to be handled. These syscalls can create a new
* process, kill a process and all of its children, cause the process to
* wait on a semaphore, wake up a process on a semaphore, specify the
* state trap exception vectors for a process, get the current CPU usage
* time for a process, have a process wait for the clock and have a
* process wait for I/O.
* 
* If the syscall is between 9 and 255, it is passed up or killed as a 
* system trap.
*
* Written by Jake Wagner
* Last Updated: 11-1-16
***********************************************************************/

#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "../e/initial.e"

#include "/usr/include/uarm/libuarm.h"

/***********************Global Definitions*****************************/

/*No Global Definitions*/

/*************************Main Functions*******************************/

/***********************************************************************
 *Function that handles a program trap exception. All program trap
 *exceptions are handled as pass up or die as a program trap.
 *RETURNS: N/a
 **********************************************************************/
void progTrpHandler(){
	
	state_t* oldState = (state_t*) PROGTRPOLDADDR;
		
	/*Call a Program Trap Pass Up or Die*/
	passUpOrDie(PROGTRAP);
	
}

/***********************************************************************
 *Function that handles a TLB trap exception. All TLB trap exceptions 
 *are handled as pass up or die as a TLB trap.
 *RETURNS: N/a
 **********************************************************************/
void tlbHandler(){
	
	state_t* oldState = (state_t*) TLBOLDADDR;
	debugA(oldState->s_CP15_Cause & CAUSEMASK);
		
	/*Call a TLB Trap Pass Up or Die*/
	passUpOrDie(TLBTRAP);
	
}

/***********************************************************************
 *Function that handles all syscall exceptions from syscall 1-255. 
 *It handles each different call with a switch statement based on the 
 *syscall number that is stored as a parameter in the sysCallOld area in
 *low memory.
 *RETURNS: Varies based on Syscall Number
 **********************************************************************/
void sysCallHandler(){
		
	/*Local Variable Declarations*/
	pcb_PTR newPcb;
	pcb_PTR process = NULL;
	int *semAdd;
	int semDev;
	cpu_t elapsedTime;
	cpu_t stopTOD;
	state_t *sysCallOld;
	state_t *progTrpOld;
	int system = FALSE;
	
	
	state_t* oldState = (state_t*) SYSCALLOLDADDR;
	int sysCallNum = oldState->s_a1;
	
	/*Increment PC by 8*/
	oldState->s_pc = oldState->s_pc + 8;
	
	/*Move the old state into the current process*/
	moveState(oldState, &(currentProcess->p_s));
	
	/*If the state was in system mode...*/
	if((currentProcess->p_s.s_cpsr & SYSTEMMODE) == SYSTEMMODE){
		system = TRUE;
	}
	
	/*If in system mode...*/
	if(system){
		
		/*Based on the type of Syscall...*/
		switch(sysCallNum){

			/***********************************************************
			*Syscall 1
			*This syscall creates a new process to execute, makes it a
			*child of the current process and add it to the ready queue.
			*It returns whether or not it was successful.
			***********************************************************/
			case CREATEPROCESS:		
				newPcb = allocPcb();
				
				/*If the free list was not empty...*/
				if(newPcb != NULL){
					
					/*Copy SUCCESS code into return register*/
					oldState->s_a1 = SUCCESS;

					processCount++;
					
					/*Copy the state from a1 to the new pcb*/
					moveState((state_t *) oldState->s_a2, 
														&(newPcb->p_s));
					
					/*Make it a child of current process and add it to
					 *the ready queue
					 */
					insertChild(currentProcess, newPcb);
					insertProcQ(&(readyQueue), newPcb);	
				}
				
				/*The free list was empty*/
				else{
					
					/*Copy FAILURE code into return register*/
					oldState->s_a1 = FAILURE;
				}
				
				/*Return to current process*/				
				headBackHome();
				break;
				
			/***********************************************************
			*Syscall 2
			*This syscall kills the current process, all of its children
			*and calls to get a new job.
			***********************************************************/
			case TERMINATEPROCESS:
			
				/*Recursively kill process and children*/
				nukeItTilItPukes(currentProcess);
				currentProcess = NULL;
				
				/*Get a new job*/
				getNewJob();
				break;
				
			/***********************************************************
			 *Syscall 3
			 *This syscall performs a V operation on the specified
			 *semaphore. If the semaphore is less than or equal to zero,
			 *it unblocks a process, adds it to the ready queue and then
			 *returns to the current process.
			 **********************************************************/ 				
			case VERHOGEN:
						
				semAdd = (int *) oldState->s_a2;
				
				/*Increment semaphore address*/
				*semAdd = *semAdd + 1;
				
				if(*semAdd <= 0){
					
					/*Unblock the next process*/
					process = removeBlocked(semAdd);
					process->p_semAdd = NULL;
					
					/*Add it to the ready queue*/
					insertProcQ(&(readyQueue), process);
				}

				/*Return to current process*/
				headBackHome();
				break;

			/***********************************************************
			 *Syscall 4
			 *This syscall performs a P operation on the specified
			 *semaphore. If the semaphore is less than zero, it stores
			 *the elapsed time, subtracts the elapsed time from the time
			 *to the next psuedo clock tick, blocks the process, and 
			 *then calls to get a new job. Otherwise, it returns to the 
			 *current process.
			 **********************************************************/
			case PASSEREN:
							
				semAdd = (int *) oldState->s_a2;
				
				/*Decrement semaphore address*/
				*semAdd = *semAdd - 1;
												
				if(*semAdd < 0){
										
					/*Store ending TOD*/
					STCK(stopTOD);
					
					/*Store elapsed time*/
					elapsedTime = stopTOD - startTOD;
					currentProcess->p_time = currentProcess->p_time 
														+ elapsedTime;
					timeLeft = timeLeft - elapsedTime;
					
					/*Block the currentProcess*/
					insertBlocked(semAdd, currentProcess);
					currentProcess = NULL;
										
					/*Get a new job*/
					getNewJob();
				}
				
				/*Return to current process*/
				headBackHome();
				break;
			
			/***********************************************************
			 *Syscall 5 
			 *This syscall calls to set up the areas to handle all
			 *trap exceptions by calling to a helper function.
			 **********************************************************/
			case SESV:
				
				/*Pass to Syscall 5 helper function*/
				sysFiveHandle(oldState->s_a2); 
				break;
			
			/***********************************************************
			 *Syscall 6
			 *This syscall returns the current CPU time that a process
			 *has been using since its creation and stores it into v0.
			 *It also subracts the elapsed time from the time to next
			 *pseudo clock tick.
			 **********************************************************/
			case GETCPUTIME:
			
				/*Store ending TOD*/
				STCK(stopTOD);
				
				/*Store elapsed time*/
				elapsedTime = stopTOD - startTOD;
				currentProcess->p_time = currentProcess->p_time 
													+ elapsedTime;
				timeLeft = timeLeft - elapsedTime;
				
				/*Copy current process time into return register*/		
				currentProcess->p_s.s_a1 = currentProcess->p_time;
				
				/*Store starting TOD*/
				STCK(startTOD);
				
				/*Return to previous process*/
				headBackHome();
				break;
				
			/***********************************************************
			 *Syscall 7
			 *This syscall performs a P operation on the clock 
			 *semaphore. If the semaphore is less than zero, it stores
			 *the elapsed time, subtracts the elapsed time from the time
			 *to the next psuedo clock tick, blocks the process, and 
			 *then calls to get a new job. Otherwise, it returns to the 
			 *current process.
			 **********************************************************/			
			case WAITFORCLOCK:
				
				semDev = CLCKTIMER;
				
				/*Decrement semaphore address*/
				semaphoreArray[semDev] = semaphoreArray[semDev] - 1;
				if(semaphoreArray[semDev] < 0){
										
					/*Store ending TOD*/
					STCK(stopTOD);
					
					/*Store elapsed time*/
					elapsedTime = stopTOD - startTOD;
					currentProcess->p_time = currentProcess->p_time 
														+ elapsedTime;
					timeLeft = timeLeft - elapsedTime;
					
					/*Block the process*/
					insertBlocked(&(semaphoreArray[semDev]), 
												currentProcess);
					currentProcess = NULL;
					softBlockCount++;
					
					/*Get a new job*/
					getNewJob();
				}
				
				/*ERROR*/				
				PANIC();
				break;
				
			/***********************************************************
			 *Syscall 8
			 *This syscall performs a P operation on the specified
			 *device semaphore. If the semaphore is less than zero, it 
			 *stores the elapsed time, subtracts the elapsed time from
			 *the time to the next pseudo clock tick, blocks the 
			 *process, and then calls to get a new job. Otherwise, it 
			 *returns to the current process.
			 **********************************************************/
			case WAITFORIO:
				
				/*Get the proper semaphore device number*/
				semDev = DEVPERINT*(oldState->s_a2 - DISKINT) + 
														oldState->s_a3;
							
				/*If the terminal is a write terminal...*/					
				if(!(oldState->s_a4) && (oldState->s_a2 == TERMINT)){
					semDev = semDev + DEVPERINT;
				}
				
				/*Decrement semaphore address*/
				semaphoreArray[semDev] = semaphoreArray[semDev] - 1;

				if(semaphoreArray[semDev] < 0){
										
					/*Store ending TOD*/
					STCK(stopTOD);
					
					elapsedTime = stopTOD - startTOD;
					currentProcess->p_time = currentProcess->p_time 
														+ elapsedTime;
					timeLeft = timeLeft - elapsedTime;
					
					/*Block the process*/
					insertBlocked(&(semaphoreArray[semDev]), 
												currentProcess);
					currentProcess = NULL;
					softBlockCount++;	
					
					/*Get a new job*/	
					getNewJob();
				}
				else {
					currentProcess->p_s.s_a1 = devStatus[semDev];
					headBackHome();
				}
				break;
				
			/***********************************************************
			 *SysCall 9-255
			 *These syscalls are handled as pass up or die as a system
			 *trap.
			***********************************************************/ 				
			default:
				
				/*Call a Syscall Pass Up or Die*/
				passUpOrDie(SYSTRAP);
				break;
				
		}
	}
	
	/*The process was not in system mode*/
	
	/*If it was a Syscall 1-8...*/
	if(sysCallNum >= CREATEPROCESS && sysCallNum <= WAITFORIO){
		
		/*Get the new areas in memory*/
		sysCallOld = (state_t *) SYSCALLOLDADDR;
		progTrpOld = (state_t *) PROGTRPOLDADDR;
		
		/*Move state from sysCallOld to progTrpOld*/
		moveState(sysCallOld, progTrpOld);
			
		/*Set cause register to priviledged instruction*/
		progTrpOld->s_CP15_Cause = RESERVED;

		/*Pass up to Program Trap Handler*/
		progTrpHandler();
	}
	
	/*Syscall 9-255*/
	
	/*Call a Syscall Pass Up or Die*/
	passUpOrDie(SYSTRAP);
	
	
}

/*************************Helper Functions*****************************/

/***********************************************************************
 *Function that performs the passing up of a process or killing of it
 *and all of its progeny based on whether or not the process has been
 *set up to handle it. If it's old and new areas have been set, it will
 *pass the process up. Otherwise, kill it and all of its children.
 *RETURNS: N/a
 **********************************************************************/
void passUpOrDie(int type){
	
	/*Based on the type of trap...*/
	switch(type){
		
		case PROGTRAP:
				
			/*If the handler has already been set up...*/
			if(currentProcess->oldPrgm != NULL){
				
				/*Move the areas around*/
				moveState((state_t *) PROGTRPOLDADDR, 
											(currentProcess->oldPrgm));
				moveState(currentProcess->newPrgm, 
												&(currentProcess->p_s));
				
				/*Return to the current process*/
				headBackHome();
			}
			break;
		
		case TLBTRAP:
			
			/*If the handler has already been set up...*/
			if(currentProcess->oldTlb != NULL){
				
				/*Move the areas around*/
				moveState((state_t *) TLBOLDADDR, 
											(currentProcess->oldTlb));
				moveState(currentProcess->newTlb, 
											&(currentProcess->p_s));	
				
				/*Return to current process*/
				headBackHome();
			}
			break;
			
		case SYSTRAP:
		
			/*If the handler has already been set up...*/
			if(currentProcess->oldSys != NULL){
				
				/*Move the areas around*/
				moveState((state_t *) SYSCALLOLDADDR, 
											(currentProcess->oldSys));
				moveState(currentProcess->newSys,
												&(currentProcess->p_s));
				
				/*Return to current process*/
				headBackHome();
			}
			break;
	
	}
	
	/*Kill the job and get a new one*/
	nukeItTilItPukes(currentProcess);
	getNewJob();
}
	
/***********************************************************************
 *Function that handles a syscall 5 system call. It sets up the old and
 *new areas in the current process with the given states in a3 and a4.
 *If a system 5 system call has already been executed for the process,
 *it kills the process and its children.
 *RETURNS: N/a
 **********************************************************************/
void sysFiveHandle(int type){
	/*Get the old status*/
	state_t* oldState = (state_t*) SYSCALLOLDADDR;
	
	/*Based on the type of trap...*/
	switch(type){
		
		case TLBTRAP:
		
			/*If the area hasn't already been populated...*/
			if(currentProcess->oldTlb == NULL){
				
				/*Set old and new areas*/
				currentProcess->oldTlb = (state_t *)oldState->s_a3;
				currentProcess->newTlb = (state_t *)oldState->s_a4;
				
				/*Return to current process*/
				headBackHome();
			}
			
		case PROGTRAP:
			
			/*If the area hasn't been already populated...*/
			if(currentProcess->oldPrgm == NULL){
				
				/*Set old and new areas*/
				currentProcess->oldPrgm = (state_t *)oldState->s_a3;
				currentProcess->newPrgm = (state_t *)oldState->s_a4;
				
				/*Return to current process*/
				headBackHome();
			}
			
		case SYSTRAP:
			
			/*If the area hasn't been already populated...*/
			if(currentProcess->oldSys == NULL){
				
				/*Set old and new areas*/
				currentProcess->oldSys = (state_t *)oldState->s_a3;
				currentProcess->newSys = (state_t *)oldState->s_a4;
				
				/*Return to current process*/
				headBackHome();
			}
	}
	
	/*Kill the job and get a new one*/
	nukeItTilItPukes(currentProcess);
	getNewJob();
}

/***********************************************************************
 *Function that performs the LDST instruction to load the current
 *process state into the processor.
 *RETURNS: N/a
 **********************************************************************/
void headBackHome(){
	
	/*Load the current process*/
	LDST(&(currentProcess->p_s));

}

/***********************************************************************
 *Function that recursively kills a process and all of its children. It
 *performs head recursion and checks to see if the process killed was
 *the current process, on the ready queue or blocked by a semaphore and
 *makes changes to processCount and softBlockCount accordingly.
 *RETURNS: N/a
 **********************************************************************/
void nukeItTilItPukes(pcb_PTR parent){	
	
	/*While it has children...*/
	while(!emptyChild(parent)){
		/*Recursive death on child*/
		nukeItTilItPukes(removeChild(parent));
	}
	
	/*If the current process is the root...*/
	if (currentProcess == parent){
		outChild(parent);
	}
	
	/*If the process is on the ready queue...*/
	else if(parent->p_semAdd == NULL){
		outProcQ(&(readyQueue), parent);
	}
	
	else{
		/*Process is on the asl*/
		outBlocked(parent);
		
		/*If the pcb was on a device semaphore...*/
		if((parent->p_semAdd >= &(semaphoreArray[0])) && 
					(parent->p_semAdd <= &(semaphoreArray[CLCKTIMER]))){
						
			softBlockCount--;
		}
		/*The pcb is normally blocked*/
		else{
			
			/*Decrement semaphore address*/
			*(parent->p_semAdd) = *(parent->p_semAdd) + 1;
		}
	}
	
	/*Free the process block and decrement process count*/
	freePcb(parent);
	processCount--;
}
	
