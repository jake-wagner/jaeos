#ifndef SCHEDULER
#define SCHEDULER

/*********************** SCHEDULER.E ****************************
*
* The externals declaration file for the Scheduler Module for
* JAEOS.
*
* Written by Jacob Wagner
* Last Modified: 4-20-16
*/

#include "../h/types.h"


extern int processCount;
extern int softBlockCount;
extern pcb_PTR currentProcess;
extern pcb_PTR readyQueue;
extern int waitFlag;
extern cpu_t startTOD;

extern void getNewJob();
extern void processJob(pcb_PTR newJob);
extern void moveState(state_t *source, state_t *target);

/***************************************************************/

#endif
