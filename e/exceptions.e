#ifndef EXCEPTIONS
#define EXCEPTIONS

/********************** EXCEPTIONS.E ****************************
*
* The externals declaration file for the Exception Handler
* Module for JAEOS.
*
* Written by Jacob Wagner
* Last Modified: 4-20-16
*/

#include "../h/types.h"
#include "../h/const.h"

extern int processCount;
extern int softBlockCount;
extern pcb_PTR currentProcess;
extern pcb_PTR readyQueue;
extern cpu_t startTOD;
extern int semaphoreArray[MAXSEMA]; 

extern void progTrpHandler();
extern void tlbHandler();
extern void sysCallHandler();
extern void passUpOrDie(int type);
extern void sysFiveHandle(int type);
extern void headBackHome();
extern void nukeItTilItPukes(pcb_PTR parent);

/***************************************************************/

#endif
