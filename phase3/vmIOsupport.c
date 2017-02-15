/***********************************************************************
* VMIOSUPPORT.C
* 
* This file contains the methods to handle any user mode level Syscalls,
* Program Trap Exceptions and Memory Management Exceptions in the JAEOS 
* operating system.
* 
* When running with virtual memory on, all program traps are handled by
* ending the running process.
* 
* Any memory management traps (mainly TLB invalid exceptions) are 
* handled by determining the process ID of the offending process, 
* choosing a frame of the swap pool data structure, backing up any 
* current data in the frame if it was occupied to backing store, reading 
* in the missing data from backing store and updating the respective 
* page tables and swap pool data structure. Finally, the TLB is 
* invalidated and execution resumes.
* 
* The available virtual memory Syscalls range from Syscall 9 (Read
* Terminal) to Syscall 18 (Virtual Terminate Process). These Syscalls
* can read data from the terminal, write data to the terminal, perform
* p and v operations on virtual semaphores, delay a process, read to 
* disk and write from disk, write to a printer/file, get the current TOD
* for a process, and terminate a process.
*
* Written by Jake Wagner
* Last Updated: 11-1-16
***********************************************************************/

#include "../h/const.h"
#include "../h/types.h"

#include "../e/adl.e"
#include "../e/avsl.e"

#include "../e/scheduler.e"
#include "../e/initProc.e"
#include "../e/vmIOsupport.e"

#include "/usr/include/uarm/libuarm.h"

/***********************Global Definitions*****************************/

/*No Global Definitions*/

/*************************Main Functions*******************************/

/***********************************************************************
 *Function that handles Virtual Memory Program Trap Exceptions. All
 *exceptions of these types are handled by killing off the process.
 *RETURNS: N/a
 **********************************************************************/
void vmPrgmHandler(){
	
	int procID = ((getEntryHi() & ENTRYMASK) >> ASIDSHIFT);
	
	/*Commit seppuku*/
	virtualDeath(procID);
	
}

/***********************************************************************
 *Function that handles Virtual Memory TLB Trap Exceptions. This
 *function only handles invalid TLB exceptions. All other TLB exceptions
 *are handled by killing off the process. This function examines the
 *offending process and brings in the missing page into the swap pool
 *and updates the swapPool data structure and backs up any necessary
 *information to backingstore.
 *RETURNS: N/a
 **********************************************************************/
