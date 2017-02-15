#ifndef INTERRUPTS
#define INTERRUPTS

/********************** INTERRUPTS.E *****************************
*
* The externals declaration file for the Interrupts Exception Handler
* Module for JAEOS.
*
* Written by Jacob Wagner
* Last Modified: 4-20-16
*/

#include "../h/types.h"

extern int waitFlag;
extern cpu_t startTOD;

extern void interruptHandler();
extern int getDeviceNumber(int lineNumber);
extern void handleTerminal(int devNumber);
extern void returnFromInterrupt();


/***************************************************************/

#endif
