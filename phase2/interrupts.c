/***********************************************************************
* INTERRUPTS.C
* 
* This file contains the methods to handle interrupts in the JAEOS 
* operating system. An interrupt can only occur when the current 
* processor state has its fast interrupt request disabled and interrupt 
* request disabled bit's turned off.
* 
* An interrupt can be caused by the interval timer going off or devices
* requiring acknowlegment. When the interval timer goes off, it can
* either be a process' quantum ending or the psuedo clock timer going
* off.
* 
* There are five devices that are supported: disk, tape, network, 
* printer and terminal devices. All devices except for the two clocks 
* and terminal are handled by performing a V operation on the specified 
* device semaphore and unblocking a waiting process.
* 
* The same happens for terminal devices except that there is only one
* device for read and write and two semaphores, one for each case. Here,
* writing takes precedence over reading. In every case, the device is
* acknowledged and its status is stored in the return value. 
* 
* When an interrupt occurs, the current TOD is stored and the elapsed
* time that the current process has been running is stored to the
* the processes time sheet since the interrupt may not be occuring on
* behalf of that process. It also subtracts the elapsed time from the 
* time left to the next pseudo clock interrupt.
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
#include "../e/interrupts.e"
#include "../e/initial.e"

#include "/usr/include/uarm/libuarm.h"

/***********************Global Definitions*****************************/

/*No Global Definitions*/

/*************************Main Functions*******************************/

/***********************************************************************
 *Function that handles interrupts. It handles the interrupts caused by
 *the psuedo-clock, disk, tape, network, printer and terminal
 *devices. Terminal interrupts are passed down to a different function
 *while all others are handled here. The handler acknowledges each
 *interrupt and either returns to the current job or gets a new job.
 *RETURNS: N/a
 **********************************************************************/
void interruptHandler(){
		
	/*Local Variable Declarations*/
	int devNum;
	int lineNum;
	int deviceIndex;
	cpu_t elapsedTime;
	device_t* dev;
	pcb_PTR process;
	unsigned int currentStatus;
	state_t* oldInt = (state_t *) INTERRUPTOLDADDR;
	devregarea_t* devReg = (devregarea_t *) DEVREGAREAADDR;
	int pendingDevice = oldInt->s_CP15_Cause >> 24;
	
	/*Decrement pc to the instruction that was executing*/
	oldInt->s_pc = oldInt->s_pc - 4;
	
	/*If there was a process running...*/
	if(currentProcess != NULL){
		cpu_t stopTOD;
		
		/*Store ending TOD*/
		STCK(stopTOD);
		
		/*Store elapsed time*/
		elapsedTime = stopTOD - startTOD;
		currentProcess->p_time = currentProcess->p_time + elapsedTime;
		timeLeft = timeLeft - elapsedTime;
		
		/*Move the old state into the current process*/
		moveState(oldInt, &(currentProcess->p_s));
	}
		
	/*If the interrupt was an interval timer interrupt...*/
	if((pendingDevice & LINETWO) == LINETWO){
				
		/*If it was the interval timer...*/ 
		if(intTimerFlag || (timeLeft < 0)){
			
			/*Unblock the first process*/
			process = removeBlocked(&(semaphoreArray[CLCKTIMER]));
			
			/*While there is still a process to remove...*/
			while(process != NULL){
				
				process->p_semAdd = NULL;
				softBlockCount--;
				
				/*Add it to the ready queue*/
				insertProcQ(&(readyQueue), process);
				
				/*Remove the next process*/
				process = removeBlocked(&(semaphoreArray[CLCKTIMER]));
			}
			
			/*Set the seamphore to zero*/
			semaphoreArray[CLCKTIMER] = 0;

			/*Reload the interval timer*/
			setTIMER(QUANTUM);
			timeLeft = INTERVALTIME;
			intTimerFlag = FALSE;
					
			/*Leave the interrupt*/
			returnFromInterrupt();
			
		}
		/*It was a process's quantum ending*/
		else{
		
			/*If there was a process running...*/
			if(currentProcess != NULL){
			
				/*Add the process back to the ready queue*/
				insertProcQ(&(readyQueue), currentProcess);
				currentProcess = NULL;
			}
			
			setTIMER(QUANTUM);
			
			/*Get a new job*/
			getNewJob();
		}
	}
	else{
		/*If the interrupt was a disk device interrupt...*/
		if((pendingDevice & LINETHREE) == LINETHREE){
			
			/*Get the device number*/
			devNum = getDeviceNumber(DISKINT);
			lineNum = DISKINT;
		}
		
		/*If the interrupt was a tape device interrupt...*/
		else if((pendingDevice & LINEFOUR) == LINEFOUR){
			
			/*Get the device number*/
			devNum = getDeviceNumber(TAPEINT);
			lineNum = TAPEINT;
		}
		
		/*If the interrupt was a network device interrupt...*/
		else if((pendingDevice & LINEFIVE) == LINEFIVE){
			
			/*Get the device number*/
			devNum = getDeviceNumber(NETWINT);
			lineNum = NETWINT;
		}
		
		/*If the interrupt was a printer device interrupt...*/
		else if((pendingDevice & LINESIX) == LINESIX){
			
			/*Get the device number*/
			devNum = getDeviceNumber(PRNTINT);
			lineNum = PRNTINT;
		}
		
		/*If the interrupt was a terminal device interrupt...*/
		else if((pendingDevice & LINESEVEN) == LINESEVEN){
			
			/*Get the device number*/
			devNum = getDeviceNumber(TERMINT);
			lineNum = TERMINT;
		}
		
		/*If the interrupt was a terminal interrupt...*/
		if(lineNum == TERMINT){
					
			/*Pass it to a helper*/
			handleTerminal(devNum);
		}
		
		/*Get the modified line number*/
		lineNum = lineNum - DISKINT;
		
		/*Get the index of the device*/
		deviceIndex = (DEVPERINT * lineNum) + devNum;
		
		/*Get the device generating the interrupt*/
		dev = (device_t *) (devReg->devregbase + (deviceIndex * DEVREGSIZE));
		
		/*Increment semaphore address*/
		semaphoreArray[deviceIndex] = semaphoreArray[deviceIndex] + 1;
		
		if(semaphoreArray[deviceIndex] <= 0){
			
			/*Unblock the next process*/
			process = removeBlocked(&(semaphoreArray[deviceIndex]));
			if(process != NULL){
				process->p_semAdd = NULL;
				
				/*Set status of interrupt for the waiting process*/
				process->p_s.s_a1 = dev->d_status;		
				softBlockCount--;
				
				/*Add it to the ready queue*/
				insertProcQ(&(readyQueue), process);
			}
			else{
				/*Set status of interrupt for the current process*/
				devStatus[deviceIndex] = dev->d_status;
			}
		}
		
		/*Acknowledge the command*/
		dev->d_command = ACK;
		
		/*Leave the interrupt*/
		returnFromInterrupt();
	}
	
}

