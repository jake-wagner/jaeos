/***********************************************************************
 * PCB.C
 * 
 * This file creates and maintains Process Control Blocks (PCBs) in the
 * JAEOS Operating System. 
 * 
 * PCBs are instantiated via an array and kept on a doubly linked 
 * circular pcbFree list(queue). This list has a tail pointer that 
 * points to the end of the queue. Active PCB's are also kept in a queue
 * format with a pointer to the tail of the queue. 
 * 
 * This interface has mutator methods to add objects to a queue, remove 
 * the first object from the queue, remove a specific object from a 
 * queue and also instantiate new queues. It also has accessor methods 
 * that determine if a given queue is empty and retrieve the item at the 
 * head of the queue.
 * 
 * Each PCB can also be the parent of other PCBs and each child of a 
 * parent is kept in a doubly linked linear queue with a pointer to the 
 * first child in the list. It has mutator methods to add children,
 * remove the first child and to remove a specific child and has a 
 * mutator method to determine if a PCB has any children.
 * 
 * Written by Jake Wagner
 * Last Updated: 11-1-16
 **********************************************************************/

#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"


/**************************Global Definitions**************************/
/*The pointer to the head of the PCB Free List*/
HIDDEN pcb_PTR pcbList_h;


/*********************Free PCB List Implementation*********************/

/***********************************************************************
 *Function that adds a PCB that is no longer in use to the PCB Free 
 *List.
 *RETURNS: N/a
 **********************************************************************/
void freePcb(pcb_PTR p){
	
	/*Wash the dishes*/
		p->p_next = NULL;
		p->p_prev = NULL;
		p->p_prnt = NULL;
		p->p_child = NULL;
		p->p_nextSib = NULL;
		p->p_prevSib = NULL;
		p->p_time = 0;
		
		p->oldSys = NULL;
		p->newSys = NULL;
		p->oldPrgm = NULL;
		p->newPrgm = NULL;
		p->oldTlb = NULL;
		p->newTlb = NULL;
		
	/*Call to insertProcQ*/
	insertProcQ(&(pcbList_h), p);
}


/***********************************************************************
 *Function that removes a PCB from the PCB Free List and returns it.
 *RETURNS: A pcb_PTR to a pcb from the free list or NULL if the free 
 *list is empty
 **********************************************************************/
pcb_PTR allocPcb(){
	
	/*Remove the PCB to return from the free list*/
	pcb_PTR retPcb = removeProcQ(&(pcbList_h));

	if(retPcb != NULL){
		/*Wash the dishes*/
		retPcb->p_next = NULL;
		retPcb->p_prev = NULL;
		retPcb->p_prnt = NULL;
		retPcb->p_child = NULL;
		retPcb->p_nextSib = NULL;
		retPcb->p_prevSib = NULL;
		retPcb->p_time = 0;
		
		retPcb->oldSys = NULL;
		retPcb->newSys = NULL;
		retPcb->oldPrgm = NULL;
		retPcb->newPrgm = NULL;
		retPcb->oldTlb = NULL;
		retPcb->newTlb = NULL;
	}
	
	return retPcb;
}


/***********************************************************************
 *Function that initializes the PCB Free List.
 *RETURNS: N/a
 **********************************************************************/
void initPcbs(){
	
	int i;
	/*Create a static array of PCBs*/
	static pcb_t procTable[MAXPROC];
	
	/*Initialize the head of the free list to NULL*/
	pcbList_h = mkEmptyProcQ();
	
	/*For each index in the array...*/
	for (i = 0; i < MAXPROC; i++){
		/*Add it to the PCB Free List*/
		freePcb(&(procTable[i]));
	}
}


/*********************Process Queue Implementation*********************/

/***********************************************************************
 *Function that creates an initially empty process queue.
 *RETURNS: a pcb_PTR to an empty process queue
 **********************************************************************/
pcb_PTR mkEmptyProcQ(){
	return NULL;
}


/***********************************************************************
 *Function that checks if the given process queue is empty.
 *RETURNS: TRUE if the process queue is empty, FALSE otherwise
 **********************************************************************/
int emptyProcQ(pcb_PTR tp){
	return (tp == NULL);
}


/***********************************************************************
 *Function that inserts an element into the process queue pointed at
 *by the given tail pointer.
 *RETURNS: N/A
 **********************************************************************/
void insertProcQ(pcb_PTR *tp, pcb_PTR p){
	
	/*If the queue is empty...*/
	if (emptyProcQ(*tp)){
		p->p_next = p;
		p->p_prev = p;
	}
	
	/*The queue is not empty*/
	else{
		/*Merge the new element into the list*/
		p->p_next = (*tp)->p_next;
		(*tp)->p_next->p_prev = p;
		(*tp)->p_next = p;
		p->p_prev = *tp;
	}
	
	/*Set the tail pointer to the new node*/
	*tp = p;
}


/***********************************************************************
 *Function that returns the head of the process queue pointed at by the
 *given tail pointer but does not change the tail pointer.
 *RETURNS: a pcb_PTR to the pcb at the head of the queue
 **********************************************************************/
