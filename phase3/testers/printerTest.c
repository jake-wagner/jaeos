/* Does nothing but outputs to the printer and terminates */
#include "../../h/const.h"
#include "../../h/types.h"

#include "/usr/include/uarm/libuarm.h"

#include "h/tconst.h"
#include "print.e"

void main() {
	print(WRITETERMINAL, "printTest is ok\n");
	
	print(WRITEPRINTER, "printTest is ok\n");
	
	SYSCALL(TERMINATE, 0, 0, 0);
}
