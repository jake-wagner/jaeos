/***********************************************************************
 * ASL.C
 * 
 * This file creates and maintains the Active Semaphore List in the 
 * JAEOS Operating System.
 * 
 * Semaphores are instantiated via an array and kept on a singly linked
 * linear stack with a head pointer. Active Semaphores are kept in a 
 * singly linked linear list with a dummy node at the head and kept in 
 * sorted order by their field "int *semAdd". Each semaphore also has an
 * associated process queue field. '
 * 
 * This interface has mutator methods to 
 * add PCBs to semaphores, add semaphores, remove head pcbs from 
 * semaphores, remove specific pcbs from semaphores and remove 
 * semaphores. It has accessor methods to access the first pcb in a 
 * semaphores process queue.
 *
 * Written by Jake Wagner
 * Last Updated: 11-1-16
 **********************************************************************/

#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"
#include "../e/asl.e"

/***********************Global Definitions*****************************/

/*Definition of a semaphore*/
typedef struct semd_t {

	struct semd_t 	*s_next;
	int 			*s_semAdd;
	pcb_t 			*s_procQ;	

} semd_t;

/*The pointer to the head of the Semaphore Free List*/
HIDDEN semd_t *semdList_h;

/*The pointer to the head of the Active Semaphore List*/
HIDDEN semd_t *activeSemdList_h;

/*************************Helper Functions*****************************/

/***********************************************************************
 *Function that returns the semaphore previous to the specified 
 *semaphore.
 *RETURNS: a pointer to the semaphore preceding the specified semaphore
 **********************************************************************/
HIDDEN semd_t *getPrevSemd(int *semAdd){
	
		semd_t *retSemd = activeSemdList_h;
		
		/*While there is still a node to look at...*/
		while((retSemd->s_next != NULL) &&
					(retSemd->s_next->s_semAdd < semAdd)){
			retSemd = retSemd->s_next;
		}
		return retSemd;
}


/******************Free Semaphore List Implementation******************/

/***********************************************************************
 *Function that adds a Semaphore that is no longer in use to the 
 *Semaphore Free List.
 *RETURNS: N/a
 **********************************************************************/
HIDDEN void freeSemd(semd_t *semd){
	
	/*No elements are on free stack, set first one*/
	if(semdList_h == NULL){
		semdList_h = semd;
		semdList_h->s_next = NULL;
	}
	/*Push items on the free stack*/
	else{
		semd->s_next = semdList_h;
		semdList_h = semd;
	}
}


/***********************************************************************
 *Function that removes a Semaphore from the Semaphore Free List and 
 *returns it.
 *RETURNS: a pointer to a semaphore taken from the free list or NULL if
 *the free list is empty
 **********************************************************************/
HIDDEN semd_t *allocSemd(){
	
	/*If the head of the list is NULL...*/
	if (semdList_h == NULL){
		return NULL;
	}
	/*The head of the list is not NULL*/
	else{
		semd_t *retSemd = semdList_h;

		if (semdList_h->s_next == NULL){
			semdList_h = NULL;		
		}

		/*The node's next is not NULL*/
		else{

			semdList_h = semdList_h->s_next;
			retSemd->s_next = NULL;

		}
		/*Wash the dishes*/
		retSemd->s_procQ = NULL;
		retSemd->s_next = NULL;
		retSemd->s_semAdd = NULL;
		return retSemd;
	}
}


/***********************************************************************
 *Function that initializes the Semaphore Free List.
 *RETURNS: N/a
 **********************************************************************/
void initASL(){
	
	int i;
	semd_t *dummy;
	/*Create a static array of semaphores*/
	static semd_t semdTable[(MAXPROC + 1)];
	semdList_h = NULL;

	for (i = 0; i < (MAXPROC +1); i++){
		/*Add it to the Semaphore Free List*/
		freeSemd(&(semdTable[i]));
	}
	/*Dummy node for the head of the list*/
	dummy = allocSemd();
	dummy->s_next = NULL;
	dummy->s_semAdd = 0;

	/*Set the head to the dummy*/
	activeSemdList_h = dummy;	
}


/***************Active Semaphore List Implementation*******************/

/***********************************************************************
 *Function that inserts a PCB node into the process queue of the 
 *specified semaphore or create the semaphore if it is not already in 
 *the Active Semaphore List.
 *RETURNS: TRUE if a new semaphore cannot be allocated, FALSE if the pcb
 *was added successfully
 **********************************************************************/