void vmMemHandler(){
	
	debugF(0x99999999);

	/*Local Variable Declarations*/
	int missingSegNum, missingPageNum, frameNumber, currentPageNum, 
														  currentProcID;
	memaddr swapAddr;
	devregarea_t* devReg = (devregarea_t *) DEVREGAREAADDR;
	memaddr RAMTOP = devReg->ramtop;
	memaddr SWAPPOOLSTART =  RAMTOP - (2 * PAGESIZE) - 
												  (SWAPSIZE * PAGESIZE);
												  
	int missingProcID = ((getEntryHi() & ENTRYMASK) >> ASIDSHIFT);
	state_t* oldState = (state_t*) 
						  &(uProcs[missingProcID-1].Told_trap[TLBTRAP]);
						  
	int cause = oldState->s_CP15_Cause;
	
	/*If we can't find cause for TLB exception, nuke it!*/
	if((cause != TLBL) && (cause != TLBS)){
		
		debugA(6666, cause);
		
		/*Commit swimpaku*/
		virtualDeath(missingProcID);
	}
	
	/*Which segment and which page is missing?
	Check the segment number & page number respectively*/
	missingSegNum =(oldState->s_CP15_EntryHi >> 30);
	missingPageNum = ((oldState->s_CP15_EntryHi & 0X3FFFF000) >> 12);
	
	/*If the missing page number was higher than the table size...*/
	if(missingPageNum >= KUSEGPTESIZE){
		missingPageNum = KUSEGPTESIZE - 1;
	}
	
	/*Mutex on the swapPool data structure*/
	SYSCALL(PASSEREN, (int)&swapSem, 0, 0);
	
 	/*Pick a frame to use and get it's address*/
 	frameNumber = chooseFrame();
 	swapAddr = SWAPPOOLSTART + (frameNumber * PAGESIZE);
	
 	/*If the frame is currently occupied...*/
 	if(swapPool[frameNumber].sw_asid != -1){

  		enableInterrupts(FALSE);
		
 		/*Turn off the valid bit for the current occupant*/
 		swapPool[frameNumber].sw_pte->pte_entryLO = 
					swapPool[frameNumber].sw_pte->pte_entryLO & ~VALID;
 		
 		/*Clear the TLB*/
 		TLBCLR();
 		enableInterrupts(TRUE);
 		
 		currentProcID = swapPool[frameNumber].sw_asid;
 		currentPageNum = swapPool[frameNumber].sw_pageNo;
 		
 		/*Write the page to backingstore*/
		readWriteBacking(currentPageNum, currentProcID, 
									  USERPROCHEAD, WRITEBLK, swapAddr);
	 	
 	}
	
	/*Read missing page into swap pool*/
	readWriteBacking(missingPageNum, missingProcID, 
									   USERPROCHEAD, READBLK, swapAddr);
	
	enableInterrupts(FALSE);
	
	/*Update swap pool to reflect new page*/
 	swapPool[frameNumber].sw_asid = missingProcID;
 	swapPool[frameNumber].sw_segNo = missingSegNum;
 	swapPool[frameNumber].sw_pageNo = missingPageNum;
 	
 	if(missingSegNum == KUSEG3){
		/*Update kUSeg3 page table*/
		swapPool[frameNumber].sw_pte = 
									 &(kUSeg3.pteTable[missingPageNum]);
		swapPool[frameNumber].sw_pte->pte_entryLO = swapAddr | VALID 
													   | DIRTY | GLOBAL;
	}
	else{
		/*Update the missing page's page table entry*/
		swapPool[frameNumber].sw_pte = 
		   &(uProcs[missingProcID - 1].Tp_pte.pteTable[missingPageNum]);
		swapPool[frameNumber].sw_pte->pte_entryLO = swapAddr | VALID 
																| DIRTY;
	}

	/*Update TLB*/
	TLBCLR();
	enableInterrupts(TRUE);
	
	/*Release mutex from swapPool*/
	SYSCALL(VERHOGEN,(int)&swapSem, 0, 0);
	
	/*Return to current process*/
	LDST(oldState);
	
}
/***********************************************************************
 *Function that handles all of the Virtual Memory Syscalls. It has a
 *switch statement that can handle Syscalls 9-18 for user mode 
 *processes.
 *RETURNS: N/a
 **********************************************************************/
