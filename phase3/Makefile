#Makefile for Phase 3 of JAEOS#

#Written by Jake Wagner 
#Last Updated: 11-2-16#

SUPDIR = /usr/include/uarm

DEFS = ../h/const.h ../h/types.h ../e/pcb.e ../e/asl.e ../e/initial.e ../e/interrupts.e ../e/scheduler.e ../e/exceptions.e ../e/adl.e ../e/initProc.e ../e/vmIOsupport.e ../e/avsl.e $(SUPDIR)/libuarm.h Makefile

TDEFS = ./testers/print.e ./testers/h/tconst.h ../h/const.h ../h/types.h $(SUPDIR)/libuarm.h Makefile

CFLAGS =  -mcpu=arm7tdmi -c
LDAOUTFLAGS =  -T $(SUPDIR)/ldscripts/elf32ltsarm.h.uarmaout.x
LDCOREFLAGS =  -T $(SUPDIR)/ldscripts/elf32ltsarm.h.uarmcore.x
MATHFLAGS = $(SUPDIR)/crtso.o $(SUPDIR)/libdiv.o

CC = arm-none-eabi-gcc 
LD = arm-none-eabi-ld

UDEV = uarm-mkdev

#main target
all: kernel.core.uarm readTape.uarm fibTape.uarm swapTape.uarm todTape.uarm diskTape.uarm pvATape.uarm pvBTape.uarm printerTape.uarm disk0.uarm disk1.uarm

disk0.uarm:
	$(UDEV) -d disk0.uarm
disk1.uarm:
	$(UDEV) -d disk1.uarm

readTape.uarm: read_t.aout.uarm
	$(UDEV) -t readTape.uarm read_t.aout.uarm

fibTape.uarm: fib_t.aout.uarm
	$(UDEV) -t fibTape.uarm fib_t.aout.uarm

swapTape.uarm: swap_t.aout.uarm
	$(UDEV) -t swapTape.uarm swap_t.aout.uarm

todTape.uarm: tod_t.aout.uarm
	$(UDEV) -t todTape.uarm tod_t.aout.uarm

diskTape.uarm: disk_t.aout.uarm
	$(UDEV) -t diskTape.uarm disk_t.aout.uarm

pvATape.uarm: pvA_t.aout.uarm
	$(UDEV) -t pvATape.uarm pvA_t.aout.uarm

pvBTape.uarm: pvB_t.aout.uarm
	$(UDEV) -t pvBTape.uarm pvB_t.aout.uarm

printerTape.uarm: printer_t.aout.uarm
	$(UDEV) -t printerTape.uarm printer_t.aout.uarm
	

read_t.aout.uarm: read_t
	elf2uarm -a read_t

read_t: print.o readTest.o
	$(LD) $(LDAOUTFLAGS) -o read_t print.o readTest.o $(MATHFLAGS) $(SUPDIR)/libuarm.o

fib_t.aout.uarm: fib_t
	elf2uarm -a fib_t

fib_t: print.o fibTest.o
	$(LD) $(LDAOUTFLAGS) -o fib_t print.o fibTest.o $(MATHFLAGS) $(SUPDIR)/libuarm.o

swap_t.aout.uarm: swap_t
	elf2uarm -a swap_t

swap_t: print.o swapTest.o
	$(LD) $(LDAOUTFLAGS) -o swap_t print.o swapTest.o $(MATHFLAGS) $(SUPDIR)/libuarm.o

tod_t.aout.uarm: tod_t
	elf2uarm -a tod_t

tod_t: print.o todTest.o
	$(LD) $(LDAOUTFLAGS) -o tod_t print.o todTest.o $(MATHFLAGS) $(SUPDIR)/libuarm.o

disk_t.aout.uarm: disk_t
	elf2uarm -a disk_t

disk_t: print.o diskTest.o
	$(LD) $(LDAOUTFLAGS) -o disk_t print.o diskTest.o $(MATHFLAGS) $(SUPDIR)/libuarm.o