/*************************Helper Functions*****************************/

/***********************************************************************
 *Function that gets the device number of the device generating an
 *interrupt. It examines the bit map and determines which bit is on and
 *the device generating the interrupt.
 *RETURNS: The number of the device on the line generating the interrupt
 **********************************************************************/
int getDeviceNumber(int lineNumber){
	
	/*Local Variable Declarations*/
	unsigned int *devBitMap;
	unsigned int currentDevice = DEVICEONE;
	int devNum = 0;
	int found = FALSE;
	
	/*Get the modified line number*/
	lineNumber = lineNumber - 3;
	
	/*Examine the bit map*/
	devBitMap = (unsigned int *) (INTBITMAPADDR + (lineNumber * DEVREGLEN));
	
	/*While the interrupting device hasn't been found...*/
	while(!found){
		
		/*If the bit is on...*/
		if((currentDevice & *devBitMap) == currentDevice){
			found = TRUE;
		}
		
		/*Move to the next bit*/
		else{
			currentDevice = currentDevice << 1;
			devNum++;
		}
	}
	
	/*Return the number of the device on the line*/
	return devNum;
}

/***********************************************************************
 *Function that handles terminal interrupts. It checks first to see if
 *the terminal was transmitting. Then it performs a V operation on the
 *semaphore for that device and acknowledges the interrupt.
 *RETURNS: N/a
 **********************************************************************/
void handleTerminal(int devNumber){	
	
	/*Local Variable Declarations*/
	pcb_PTR process;
	int semAdd = (TERMINT-DISKINT)*DEVPERINT + devNumber;
	int receive = TRUE;
	devregarea_t* devReg = (devregarea_t *) DEVREGAREAADDR;
	device_t* dev = (device_t *) (devReg->devregbase + (semAdd * DEVREGSIZE));
	
	/*If a character was transmitted and is not ready...*/
	if((dev->t_transm_status & 0x0F) != READY){
		
		/*Increase the semaphore device number by 8*/
		semAdd = semAdd + DEVPERINT;
		receive = FALSE;
	}
	
	/*Increment semaphore address*/
	semaphoreArray[semAdd] = semaphoreArray[semAdd] + 1;
	
	if(semaphoreArray[semAdd] <= 0){	
		
		/*Unblock the process*/
		process = removeBlocked(&(semaphoreArray[semAdd]));
		if(process != NULL){
			process->p_semAdd = NULL;
			
			/*If the device was receiving...*/
			if(receive){

				/*Acknowledge the read*/
				process->p_s.s_a1 = dev->t_recv_status;
				dev->t_recv_command = ACK;
			}
			
			/*The device was transmitting*/
			else{

				/*Acknowledge the transmit*/
				process->p_s.s_a1 = dev->t_transm_status;
				dev->t_transm_command = ACK;
			}
			
			softBlockCount--;
			
			/*Add it to the ready queue*/
			insertProcQ(&(readyQueue), process);
		}
	}
	
	/*Leave the interrupt*/
	returnFromInterrupt();
}

/***********************************************************************
 *Function that checks to see if the processor was in a waiting state
 *before the interrupt and either loads the current process or calls to
 *the scheduler.
 *RETURNS: N/a
 **********************************************************************/
void returnFromInterrupt(){
	
	/*If processor wasn't waiting...*/
	if(currentProcess != NULL){
		/*Store start TOD*/
		STCK(startTOD);
		
		/*Continue where it left off*/
		LDST(&(currentProcess->p_s));
	}
	
	/*Otherwise, get a new job*/
	getNewJob();
}
