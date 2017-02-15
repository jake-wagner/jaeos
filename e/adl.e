#ifndef ADL
#define ADL

/********************** ADL.E *******************************
 *
 * The externals declaration file for the Active Delay Daemon
 * Module for JAEOS.
 *
 * Written by Jacob Wagner
 * Last Modified: 11-1-16
 */

#include "../h/types.h"
#include "../h/const.h"

extern int headDelaydTime();
extern int insertDelay(int wakeTime, int procID);
extern int removeDelay();
extern void initADL();

/***************************************************************/

#endif

