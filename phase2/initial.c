/***********************************************************************
 * INITIAL.C
 * 
 * This file contains the methods to initialize the JAEOS operating
 * system and represents the booting sequence. It initializes the areas 
 * in low memory, initializes the semaphore array and sets up the 
 * process control blocks and semaphores. It then creates a starting job 
 * for the operating system to start executing.
 * 
 * Written by Jake Wagner
 * Last Updated: 11-1-16
 **********************************************************************/

#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"
#include "../e/asl.e"

#include "../e/initial.e"
#include "../e/scheduler.e"
#include "../e/exceptions.e"
#include "../e/interrupts.e"

#include "/usr/include/uarm/libuarm.h"

/***********************Global Definitions*****************************/

int processCount;
int softBlockCount;
pcb_PTR currentProcess;
pcb_PTR readyQueue;
cpu_t startTOD;
int semaphoreArray[MAXSEMA]; 
int devStatus[MAXSEMA];
int intTimerFlag;
cpu_t timeLeft;

extern void test();


/*************************Main Functions*******************************/

/***********************************************************************
 *Function that acts as the boot sequence for the JAEOS Operating 
 *System. It initializes the semaphore array and initializes the 
 *important data areas in low memory. It initializes the process blocks 
 *and semaphore lists and creates a starting process to execute and then
 *calls to the scheduler.
 *RETURNS: N/a
 **********************************************************************/
int main(){
	
	/*Local Variable Declarations*/
	int i;
	pcb_PTR start = NULL;
	state_t *statePtr;
	
	devregarea_t *bus = (devregarea_t *) DEVREGAREAADDR;

	/*Set status to all interrupts disabled and system mode on*/
	setSTATUS(ALLOFF | IRQDISABLED | FIQDISABLED | SYSTEMMODE);
	
	
	/*Initialize array of semaphores to 0*/
	for (i = 0; i < MAXSEMA; i++){
		semaphoreArray[i] = 0;
		devStatus[i] = 0;
	}
	
	/*Populate the four new areas in low memory
	 *	Set the stack pointer to last page of physical
	 *	Set the pc to address of handler
	 *	Set the cpsr to
	 * 		Interrupts disabled
	 * 		Supervisor mode on
	 */
	statePtr = (state_t *) SYSCALLNEWADDR;
	STST(statePtr);
	statePtr->s_pc = (memaddr) sysCallHandler;		
	statePtr->s_sp = bus->ramtop;
	statePtr->s_cpsr = ALLOFF | IRQDISABLED | 
								FIQDISABLED | SYSTEMMODE;
	
	statePtr = (state_t *) PROGTRPNEWADDR;
	STST(statePtr);
	statePtr->s_pc = (memaddr) progTrpHandler;
	statePtr->s_sp = bus->ramtop;
	statePtr->s_cpsr = ALLOFF | IRQDISABLED | 
								FIQDISABLED | SYSTEMMODE;
								
	statePtr = (state_t *) TLBNEWADDR;
	STST(statePtr);
	statePtr->s_pc = (memaddr) tlbHandler;
	statePtr->s_sp = bus->ramtop;
	statePtr->s_cpsr = ALLOFF | IRQDISABLED | 
								FIQDISABLED | SYSTEMMODE;
	
	statePtr = (state_t *) INTERRUPTNEWADDR;
	STST(statePtr);
	statePtr->s_pc = (memaddr) interruptHandler;
	statePtr->s_sp = bus->ramtop;
	statePtr->s_cpsr = ALLOFF | IRQDISABLED | 
								FIQDISABLED | SYSTEMMODE;
	
	/*Initialize PCBs and ASL*/
	initPcbs();
	initASL();
	
	/*Initialize global variables*/
	processCount = 0;
	softBlockCount = 0;
	currentProcess = NULL;
	startTOD = 0;
	readyQueue = mkEmptyProcQ();
	
	/*Allocate a starting process*/
	start = allocPcb();
	
	/*Initialize starting PCB state
	 *	Set the stack pointer to penultimate page of physical memory
	 *	Set the pc to p2test
	 *	Set the cpsr
	 *		Interrupts enabled
	 *		Supervisor mode on
	 */
	STST(&(start->p_s));
	start->p_s.s_pc = (memaddr) test;
	start->p_s.s_sp = bus->ramtop - (2 * PAGESIZE);
	start->p_s.s_cpsr = ALLOFF | SYSTEMMODE;
	
	/*Start the interval timer and set pseudo clock timer*/
	timeLeft = INTERVALTIME;
	intTimerFlag = FALSE;
	setTIMER(QUANTUM);
	 
	/*Increment the number of current processes*/
	processCount++;
	/*Insert the new process onto the ready queue*/
	insertProcQ(&(readyQueue), start);
	
	/*Call to the scheduler*/
	getNewJob();
	
	return 1;
}
