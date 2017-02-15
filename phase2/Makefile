#Makefile for Phase 2 of JAEOS#

#Written by Jake Wagner#
#Last Updated: 2-23-16#

SUPDIR = /usr/include/uarm

DEFS = ../h/const.h ../h/types.h ../e/asl.e ../e/pcb.e ../e/initial.e ../e/interrupts.e ../e/scheduler.e ../e/exceptions.e $(SUPDIR)/libuarm.h Makefile

CFLAGS =  -mcpu=arm7tdmi -c
LDCOREFLAGS =  -T $(SUPDIR)/ldscripts/elf32ltsarm.h.uarmcore.x
MATHFLAGS = $(SUPDIR)/crtso.o $(SUPDIR)/libdiv.o

CC = arm-none-eabi-gcc 
LD = arm-none-eabi-ld

#main target
all: kernel.core.uarm 

kernel.core.uarm: kernel
	elf2uarm -k kernel

kernel: p2test.o initial.o interrupts.o scheduler.o exceptions.o asl.o pcb.o
	$(LD) $(LDCOREFLAGS) -o kernel p2test.o initial.o interrupts.o scheduler.o exceptions.o asl.o pcb.o $(MATHFLAGS) $(SUPDIR)/libuarm.o

p2test.o: p2test.c $(DEFS)
	$(CC) $(CFLAGS) p2test.c

initial.o: initial.c $(DEFS)
	$(CC) $(CFLAGS) initial.c
	
interrupts.o: interrupts.c $(DEFS)
	$(CC) $(CFLAGS) interrupts.c
	
scheduler.o: scheduler.c $(DEFS)
	$(CC) $(CFLAGS) scheduler.c

exceptions.o: exceptions.c $(DEFS)
	$(CC) $(CFLAGS) exceptions.c

asl.o: ../phase1/asl.c $(DEFS)
	$(CC) $(CFLAGS) ../phase1/asl.c

pcb.o: ../phase1/pcb.c $(DEFS)
	$(CC) $(CFLAGS) ../phase1/pcb.c



clean:
	rm -f *.o term*.uarm kernel


distclean: clean
	-rm kernel.*.uarm
