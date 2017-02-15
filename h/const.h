#ifndef CONSTS
#define CONSTS

/**************************************************************************** 
 *
 * This header file contains utility constants & macro definitions.
 * 
 * Written by Jake Wagner
 * Last Updated: 4-20-16
 ****************************************************************************/

#include "/usr/include/uarm/uARMconst.h"
#include "/usr/include/uarm/arch.h"

/* hardware & software constants */
#define PAGESIZE		4096	/* page size in bytes */
#define WORDLEN			4		/* word size in bytes */
#define PTEMAGICNO		0x2A

#define SWAPSIZE		(2 * MAXUSERPROC)
#define UPROCSTCKSIZE	(2 * PAGESIZE)

/* memory address information */
#define ROMPAGESTART	0x20000000	 /* ROM Reserved Page */
#define OSCODETOP		(ROMPAGESTART + (32 * PAGESIZE))
#define TAPEBUFFTOP		(OSCODETOP + (DEVPERINT * PAGESIZE))
#define DISKBUFFTOP		(TAPEBUFFTOP + (DEVPERINT * PAGESIZE))
#define EXECTOP			(DISKBUFFTOP + ((MAXUSERPROC * 2) * PAGESIZE))

/* addresses of handler new/old areas */
#define INTERRUPTOLDADDR	0x7000
#define INTERRUPTNEWADDR	0x7058
#define TLBOLDADDR			0x70B0
#define TLBNEWADDR			0x7108
#define PROGTRPOLDADDR		0x7160
#define PROGTRPNEWADDR		0x71B8
#define SYSCALLOLDADDR		0x7210
#define SYSCALLNEWADDR		0x7268

/* bus register addresses */
#define INTBITMAPADDR		0x6FE0
#define DEVREGAREAADDR		0x2D0
#define TODLOADDR			0x2E0
#define INTERVALTMR			0x2E4
#define TIMESCALEADDR		0x2E8

/* segment addresses */
#define KUSEG3ADDR			0xC0000000
#define KUSEG2ADDR			0x80000000
#define KSEGOSADDR			0x00008000
#define SEG0				0x00000000

/* segment information */
#define SEGTBLSTART		0x00007600
#define SEGTBLWIDTH		0x0000000C
#define KUSEGPTESIZE	32
#define KSEGOSPTESIZE	64
#define KUSEG3			3

/* current program status (cpsr) bit patterns */
#define ALLOFF			0
#define IRQDISABLED		0x80
#define FIQDISABLED		0x40
#define THUMBENABLED	0x20
#define ALLINTENABLED	0xFFFFFF3F
#define ALLINTDISABLED	0x000000C0

#define USERMODE		0x10
#define FIQMODE			0x11
#define IRQMODE			0x12
#define SPRVSRMODE		0x13
#define ABORTMODE		0x17
#define UNDEFMODE		0x1B
#define SYSTEMMODE		0x1F

/* cpsr and cp15 register masks/clears */
#define INTMASK			0xC0
#define CAUSEMASK		0xFF
#define MODECLEAR		0xFFFFFFE0
#define MODEMASK		0x1F

/* cause constants */
#define TLBL	14
#define TLBS	15

/* lines and devices for interrupt handling */
#define LINEZERO	0x00000001
#define LINEONE		0x00000002
#define LINETWO		0x00000004
#define LINETHREE	0x00000008
#define LINEFOUR	0x00000010
#define LINEFIVE	0x00000020
#define LINESIX		0x00000040
#define LINESEVEN	0x00000080

#define DEVICEONE	0x00000001

/* system control bit (cp15) patterns */
#define VMON			0x1
#define IGNORETHUMB		0x8000

/* process and semaphore information */
#define MAXPROC		20
#define MAXSEM		MAXPROC
#define MAXUSERPROC 1
#define MAXSEMA		49

/* utility constants */
#define	TRUE		1
#define	FALSE		0
#define ON          1
#define OFF         0
#define HIDDEN		static
#define EOS			'\0'
#define SUCCESS		0
#define FAILURE		-1
#define TIMESCALE	1000000

/* syscall numbers */
#define CREATEPROCESS		1
#define TERMINATEPROCESS	2
#define VERHOGEN			3
#define PASSEREN			4
#define SESV				5
#define GETCPUTIME			6
#define WAITFORCLOCK		7
#define	WAITFORIO			8

#define READTERMINAL		9
#define WRITETERMINAL		10
#define VSEMVIRT			11
#define PSEMVIRT			12
#define	DELAY				13
#define DISK_PUT			14
#define DISK_GET			15
#define WRITEPRINTER		16
#define GETTOD				17
#define VMTERMINATE			18

/* time constants */
#define QUANTUM			5000
#define INTERVALTIME	100000

/* entry bit definitions and shifts */
#define DIRTY			(1 << 10)
#define VALID			(1 << 9)
#define GLOBAL			(1 << 8)
#define ENTRYMASK		0x00000FC0
#define ENTRYHISHIFT	12
#define MAGICNOSHIFT	24

/* bit pattern shifts */
#define ASIDSHIFT		6
#define SECTORSHIFT		8
#define HEADSHIFT		16
#define SEEKSHIFT		8
#define CHARSHIFT		8

/* specified device and device semaphore numbers */
#define BACKINGSTORE	0
#define PRINT0DEV		24
#define TERM0DEV		32
#define TERMREADSEM		32
#define TERMWRITESEM	40
#define CLCKTIMER	48

/* trap type numbers */
#define TRAPTYPES	3

#define TLBTRAP		0
#define PROGTRAP	1
#define SYSTRAP		2

/* cause codes */
#define RESERVED	20

/* device interrupts */
#define DISKINT		3
#define TAPEINT 	4
#define NETWINT 	5
#define PRNTINT 	6
#define TERMINT		7

#define DEVPERINT	8
#define DEVINTNUM	5

#define DEVREGLEN	4	/* device register field length in bytes & regs per dev */
#define DEVREGSIZE	16 	/* device register size in bytes */

/* device register field number for terminal devices */
#define RECVSTATUS      0
#define RECVCOMMAND     1
#define TRANSTATUS      2
#define TRANCOMMAND     3

/* printer device command codes */
#define PRINTCHAR		2

/* terminal device command codes */
#define TRANSCHAR		2
#define RECVCHAR		2

/* tape/disk device command codes */
#define DISKSEEK		2
#define READBLK			3
#define WRITEBLK		4
#define	USERPROCHEAD	0
#define GLOBALHEAD		1

/* terminal read/write code */
#define READTERM		1
#define WRITETERM		0

/* disk marker codes */
#define EOT				0
#define EOF				1
#define EOB				2
#define TS				3

/* device common status codes */

/* device STATUS codes */
#define UNINSTALLED		0
#define READY			1
#define TRANSMITCHAR	5
#define RECEIVECHAR		5
#define BUSY			3

/* device COMMAND codes */
#define RESET		0
#define ACK			1

/* useful macros */
#define STCK(T) ((T) = ((* ((cpu_t *) TODLOADDR)) / (* ((cpu_t *) TIMESCALEADDR))))


#endif