pvA_t.aout.uarm: pvA_t
	elf2uarm -a pvA_t

pvA_t: print.o pvTestA.o
	$(LD) $(LDAOUTFLAGS) -o pvA_t print.o pvTestA.o $(MATHFLAGS) $(SUPDIR)/libuarm.o

pvB_t.aout.uarm: pvB_t
	elf2uarm -a pvB_t

pvB_t: print.o pvTestB.o
	$(LD) $(LDAOUTFLAGS) -o pvB_t print.o pvTestB.o $(MATHFLAGS) $(SUPDIR)/libuarm.o

printer_t.aout.uarm: printer_t
	elf2uarm -a printer_t

printer_t: print.o printerTest.o 
	$(LD) $(LDAOUTFLAGS) -o printer_t print.o printerTest.o $(MATHFLAGS) $(SUPDIR)/libuarm.o

readTest.o: ./testers/readTest.c $(TDEFS)
	$(CC) $(CFLAGS) ./testers/readTest.c

fibTest.o: ./testers/fibTest.c $(TDEFS)
	$(CC) $(CFLAGS) ./testers/fibTest.c

swapTest.o: ./testers/swapTest.c $(TDEFS)
	$(CC) $(CFLAGS) ./testers/swapTest.c

todTest.o: ./testers/todTest.c $(TDEFS)
	$(CC) $(CFLAGS) ./testers/todTest.c

diskTest.o: ./testers/diskTest.c $(TDEFS)
	$(CC) $(CFLAGS) ./testers/diskTest.c

pvTestA.o: ./testers/pvTestA.c $(TDEFS)
	$(CC) $(CFLAGS) ./testers/pvTestA.c

pvTestB.o: ./testers/pvTestB.c $(TDEFS)
	$(CC) $(CFLAGS) ./testers/pvTestB.c

printerTest.o: ./testers/printerTest.c $(TDEFS)
	$(CC) $(CFLAGS) ./testers/printerTest.c

print.o: ./testers/print.c $(TDEFS)
	$(CC) $(CFLAGS) ./testers/print.c


kernel.core.uarm: kernel
	elf2uarm -k kernel

kernel: initial.o interrupts.o scheduler.o exceptions.o asl.o pcb.o vmIOsupport.o initProc.o avsl.o adl.o
	$(LD) $(LDCOREFLAGS) -o kernel initial.o interrupts.o scheduler.o exceptions.o asl.o pcb.o vmIOsupport.o initProc.o avsl.o adl.o $(MATHFLAGS) $(SUPDIR)/libuarm.o

initProc.o: initProc.c $(DEFS)
	$(CC) $(CFLAGS) initProc.c

vmIOsupport.o: vmIOsupport.c $(DEFS)
	$(CC) $(CFLAGS) vmIOsupport.c

avsl.o: avsl.c $(DEFS)
	$(CC) $(CFLAGS) avsl.c

adl.o: adl.c $(DEFS)
	$(CC) $(CFLAGS) adl.c

initial.o: ../phase2/initial.c $(DEFS)
	$(CC) $(CFLAGS) ../phase2/initial.c

interrupts.o: ../phase2/interrupts.c $(DEFS)
	$(CC) $(CFLAGS) ../phase2/interrupts.c
 
scheduler.o: ../phase2/scheduler.c $(DEFS)
	$(CC) $(CFLAGS) ../phase2/scheduler.c

exceptions.o: ../phase2/exceptions.c $(DEFS)
	$(CC) $(CFLAGS) ../phase2/exceptions.c
 
asl.o: ../phase1/asl.c $(DEFS)
	$(CC) $(CFLAGS) ../phase1/asl.c

pcb.o: ../phase1/pcb.c $(DEFS)
	$(CC) $(CFLAGS) ../phase1/pcb.c



clean:
	rm -f *.o *.uarm kernel


distclean: clean
	-rm kernel.*.uarm tape0.uarm