pcb_PTR headProcQ(pcb_PTR tp){
	
	/*If the queue is empty...*/
	if (emptyProcQ(tp)){
		return NULL;
	}
	return tp->p_next;
}


/***********************************************************************
 *Function that removes the element at the head of the process queue
 *pointed at by the given tail pointer and returns it.
 *RETURNS: a pcb_PTR to the pcb removed from the specified process 
 *queue, or NULL if the process queue is empty
 **********************************************************************/
pcb_PTR removeProcQ(pcb_PTR *tp){
	
	/*If the queue is empty*/
	if (emptyProcQ(*tp)){
		/*Return NULL*/
		return NULL;
	}
	return outProcQ(tp, (*tp)->p_next);	
}


/***********************************************************************
 *Function that removes the specified element from the process queue
 *pointed at by the given tail pointer and returns it.
 *RETURNS: a pcb_PTR to the pcb removed from the specified process 
 *queue, NULL if the process queue is empty or NULL if the node was not 
 *in the specified queue
 **********************************************************************/
pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p){

	/*If the queue is empty*/
	if (emptyProcQ(*tp)){
		return NULL;
	}
	/*If the node to remove is the tail pointer...*/
	if (p == *tp){
		if((*tp)->p_next != *tp){
			(*tp)->p_prev->p_next = (*tp)->p_next;
			(*tp)->p_next->p_prev = (*tp)->p_prev;
			*tp = (*tp)->p_prev;
		}
		else{
			*tp = NULL;				
		}
		return p;
	}

	/*The node isn't the tail pointer*/		
	else{
		pcb_PTR retPcb = (*tp)->p_next;
		while (retPcb != *tp){
				
			/*If the pointer points at the node to be removed...*/
			if (retPcb == p){
				/*Unweave the node*/
				retPcb->p_prev->p_next = retPcb->p_next;
				retPcb->p_next->p_prev = retPcb->p_prev;

				retPcb->p_prev = NULL;
				retPcb->p_next = NULL;

				return retPcb;
			}

			retPcb = retPcb->p_next;
		}
		/*Error Case: The node to be removed was not found, return 
		 *NULL*/
		return NULL;
	}
}


/*********************Process Tree Implementation**********************/

/***********************************************************************
 *Function to check if the given PCB has a child.
 *RETURNS: TRUE if the given pcb has no children, FALSE otherwise
 **********************************************************************/
int emptyChild(pcb_PTR p){
	return (p->p_child == NULL);
}


/***********************************************************************
 *Function that inserts a child into the list of children for
 *the specified parent.
 *RETURNS: N/a
 **********************************************************************/
void insertChild(pcb_PTR prnt, pcb_PTR p){
	
	if(emptyChild(prnt)){
		p->p_prevSib = NULL;
	}
	else{
		/*Set the child's next sibling as the new node*/
		prnt->p_child->p_nextSib = p;
		p->p_prevSib = prnt->p_child;
	}

	p->p_nextSib = NULL;

	/*Set new child as our head child*/	
	prnt->p_child = p;
	p->p_prnt = prnt;
}


/***********************************************************************
 *Function that removes the first child of the specified parent.
 *RETURNS: a pcb_PTR to the pcb removed from the specified parent's list
 *of children or NULL if there are no children
 **********************************************************************/
pcb_PTR removeChild(pcb_PTR prnt){

	pcb_PTR retPcb = NULL;
	/*If the parent has no child...*/
	if (emptyChild(prnt)){
		return NULL;
	}

	retPcb =  prnt->p_child;

	if (retPcb->p_prevSib == NULL){
		
		/*Remove the node*/
		retPcb->p_prnt = NULL;
		prnt->p_child = NULL;

		return retPcb;
	}
	/*Set the parent's child the current child's previous sibling*/
	prnt->p_child = retPcb->p_prevSib;

	retPcb->p_prevSib->p_nextSib = NULL;
	retPcb->p_prevSib = NULL;
	retPcb->p_prnt = NULL;
	return retPcb;
}


/***********************************************************************
 *Function that removes the specified child from its parent and
 *siblings.
 *RETURNS: a pcb_PTR to the pcb removed from it's parent and siblings or
 *NULL if it is not a child of any pcb
 **********************************************************************/
pcb_PTR outChild(pcb_PTR p){
	
	/*If the node has no parent...*/
	if (p->p_prnt == NULL){
		return NULL;
	}
	/*The node has a parent*/
	else{
		/*If the node is the child of the parent...*/
		if (p == p->p_prnt->p_child){
			return removeChild(p->p_prnt);
		}
		/*The node is not the child of the parent*/
		else{
			/*If the node has no previous sibling...*/
			if (p->p_prevSib == NULL){
				p->p_nextSib->p_prevSib = NULL;

			}
			/*The node has a previous sibling*/
			else{
				/*Unweave the node*/
				p->p_nextSib->p_prevSib = p->p_prevSib;
				p->p_prevSib->p_nextSib = p->p_nextSib;
				p->p_prevSib = NULL;
			}
			p->p_nextSib = NULL;
			p->p_prnt = NULL;
			return p;
		}
	}
}
