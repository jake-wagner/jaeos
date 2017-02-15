/***********************************************************************
* INITPROC.C
* 
* This file sets up the intitial segment table and eight page tables for
* each of the eight processes that will be running in the JAEOS 
* operating system. It also starts the active delay daemon process and 
* initializes the AVSL and the ADL.
*
* It also contains methods to copy one page of memory at a specified
* location to another specified location, enable and disable interrupts,
* the starting location for each user process.
* 
* Each process begins by reading its specified data (which is determined
* by examining the process ID and reading from the cooresponding tape
* device) and copying it from its tape device and writing it to the
* proper location on the backing store disk device (disk0). It then
* sets up its proper stack addresses, sets up its exception state 
* vectors, creates a new state to start executing code and performs a 
* load state to begin user process execution. This immediately causes
* a TLB trap for a missing/invalid page.
*
* Written by Jake Wagner
* Last Updated 11-1-16
***********************************************************************/

#include "../h/const.h"
#include "../h/types.h"

#include "../e/adl.e"
#include "../e/avsl.e"

#include "../e/initial.e"
#include "../e/initProc.e"
#include "../e/vmIOsupport.e"

#include "/usr/include/uarm/libuarm.h"

/***********************Global Definitions*****************************/

pteOS_t kSegOS;
pte_t kUSeg3;
swap_t swapPool[SWAPSIZE];

int swapSem;
int mutexSemArray[MAXSEMA];
int masterSem;
Tproc_t uProcs[MAXUSERPROC];

/*************************Main Functions*******************************/

void debugF(){
	int a = 4;
	int b = 12;
	a = b;
}

/***********************************************************************
 *Function that handles the creation of every page table for each of the
 *three segments. It does process creation for the specified number of
 *processes and sets up areas for the data associated with each of
 *these processes. It sets up the stack pointers, process ID's, status's
 *and program counters. It also initializes the mutual exclusion
 *semaphores needed for virtual memory.
 *RETURNS: N/a
 **********************************************************************/
void test(){
	 	 
	/*Local Variable Declarations*/
	int i;
	int j;
	state_t procState;
	state_t delayState;
	segTbl_t* segTable;
	devregarea_t *bus = (devregarea_t *) DEVREGAREAADDR;
			 
	/*Set up kSegOS page table*/
	kSegOS.header = (PTEMAGICNO << MAGICNOSHIFT) | KSEGOSPTESIZE;
	for (i = 0; i < KSEGOSPTESIZE; i++){
		
		kSegOS.pteTable[i].pte_entryHI = 
										((0x20000 + i) << ENTRYHISHIFT);
		kSegOS.pteTable[i].pte_entryLO = ((0x20000 + i) << ENTRYHISHIFT) 
											  | DIRTY |  VALID | GLOBAL;
	}
	
	/*Set up kUSeg3 page table*/
	kUSeg3.header = (PTEMAGICNO << MAGICNOSHIFT) | KUSEGPTESIZE;
	for (i = 0; i < KUSEGPTESIZE; i++){
		
		kUSeg3.pteTable[i].pte_entryHI = 
										((0xC0000 + i) << ENTRYHISHIFT);
		kUSeg3.pteTable[i].pte_entryLO = ALLOFF | DIRTY | GLOBAL;
	}
		
	/*Initialize the swap pool*/
	for (i = 0; i < SWAPSIZE; i++){
				
		swapPool[i].sw_asid = -1;
		swapPool[i].sw_pte = NULL;
	}
	
	/*Initialize the swap semaphore to 1 for mutual exclusion*/
	swapSem = 1;
	
	/*Initialize the array of semaphores to 1*/
	for (i = 0; i < MAXSEMA; i++){
		mutexSemArray[i] = 1;
	}
	
	/*Initialize the master semaphore to 0 for synchronization*/
	masterSem = 0;
		
	/*Initialize each process*/
	for (i = 1; i < MAXUSERPROC + 1; i++){
		
		uProcs[i-1].Tp_pte.header = (PTEMAGICNO << MAGICNOSHIFT) | 
														   KUSEGPTESIZE;
														   
		for(j = 0; j < KUSEGPTESIZE; j++){
			
			uProcs[i-1].Tp_pte.pteTable[j].pte_entryHI = 
					 ((0x80000 + j) << ENTRYHISHIFT) | (i << ASIDSHIFT);
			uProcs[i-1].Tp_pte.pteTable[j].pte_entryLO = ALLOFF | DIRTY;
		}
		
		/*Initialize the last entry in the table*/
		uProcs[i-1].Tp_pte.pteTable[KUSEGPTESIZE-1].pte_entryHI = 
						  (0xBFFFF  << ENTRYHISHIFT) | (i << ASIDSHIFT);
													
		/*Find location of the segment table*/
		segTable = (segTbl_t *) (SEGTBLSTART + (i * SEGTBLWIDTH));
		
		/*Point to its respective page tables*/
		segTable->ksegOS = &kSegOS;
		segTable->kUseg2 = &(uProcs[i-1].Tp_pte);
		segTable->kUseg3 = &kUSeg3;
		
		/*Set up the process's state*/
		procState.s_CP15_EntryHi = (i << ASIDSHIFT);
		/*procState.s_sp = EXECTOP - ((i - 1) * UPROCSTCKSIZE);*/
		procState.s_sp = bus->ramtop - (3 * PAGESIZE);
		procState.s_pc = (memaddr) uProcInit;									
		procState.s_cpsr = ALLOFF | SYSTEMMODE;
										
		uProcs[i-1].Tp_sem = 0;
										
		/*Bring the process to life*/
		SYSCALL(CREATEPROCESS, (int)&procState, 0, 0);
	}
	
	/*initADL();*/
	/*initAVSL();*/
	
	delayState.s_CP15_EntryHi = ((MAXUSERPROC + 2) << ASIDSHIFT);
	delayState.s_sp = EXECTOP - (MAXUSERPROC * UPROCSTCKSIZE);
	delayState.s_pc = (memaddr) delayDaemon;									
	delayState.s_cpsr = ALLOFF | SYSTEMMODE;
										
	/*SYSCALL(CREATEPROCESS, (int)&delayState, 0, 0);*/
		
	/*Call passeren on the master semaphore for each process created*/
	for (i = 0; i < MAXUSERPROC; i++){
		
		SYSCALL(PASSEREN, (int)&masterSem, 0, 0);
	}
	
	/*Terminate the creation process*/
	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}