void vmSysHandler(){
	
	/*Local Variable Declarations*/
	cpu_t curTOD;
	int retProcID;
	int *vSemAdd;
	cpu_t delayTime;
	int procID = ((getEntryHi() & ENTRYMASK) >> ASIDSHIFT);
	state_t* oldState = (state_t*) &uProcs[procID-1].Told_trap[SYSTRAP];
	
	int sysCallNum = oldState->s_a1;
	
	/*Based on the type of syscall...*/
	switch(sysCallNum){
		
		/***************************************************************
		*Syscall 9
		*This syscall reads the specified characters from the terminal 0
		*device.
		***************************************************************/
		case READTERMINAL:
			
			readTerminal((char*) oldState->s_a2, procID);
			break;
		
		/***************************************************************
		*Syscall 10
		*This syscall writes specified characters to the terminal 0 
		*device.
		***************************************************************/
		case WRITETERMINAL:
			
			writeTerminal((char*) oldState->s_a2, 
												oldState->s_a3, procID);
			break;
			
		/***************************************************************
		*Syscall 11
		*This syscall performs a virtual 'V' operation on the specified
		*virtual semaphore.
		***************************************************************/
		case VSEMVIRT:
		
			vSemAdd = (int *) oldState->s_a2;

			/*Increment the virtual semaphore address*/
			*vSemAdd = *vSemAdd + 1;
			
			if(*vSemAdd <= 0){
				
				/*Virtually unblock the process*/
				retProcID = vRemoveBlocked(vSemAdd);
				
				if(retProcID == FAILURE){
					virtualDeath(procID);
				}
				
				SYSCALL(VERHOGEN, 
						   (int) &(uProcs[retProcID - 1].Tp_sem), 0, 0);
				
			}
			break;
			
		/***************************************************************
		*Syscall 12
		*This syscall performs a virtual 'P' operation on the specified
		*virtual semaphore.
		***************************************************************/
		case PSEMVIRT:

			vSemAdd = (int *) oldState->s_a2;
			
			/*Decrement the virtual semaphore address*/
			*vSemAdd = *vSemAdd - 1;
			
			if(*vSemAdd < 0){
				
				/*Virtually block the process*/
				vInsertBlocked(vSemAdd, procID);
				SYSCALL(PASSEREN, 
							  (int) &(uProcs[procID - 1].Tp_sem), 0, 0);
				
			}

			break;
			
		/***************************************************************
		*Syscall 13
		*This syscall pauses execution for a specific process for the
		*specified amount of time by virtually blocking the process and
		*inserting it onto the active delay list.
		***************************************************************/
		case DELAY:
			
			delayTime = oldState->s_a2 * TIMESCALE;
					
			/*Virtually block the process*/
			vInsertBlocked(&(uProcs[procID - 1].Tp_sem), procID);

			/*Insert a delay node*/
			delayTime = STCK(curTOD) + delayTime;
			insertDelay(delayTime, procID);
			
			SYSCALL(PASSEREN, (int) &(uProcs[procID - 1].Tp_sem), 0, 0);
		
			break;
		
		/***************************************************************
		*Syscall 14
		*This syscall performs a DISK_PUT operation and writes data from
		*the specified virtual address onto the disk at the given block 
		*number.
		***************************************************************/
		case DISK_PUT:
			
			diskIO((int*) oldState->s_a2, oldState->s_a3, 
									  oldState->s_a4, WRITEBLK, procID);
			break;
	
		/***************************************************************
		*Syscall 15
		*This syscall performs a DISK_GET operation and read data from 
		*the disk at the given block number and store it in the
		*specified virtual address.
		***************************************************************/
		case DISK_GET:
		
			diskIO((int*) oldState->s_a2, oldState->s_a3,
									   oldState->s_a4, READBLK, procID);
			break;
		
		/***************************************************************
		*Syscall 16
		*This syscall writes the specified characters to the printer.
		***************************************************************/
		case WRITEPRINTER:
		
			writePrinter((char*) oldState->s_a1, oldState->s_a2, procID);
			break;
		
		/***************************************************************
		*Syscall 17
		*This syscall returns the amount of time that has passed since
		*the system was booted.
		***************************************************************/
		case GETTOD:
			
			/*Get the passed time*/
			STCK(curTOD);
			oldState->s_a1 = curTOD;
			
			break;
		
		/***************************************************************
		*Syscall 18
		*This syscall calls for virtual death of the specified process.
		***************************************************************/
		case VMTERMINATE:
			
			/*Commit sudoku*/
			virtualDeath(procID);
			
			break;
	}
	
	/*Return to execution*/
	LDST(oldState);

}

/*************************Helper Functions*****************************/

/***********************************************************************
 *Function that chooses the next frame to use in the swap pool in a
 *first-in-first-out fashion.
 *RETURNS: Next frame victim
 **********************************************************************/
