#ifndef AVSL
#define AVSL

/********************** ADL.E *******************************
 *
 * The externals declaration file for the Active Virtual Semaphore
 * List Module for JAEOS.
 *
 * Written by Jacob Wagner
 * Last Modified: 11-1-16
 */

#include "../h/types.h"
#include "../h/const.h"

extern int vInsertBlocked(int *vSemAdd, int procID);
extern int vRemoveBlocked(int *vSemAdd);
extern void initAVSL();

#endif