/***********************************************************************
 *Function that sets up the areas for the Virtual Memory I/O Support
 *areas for each of the processes. This allows each process to be
 *interrupted by other processes whenever there is a TLB, Program Trap
 *or Syscall VM exception. It also reads in data from the tape for the
 *specific process and copies the data to the it's specified location
 *on the backingstore disk.
 *RETURNS: N/a
 **********************************************************************/
void uProcInit(){
	
	debugF(0x55555555);
	
	/*Local Variable Declarations*/
	int i;
	state_t newStartState;
	unsigned int tapeStatus, diskStatus;
	memaddr TLBTOP, PROGTOP, SYSTOP;
	device_t* diskDevice;
	device_t* tapeDevice;
	state_PTR oldState, newState;
	int currentBlock = 0;
	int finished = FALSE;
	devregarea_t* devReg = (devregarea_t *) DEVREGAREAADDR;
	
	/*Who am I?*/
	int procID = ((getEntryHi() & ENTRYMASK) >> ASIDSHIFT);
	
	debugF(0x44444444, procID);
		
	/*Get devices for the process*/
	int devNumber = ((TAPEINT - DISKINT) * DEVPERINT) + (procID - 1);
		
	/*Determine current stack addresses*/
	memaddr procExecStkTop = EXECTOP - ((procID - 1) * (UPROCSTCKSIZE));
	
	SYSTOP = PROGTOP = procExecStkTop;
	TLBTOP = SYSTOP - PAGESIZE;
		
	/*Set the process ID and status for each program trap state*/
	for (i = 0; i < TRAPTYPES; i++){
		
		/*Set up the new and old trap addresses*/
		newState = &(uProcs[procID - 1].Tnew_trap[i]);
		oldState = &(uProcs[procID - 1].Told_trap[i]);
		
		newState->s_CP15_EntryHi = procID << ASIDSHIFT;
		newState->s_cpsr = 	ALLOFF;
		newState->s_CP15_Control = ALLOFF | VMON;
		
		/*Depending on the exception type...*/										
		if(i == TLBTRAP){
			newState->s_sp = TLBTOP;
			newState->s_pc = (memaddr) vmMemHandler;
		} 
		else if(i == PROGTRAP){
			newState->s_sp = PROGTOP;
			newState->s_pc = (memaddr) vmPrgmHandler;
		}
		else if(i == SYSTRAP){
			newState->s_sp = SYSTOP;
			newState->s_pc = (memaddr) vmSysHandler;
		}
		
		/*Set up the new area*/
		SYSCALL(SESV, i, (int)oldState, (int)newState);

	}

	finished = FALSE;
	currentBlock = 0;
	
	/*Gain mutex on tape device*/
	SYSCALL(PASSEREN, (int)&mutexSemArray[devNumber], 0, 0);
	
	/*Get appropriate devices*/
	diskDevice = (device_t *) (devReg->devregbase);
	tapeDevice = (device_t *) (devReg->devregbase + (devNumber * DEVREGSIZE));
	
	/*Set the status' to ready*/
	tapeStatus = READY;
	diskStatus = READY;
	
	/*While there is there weren't any problems and there is still
	 * something to read...
	 */
	while((tapeStatus == READY) && !finished){
		
		debugF(0x33333333);
		
		enableInterrupts(FALSE);
		/*Initialize where the data should be written and set command*/
		tapeDevice->d_data0 = OSCODETOP + ((procID - 1) * PAGESIZE);
		tapeDevice->d_command = READBLK;
		
		tapeStatus = SYSCALL(WAITFORIO, TAPEINT, (procID - 1), 0);
		
		debugF(0x22222222);
		
		enableInterrupts(TRUE);

		/*Gain mutex on backing store*/
		SYSCALL(PASSEREN, (int)&mutexSemArray[BACKINGSTORE], 0, 0);
		
		enableInterrupts(FALSE);
		
		/*Perform disk seek*/
		diskDevice->d_command = (currentBlock << SEEKSHIFT) | DISKSEEK;
		diskStatus = SYSCALL(WAITFORIO, DISKINT, BACKINGSTORE, 0);
		
		enableInterrupts(TRUE);
				
		/*If the device finished seeking...*/
		if(diskStatus == READY){
			
			enableInterrupts(FALSE);
			/*Initialize where to read from and set command to write*/
			diskDevice->d_data0 = OSCODETOP + ((procID - 1) * PAGESIZE);
			diskDevice->d_command = ((procID - 1) << SECTORSHIFT) | 
															   WRITEBLK;
															   
			/*Wait for disk write I/O*/
			diskStatus = SYSCALL(WAITFORIO, DISKINT, BACKINGSTORE, 0);
			enableInterrupts(TRUE);
		}
		
		/*Release mutex on backing store*/
		SYSCALL(VERHOGEN, (int)&mutexSemArray[BACKINGSTORE], 0, 0);
		 
		/*If there is nothing else to read from tape...*/
		if(tapeDevice->d_data1 != EOB){
			finished = TRUE;
		}
		
		currentBlock++;
	}
	
	/*Release mutex on tape device*/
	SYSCALL(VERHOGEN, (int)&mutexSemArray[devNumber], 0, 0);
		 
	STST(&newStartState);
	
	/*Set up the new state to start at*/
	newStartState.s_CP15_EntryHi = (procID << ASIDSHIFT);
	newStartState.s_sp = KUSEG3ADDR;
	newStartState.s_cpsr = ALLOFF | USERMODE;
	newStartState.s_CP15_Control = VMON;
	newStartState.s_pc = 0x800000B0;
	
	debugF(0x11111111);
				
	/*Load the new state*/				
	LDST(&newStartState);
}

