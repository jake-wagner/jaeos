#ifndef VMIOSUPPORT
#define VMIOSUPPORT

/********************** VMIOSUPPORT.E ***************************
 *
 * The externals declaration file for the Virtual Memory Exceptions
 * Module for JAEOS.
 *
 * Written by Jacob Wagner
 * Last Modified: 11-1-16
 */

#include "../h/types.h"
#include "../h/const.h"

extern void vmPrgmHandler();
extern void vmMemHandler();
extern void vmSysHandler();

extern int chooseFrame();
extern void readWriteBacking(int cylinder, int sector, int head, int readWriteComm, memaddr address);
extern void virtualDeath(int procID);
extern void writeTerminal(char* virtAddr, int len, int procID);
extern void readTerminal(char* addr, int procID);
extern void writePrinter(char* virtAddr, int len, int procID);
extern void diskIO(int* blockAddr, int diskNo, int sectNo, int readWrite, int procID);

/***************************************************************/

#endif