int chooseFrame(){
	
	static int nextFrame = 0;
	
	nextFrame = (nextFrame + 1) % SWAPSIZE;
	return(nextFrame);
}

/***********************************************************************
 *Function that handles read and write to the backingstore device. Based
 *on whether or not it is a read or write command, it will seek to the
 *specified cyliner, and either read data from the specified sector to
 *the given address or write data from the specified address to that
 *sector. The head determines which side of the disk it will write on.
 *RETURNS: N/a
 **********************************************************************/
void readWriteBacking(int cylinder, int sector, int head, 
									int readWriteComm, memaddr address){
	
	/*Local Variable Declarations*/
	unsigned int diskStatus;
	devregarea_t* devReg = (devregarea_t *) DEVREGAREAADDR;
	device_t* diskDevice = (device_t *) (devReg->devregbase);
	
	/*Error case*/
	if(readWriteComm != WRITEBLK && readWriteComm != READBLK){
		PANIC();
	}
	
	/*Gain mutex on backing store*/
	SYSCALL(PASSEREN, (int)&mutexSemArray[BACKINGSTORE], 0, 0);
	
	/*Perform atomic operation and seek to correct cylinder*/
	enableInterrupts(FALSE);
	
	diskDevice->d_command = (cylinder << SEEKSHIFT) | DISKSEEK;
	diskStatus = SYSCALL(WAITFORIO, DISKINT, BACKINGSTORE, 0);
	enableInterrupts(TRUE);
			
	/*If the device finished seeking...*/
	if(diskStatus == READY){
		
		enableInterrupts(FALSE);
		/*Initialize where to read from and set command to write*/
		diskDevice->d_data0 = address;
		diskDevice->d_command = (head << HEADSHIFT) | 
							((sector-1) << SECTORSHIFT) | readWriteComm;
														   
		/*Wait for disk write I/O*/
		diskStatus = SYSCALL(WAITFORIO, DISKINT, BACKINGSTORE, 0);
		enableInterrupts(TRUE);
	}
	
	/*Release mutex on backing store*/
	SYSCALL(VERHOGEN, (int)&mutexSemArray[BACKINGSTORE], 0, 0);

}

/***********************************************************************
 *Function that handles virtual killing of the specified process. It
 *does all of the cleaning to make sure that the swapPool structure, TLB
 *and the specified process' page table are all invalidated. It then
 *calls to V the master semaphore and terminate the process and it's
 *child processes.
 *RETURNS: N/a
 **********************************************************************/
void virtualDeath(int procID){
	
	/*Local Variable Declarations*/
	int i;
	int modified = FALSE;
	
	/*Mutex on the swapPool data structure*/
	SYSCALL(PASSEREN, (int)&swapSem,0,0);
	
	/*Invalidate the page table and the swapPool entries*/
	enableInterrupts(FALSE);
	for(i = 0; i < SWAPSIZE; i++){
		if(swapPool[i].sw_asid == procID){
			swapPool[i].sw_pte->pte_entryLO = 
							(swapPool[i].sw_pte->pte_entryLO | ~VALID);
			swapPool[i].sw_asid = -1;
			modified = TRUE;
		}
	}
	if(modified){
		TLBCLR();
	}
	enableInterrupts(TRUE);
	
	/*Release mutex on swapPool*/
	SYSCALL(VERHOGEN,(int)&swapSem, 0, 0);
	
	/*Call verhogen on the master semaphore */
	SYSCALL(VERHOGEN, (int)&masterSem, 0, 0);
	
	/*Terminate the process*/
	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}

/***********************************************************************
 *Function that handles the writing of data to the terminal. It takes
 *the specified character pointer and transmits it to the terminal 
 *belonging to the specific process. It then moves to the next character 
 *until it reaches the specified end of the string.
 *RETURNS: N/a
 **********************************************************************/