/***********************************************************************
 *Function that either turns on or off interrupts for the current 
 *processor state.
 *RETURNS: N/a
 **********************************************************************/
void enableInterrupts(int onOff){
	
	/*Get current status*/
	int status = getSTATUS();
	
	/*If disabling interrupts...*/
	if(!onOff){
		status = (status | ALLINTDISABLED);
	}
	else{
		status = (status & ALLINTENABLED);
	}	
	
	/*Update status*/
	setSTATUS(status);
}


/***********************************************************************
 *Function that runs the delay daemon process that wakes up periodically
 *to see if enough time has passed to awaken any processes in the Active
 *Delay Daemon data structure.
 *RETURNS: N/a
 **********************************************************************/
void delayDaemon(){
	
	cpu_t currentTime;
	int procID;
	
	/*For the duration of the machine's miserable life...*/
	while(TRUE){
		
		/*Sleep*/
		SYSCALL(WAITFORCLOCK, 0, 0, 0);
		
		/*Wake up and get current time of day*/
		STCK(currentTime);
		
		/*While there is a process who's time has passed...*/
		while((headDelaydTime() <= currentTime) && 
									(headDelaydTime() != FAILURE)){
			procID = removeDelay();
			
			/*If nothing was pulled off...*/
			if(procID == FAILURE){
				PANIC();
			}
			
			SYSCALL(VERHOGEN, (int) &(uProcs[procID - 1].Tp_sem), 0, 0);
		}
	}
}

/***********************************************************************
 *Function that copies one page of memory from a source memory address
 *to a destination memory address.
 *Returns: N/a
 **********************************************************************/
void copyPage(int* source, int* target){
	
	int i = 0;
	while(i < (PAGESIZE / WORDLEN)){
		*target = *source;
		
		/*Move to the next word*/
		source++;
		target++;
		i++;
	}
}

