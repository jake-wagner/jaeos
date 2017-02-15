JaeOS, short for Just Another Educational Operating System, is a set of operating system software designed to run on the uARM emulator. The uARM emulator is a simplified version of the ARM7TDMI architecture (for more information, visit https://github.com/mellotanica/uarm). 

The software is written in 3 phases.

Phase 1 contains the data structures used by the emulator. It contains files for Process Control Blocks and the Active Semaphore List.

Phase 2 contains the files for starting up the system, initializing important memory regions, and handlers for system calls, program and tlb trap exceptions and interrupt handling.

Phase 3 contains the files for virtual memory management. It has virtual memory system calls, the active delay daemon and the active virtual semaphore list. This section has not yet been fully implemented.

The header files for each file are in e and the files for declaring constants and types are in h.