void writeTerminal(char* virtAddr, int len, int procID){
	
	/*Local Variable Declarations*/
	unsigned int status;
	int count = 0;
	int devNum = TERM0DEV + (procID - 1);
	devregarea_t* devReg = (devregarea_t *) DEVREGAREAADDR;
	device_t* termDev = (device_t *) (devReg->devregbase + (devNum * DEVREGSIZE));
	
	/*Gain mutex on terminal device*/
	SYSCALL(PASSEREN, 
				(int)&mutexSemArray[TERMWRITESEM + (procID - 1)], 0, 0);
	
	/*While there is still more string to write...*/
	while(count < len) {
		
		/*Set command and perform syscall atomically*/
		enableInterrupts(FALSE);
		termDev->t_transm_command = TRANSCHAR | 
							  (((unsigned int) *virtAddr) << CHARSHIFT);
		status = SYSCALL(WAITFORIO, TERMINT, (procID - 1), WRITETERM);
		enableInterrupts(TRUE);
		
		/*If it didn't transmit correctly*/
		if ((status & 0xFF) != TRANSMITCHAR){
			PANIC();
		}
		
		/*Move to the next character*/
		virtAddr++;
		count++;	
	}
	
	/*Release mutex on terminal device*/
	SYSCALL(VERHOGEN, 
				(int)&mutexSemArray[TERMWRITESEM + (procID - 1)], 0, 0);
}

/***********************************************************************
 *Function that handles the reading of data from the terminal. It reads
 *data from the terminal and then stores it in the specified virtual
 *address.
 *RETURNS: N/a
 **********************************************************************/
void readTerminal(char* addr, int procID){
	
	/*Local Variable Declarations*/
	unsigned int status;
	int finished = FALSE;
	int count = 0;
	int devNum = TERM0DEV + (procID - 1);
	state_t* oldState = (state_t*) &uProcs[procID - 1].Told_trap[SYSTRAP];
	devregarea_t* devReg = (devregarea_t *) DEVREGAREAADDR;
	device_t* termDev = (device_t *) (devReg->devregbase + (devNum * DEVREGSIZE));
	
	/*Gain mutex on terminal device*/
	SYSCALL(PASSEREN, 
				 (int)&mutexSemArray[TERMREADSEM + (procID - 1)], 0, 0);
	
	/*While there is still more string to write...*/
	while(!finished) {
		
		/*Set command and perform syscall atomically*/
		enableInterrupts(FALSE);
		termDev->t_recv_command = RECVCHAR;
		status = SYSCALL(WAITFORIO, TERMINT, (procID - 1), READTERM);
		enableInterrupts(TRUE);
		
		/*If the character entered was the end of the line...*/
		if(((status & 0xFF00) >> 8) == (0x0A)){
			finished = TRUE;
		}
		else{
			/*Set the character*/
			*addr = ((status & 0xFF00) >> 8);
			count++;
		}
		
		/*If it didn't transmit correctly*/
		if ((status & 0xFF) != RECEIVECHAR){
			PANIC();
		}
		/*Move to the next character*/
		addr++;	
	}
	
	/*Put the count into v0*/
	oldState->s_a1 = count;
	
	/*Release mutex on terminal device*/
	SYSCALL(VERHOGEN, 
				 (int)&mutexSemArray[TERMREADSEM + (procID - 1)], 0, 0);
}

/***********************************************************************
 *Function that handles the writing of data to the printer. It takes
 *the specified character pointer and transmits it to the printer 
 *belonging to the specific process. It then moves to the next character 
 *until it reaches the specified end of the string.
 *RETURNS: N/a
 **********************************************************************/
