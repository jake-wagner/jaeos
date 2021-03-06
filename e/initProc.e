#ifndef INITPROC
#define INITPROC

/********************** INITPROC.E *******************************
 *
 * The externals declaration for the Initialization of User Processes
 * Module for JAEOS.
 *
 * Written by Jacob Wagner
 * Last Modified: 11-1-16
 */

#include "../h/types.h"
#include "../h/const.h"

pteOS_t kSegOS;
pte_t kUSeg3;
swap_t swapPool[SWAPSIZE];

int swapSem;
int mutexSemArray[MAXSEMA];
int masterSem;
Tproc_t uProcs[MAXUSERPROC];

extern void test();
extern void uProcInit();
extern void enableInterrupts(int onOff);
extern void copyPage(int *source, int *target);
extern void delayDaemon();

extern void debugF();

/***************************************************************/

#endif

