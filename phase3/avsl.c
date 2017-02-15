/***********************************************************************
* AVSL.C
* 
* This file creates and maintains the Active Virtual Semaphore List in
* the JAEOS operating system.
* 
* The Active Virtual Semaphore List is used to keep track of blocked
* user processes and their virtual semaphore addresses. It is maintained
* using a circular doubly linked list with a head pointer. Virtual
* Semaphore Descriptor nodes are not kept in any particular order.
* 
* This interface has mutator methods to insert a virtual semaphore
* descriptor node onto the list and remove a specified virtual semaphore
* descriptor node from the list. It has no accessor methods.
*
* Written by Jake Wagner
* Last Updated: 11-1-16
***********************************************************************/

#include "../h/const.h"
#include "../h/types.h"

/***********************Global Definitions*****************************/

/*Definition of a virtual semaphore descriptor*/
typedef struct virtSemd_t{
	
	struct virtSemd_t	*vs_next;
	struct virtSemd_t	*vs_prev;
	int 				*vs_semaphore;
	int					vs_procID;
	
} virtSemd_t;

/*The pointer to the head of the Active Virtual Semaphore List*/
HIDDEN virtSemd_t *virtSemd_h;

/*The pointer to the head of the Virtual Semaphore Free List*/
HIDDEN virtSemd_t *virtSemdFree_h;

/************Virtual Semaphore Free List Implementation****************/

/***********************************************************************
 *Function that returns a virtual semaphore node that is no longer in
 *use back to the free list.
 *RETURNS: N/a
 **********************************************************************/
HIDDEN void freeVirtSemd(virtSemd_t *virtSemd){
	
	/*No elements are on free stack, set first one*/
	if(virtSemdFree_h == NULL){
		virtSemdFree_h = virtSemd;
		virtSemdFree_h->vs_next = NULL;
	}
	/*Push items on the free stack*/
	else{
		virtSemd->vs_next = virtSemdFree_h;
		virtSemdFree_h = virtSemd;
	}
}

/***********************************************************************
 *Function that removes a virtual semaphore node from the free list and
 *returns it.
 *RETURNS: a pointer to a virtual semaphore node
 **********************************************************************/
HIDDEN virtSemd_t *allocVirtSemd(){
	
	virtSemd_t *retSemd;
	
	/*If the head of the list is NULL...*/
	if(virtSemdFree_h == NULL){
		return NULL;
	}
	
	/*The head of the list is not NULL*/
	retSemd = virtSemdFree_h;
	
	if(virtSemdFree_h->vs_next == NULL){
		virtSemdFree_h = NULL;
	}
	
	/*The node's next is not NULL*/
	else{
		
		virtSemdFree_h = virtSemdFree_h->vs_next;
		retSemd->vs_next = NULL;
	}
	/*Wash the dishes*/
	retSemd->vs_next = NULL;
	retSemd->vs_prev = NULL;
	retSemd->vs_semaphore = NULL;
	retSemd->vs_procID = -1;
	
	return retSemd;
}

/***********************************************************************
 *Function that initializes the Active Virtual Semaphore list and the 
 *Virtual Semaphore free lists.
 *RETURNS: N/a
 **********************************************************************/
void initAVSL(){
	
	int i;
	/*Create a static array of virtual semaphores*/
	static virtSemd_t virtSemdTable[(MAXPROC + 1)];
	virtSemdFree_h = NULL;
	virtSemd_h = NULL;
	
	for (i = 0; i < (MAXPROC + 1); i++){
		/*Add it to the Virtual Semaphore Free List*/
		freeVirtSemd(&(virtSemdTable[i]));
	}
}


/***********Active Virtual Semaphore List Implementation***************/

/***********************************************************************
 *Function that allocates a new virtual semaphore node, populates it 
 *with the given semaphore address and process ID and weaves it into the
 *the active semaphore list.
 *RETURNS: TRUE if a new semaphore was successfully allocated, FALSE if
 *there were no more available semaphores
 **********************************************************************/
int vInsertBlocked(int *vSemAdd, int procID){
	
	/*Allocate a new virtual semaphore*/
	virtSemd_t *newVSemd = allocVirtSemd();
	
	/*If one could not be allocated...*/
	if(newVSemd == NULL){
		return FALSE;
	}
	
	/*Populate semaphore values*/
	newVSemd->vs_semaphore = vSemAdd;
	newVSemd->vs_procID = procID;
	
	/*If the active list is empty...*/
	if(virtSemd_h == NULL){
		
		virtSemd_h = newVSemd;
		virtSemd_h->vs_next = virtSemd_h;
		virtSemd_h->vs_prev = virtSemd_h;
	}
	
	/*If the active list is not empty...*/
	else{
		
		/*Weave into the list*/
		newVSemd->vs_next = virtSemd_h;
		virtSemd_h->vs_prev->vs_next = newVSemd;
		newVSemd->vs_prev = virtSemd_h->vs_prev;
		virtSemd_h->vs_prev = newVSemd;
	}
	
	return TRUE;
}

/***********************************************************************
 *Function that searches for a semaphore with the given semaphore 
 *address, removes it from the list and returns it to the free list.
 *RETURNS: the process ID with the matching semaphore address or FALSE
 *if the semaphore was not found.
 **********************************************************************/
int vRemoveBlocked(int *vSemAdd){
	
	int found = FALSE;
	int retProcID;
	virtSemd_t *currentVSemd = NULL;
	virtSemd_t *temp = NULL;
	
	/*If there is no list...*/
	if(virtSemd_h == NULL){
		return FALSE;
	}
	
	/*If there is only one semaphore in the list...*/
	if(virtSemd_h->vs_next == virtSemd_h){
		if(virtSemd_h->vs_semaphore == vSemAdd){
			
			/*Cleanup and save process ID*/
			retProcID = virtSemd_h->vs_procID;
			virtSemd_h->vs_next = NULL;
			virtSemd_h->vs_prev = NULL;
			freeVirtSemd(virtSemd_h);
			virtSemd_h = NULL;
			
			return retProcID;
		}
		
		/*The semaphore doesn't exist*/
		return FALSE;
	}
	
	/*If there is more than one node in the list and head is the one 
	 *being searched for*/
	if(virtSemd_h->vs_semaphore == vSemAdd){
		
		/*Unweave, reassign head and save process ID*/
		retProcID = virtSemd_h->vs_procID;
		virtSemd_h->vs_next = virtSemd_h->vs_next->vs_next;
		virtSemd_h->vs_prev = virtSemd_h->vs_prev->vs_prev;
		
		temp = virtSemd_h->vs_next;
		freeVirtSemd(virtSemd_h);
		virtSemd_h = temp;
		
		return retProcID;
	}
	
	/*The node being searched for is not the head*/
	currentVSemd = virtSemd_h->vs_next;
	while(!found && currentVSemd != virtSemd_h){
		if(currentVSemd->vs_semaphore == vSemAdd){
			found = TRUE;
		}
		else{
			currentVSemd = currentVSemd->vs_next;
		}
	}
	
	/*If we made it all the way around without finding the node..*/
	if(currentVSemd == virtSemd_h){
		return FALSE;
	}
	
	/*Save process ID and unweave the node*/
	retProcID = currentVSemd->vs_procID;
	currentVSemd->vs_prev->vs_next = currentVSemd->vs_next;
	currentVSemd->vs_next->vs_prev = currentVSemd->vs_prev;
	freeVirtSemd(currentVSemd);
	
	return retProcID;
}