void writePrinter(char* virtAddr, int len, int procID){
	
	/*Local Variable Declarations*/
	unsigned int status;
	int count = 0;
	int devNum = PRINT0DEV + (procID - 1);
	devregarea_t* devReg = (devregarea_t *) DEVREGAREAADDR;
	device_t* printDev = (device_t *) (devReg->devregbase + (devNum * DEVREGSIZE));
	
	/*Gain mutex on terminal device*/
	SYSCALL(PASSEREN, (int)&mutexSemArray[devNum], 0, 0);
	
	/*While there is still more string to write...*/
	while(count < len) {
		
		/*Set command and perform syscall atomically*/
		enableInterrupts(FALSE);
		printDev->d_data0 = (unsigned int) *virtAddr;
		printDev->d_command = PRINTCHAR;
		status = SYSCALL(WAITFORIO, PRNTINT, (procID - 1), 0);
		enableInterrupts(TRUE);
		
		/*If it didn't print correctly*/
		if ((status & 0xFF) != READY){
			PANIC();
		}
		
		/*Move to the next character*/
		virtAddr++;
		count++;	
	}
	
	/*Release mutex on terminal device*/
	SYSCALL(VERHOGEN, (int)&mutexSemArray[devNum], 0, 0);
}

/***********************************************************************
 *Function that handles the reading and writing of data to and from the
 *specified disk number. It will either write data from the given block
 *address onto the specific 1D sector number on the disk or read from
 *the specific 1D sector number into the given block address.
 *RETURNS: N/a
 **********************************************************************/
void diskIO(int* blockAddr, int diskNo, int sectNo, 
											 int readWrite, int procID){
	
	/*Local Variable Declarations*/
	int head;
	int sector;
	int cylinder;
	unsigned int diskStatus;
	int* diskBuffer = (int *)(OSCODETOP + (diskNo * PAGESIZE));
	state_t* oldState = 
					   (state_t *) &uProcs[procID-1].Told_trap[SYSTRAP];
	devregarea_t* devReg = (devregarea_t *) DEVREGAREAADDR;
	device_t* diskDevice = (device_t *) (devReg->devregbase + (diskNo * DEVREGSIZE));
	
	/*If attempting to access disk0 or access kSegOS...*/
	if(diskNo <= 0 || (memaddr) blockAddr < KUSEG2ADDR){
		
		/*Commit honnoruburu seppuburu*/
		virtualDeath(procID);
	}
	
	/*Find the head, cylinder and sector for the data*/
	head = sectNo % 2;
	sectNo = (sectNo / 2);
	sector = sectNo % 8;
	sectNo = (sectNo / 8);
	cylinder = sectNo;
	
	/*Error case*/
	if(readWrite != WRITEBLK && readWrite != READBLK){
		PANIC();
	}
	
	/*Gain mutex on disk device*/
	SYSCALL(PASSEREN, (int)&mutexSemArray[diskNo], 0, 0);
	
	/*If data is being written...*/
	if(readWrite == WRITEBLK){
		copyPage(blockAddr, diskBuffer);
	}
	
	/*Perform atomic operation and seek to correct cylinder*/
	enableInterrupts(FALSE);
	
	diskDevice->d_command = (cylinder << SEEKSHIFT) | DISKSEEK;
	diskStatus = SYSCALL(WAITFORIO, DISKINT, diskNo, 0);
	enableInterrupts(TRUE);
	
	/*If the device finished seeking...*/
	if(diskStatus == READY){
		
		enableInterrupts(FALSE);
		/*Initialize data location and read/write command*/
		diskDevice->d_data0 = (memaddr) diskBuffer;
		diskDevice->d_command = (head << HEADSHIFT) | 
							      ((sector) << SECTORSHIFT) | readWrite;
														   
		/*Wait for disk I/O*/
		diskStatus = SYSCALL(WAITFORIO, DISKINT, diskNo, 0);
		enableInterrupts(FALSE);
	
	}

	/*If data is being read...*/
	if(readWrite == READBLK){
		copyPage(diskBuffer, blockAddr);
	}
	
	oldState->s_a1 = diskStatus;
	
	/*Release mutex on disk device*/
	SYSCALL(VERHOGEN, (int)&mutexSemArray[diskNo], 0, 0);
}