int insertBlocked(int *semAdd, pcb_PTR p){

	/*Create a new semaphore pointer*/
	semd_t *newSemd = NULL;

	/*Assign the previous semaphore as our temp variable*/
	semd_t *prevSemd = getPrevSemd(semAdd);
	
	/*If the next semaphore after the found semaphore is not NULL or
	 *is not the semaphore that was being searched for...
	 */
	if ((prevSemd->s_next == NULL) || 
				(prevSemd->s_next->s_semAdd != semAdd)){	
		/*Allocate a new semaphore from the free list*/
		newSemd = allocSemd();

		/*If the semaphore found is NULL...*/
		if (newSemd == NULL){
			return TRUE;
		}

		/*Populate the values*/
		newSemd->s_semAdd = semAdd;
		newSemd->s_procQ = mkEmptyProcQ();
		
		/*Set the process block's semAdd to the semaphore*/
		p->p_semAdd = newSemd->s_semAdd;
		
		/*Insert the pcb into the process queue*/
		insertProcQ(&(newSemd->s_procQ), p);

		/*Weave the semaphore into the list*/	
		newSemd->s_next = prevSemd->s_next;
		prevSemd->s_next = newSemd;
		return FALSE;
	}
	/*Insert the pcb onto the list*/
	insertProcQ(&(prevSemd->s_next->s_procQ), p);

	/*Set the process blockss semAdd to the semaphore*/
	p->p_semAdd = semAdd;
	return FALSE;
}


/***********************************************************************
 *Function that removes the head pcb from the specified semaphore's 
 *process queue.
 *RETURNS: a pcb_PTR to the pcb removed from the specified semaphore's 
 *process queue or NULL if the semaphore wasn't found
 **********************************************************************/
pcb_PTR removeBlocked(int *semAdd){	
		
	/*Create a pcb to return*/
	pcb_PTR retPcb = NULL;
	
	semd_t *retSemd = NULL;

	/*Attempt to find the node*/
	semd_t *prevSemd = getPrevSemd(semAdd);

	/*If the next semaphore after the found semaphore is not NULL or
	 *is not the semaphore that was being searched for...
	 */
	if ((prevSemd->s_next == NULL) || 
				(prevSemd->s_next->s_semAdd != semAdd)){
		return NULL;
	}

	/*Remove the pcb and return it*/
	retPcb = removeProcQ(&(prevSemd->s_next->s_procQ));	
	
	/*If the found semaphores process queue is now empty...*/
	if (emptyProcQ(prevSemd->s_next->s_procQ)){
		
		retSemd = prevSemd->s_next;
		/*Unweave the node*/
		prevSemd->s_next = retSemd->s_next;
		retSemd->s_next = NULL;
		freeSemd(retSemd);
	}
	return retPcb;
}


/***********************************************************************
 *Function that removes the specified pcb from a process queue of a 
 *semaphore.
 *RETURNS: a pcb_PTR to the removed pcb or NULL if the specified pcb was 
 *not part of a semaphore's process queue
 **********************************************************************/
pcb_PTR outBlocked(pcb_PTR p){
	
	/*Create a pcb to return*/
	pcb_PTR retPcb = NULL;
	
	semd_t *retSemd = NULL;
	
	/*Create a new pointer to the node's semaphore*/
	semd_t *prevSemd = getPrevSemd(p->p_semAdd);
	
	if((prevSemd->s_next == NULL) || 
				(prevSemd->s_next->s_semAdd != p->p_semAdd)){
		return NULL;
	}

	/*Call to remove the pcb from the queue of the semaphore*/
	retPcb = outProcQ(&(prevSemd->s_next->s_procQ), p);
	/*If the value was not in the semaphores process queue...*/
	if (retPcb == NULL){
		return NULL;
	}
	/*If the found semaphores process queue is now empty...*/
	if (emptyProcQ(prevSemd->s_next->s_procQ)){
		retSemd = prevSemd->s_next;
		
		/*Unweave the node*/
		prevSemd->s_next = retSemd->s_next;
		retSemd->s_next = NULL;
		freeSemd(retSemd);
	}
	return retPcb;
}


/***********************************************************************
 *Function that returns the head of the specified semaphores process 
 *queue.
 *RETURNS: a pcb_PTR to the pcb at the head of the specified semaphore's
 *process queue or NULL if the semaphore was not found
 **********************************************************************/
pcb_PTR headBlocked(int *semAdd){
	
	/*Attempt to find the node*/
	semd_t *prevSemd = getPrevSemd(semAdd);

	/*If the node returned does not equal the parameter...*/
	if ((prevSemd->s_next == NULL) || 
				(prevSemd->s_next->s_semAdd != semAdd)){
		return NULL;
	}
	
	/*Return the head pcb*/
	return headProcQ(prevSemd->s_next->s_procQ);
}
