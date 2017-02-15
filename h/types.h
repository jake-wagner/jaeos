#ifndef TYPES
#define TYPES

/**************************************************************************** 
 *
 * This header file contains utility types definitions.
 * 
 * Written by Jake Wagner
 * Last Updated: 4-20-16
 ****************************************************************************/

typedef signed int cpu_t;

typedef unsigned int memaddr;

typedef struct {
	unsigned int d_status;
	unsigned int d_command;
	unsigned int d_data0;
	unsigned int d_data1;
} device_t;

#define t_recv_status		d_status
#define t_recv_command		d_command
#define t_transm_status		d_data0
#define t_transm_command	d_data1

typedef struct {
	unsigned int rambase;
	unsigned int ramtop;
	unsigned int devregbase;
	unsigned int todhi;
	unsigned int todlo;
	unsigned int intervaltimer;
	unsigned int timescale;
} devregarea_t;

#define STATEREGNUM	22
typedef struct state_t {
	int	 			s_reg[STATEREGNUM];
} state_t, *state_PTR;

typedef struct pcb_t {
	struct pcb_t *p_next;
	struct pcb_t *p_prev;
	struct pcb_t *p_prnt;
	struct pcb_t *p_child;
	struct pcb_t *p_nextSib;
	struct pcb_t *p_prevSib;
	state_PTR oldSys;
	state_PTR newSys;
	state_PTR oldPrgm;
	state_PTR newPrgm;
	state_PTR oldTlb;
	state_PTR newTlb;
	state_t p_s;
	cpu_t p_time;
	int *p_semAdd;
} pcb_t, *pcb_PTR;

#define	s_a1			s_reg[0]
#define	s_a2			s_reg[1]
#define s_a3			s_reg[2]
#define s_a4			s_reg[3]
#define s_v1			s_reg[4]
#define s_v2			s_reg[5]
#define s_v3			s_reg[6]
#define s_v4			s_reg[7]
#define s_v5			s_reg[8]
#define s_v6			s_reg[9]
#define s_sl			s_reg[10]
#define s_fp			s_reg[11]
#define s_ip			s_reg[12]
#define s_sp			s_reg[13]
#define s_lr			s_reg[14]
#define s_pc			s_reg[15]
#define s_cpsr			s_reg[16]
#define s_CP15_Control	s_reg[17]
#define s_CP15_EntryHi	s_reg[18]
#define s_CP15_Cause	s_reg[19]
#define s_todHI			s_reg[20]
#define s_todLO			s_reg[21]

typedef struct pteEntry_t {
	unsigned int	pte_entryHI;
	unsigned int	pte_entryLO;
} pteEntry_t;

typedef struct pte_t {
	int				header;
	pteEntry_t		pteTable[KUSEGPTESIZE];
} pte_t;

typedef struct pteOS_t {
	int 		header;
	pteEntry_t	pteTable[KSEGOSPTESIZE];
} pteOS_t;

typedef struct segTbl_t {
	pteOS_t			*ksegOS;
	pte_t			*kUseg2;
	pte_t			*kUseg3;
} segTbl_t;

typedef struct Tproc_t {
	int			Tp_sem;
	pte_t		Tp_pte;
	int			Tp_bckStoreAddr;
	state_t		Tnew_trap[TRAPTYPES];
	state_t		Told_trap[TRAPTYPES];
} Tproc_t, *Tproc_PTR;

typedef struct swap_t {
	int			sw_asid;
	int			sw_segNo;
	int			sw_pageNo;
	pteEntry_t	*sw_pte;
} swap_t;


#endif
