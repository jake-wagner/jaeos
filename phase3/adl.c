/***********************************************************************
* ADL.C
* 
* This file creates and maintains the Active Delay Daemon for the JAEOS
* operating system.
* 
* The Active Delay Daemon assists with process that are to be put to 
* sleep for a specified amount of time. The ADL is a linear singly
* linked list that is maintained in increasing order based on the wake
* time of the node.
* 
* This interface has mutator methods to insert a delay descriptor node
* into it's proper location in the list and to remove the first delay
* descriptor node from the list. It has an accessor method to get the
* wake time field from the head descriptor node in the list.
*
* Written by Jake Wagner
* Last Updated: 11-1-16
***********************************************************************/

#include "../h/const.h"
#include "../h/types.h"

/***********************Global Definitions*****************************/

/*Definition of a delay node*/
typedef struct delayd_t {
	
	struct delayd_t		*d_next;
	int					d_wakeTime;
	int					d_procID;
	
} delayd_t;

/*The pointer to the head of the Delay Daemon Free List*/
HIDDEN delayd_t *delaydFree_h;

/*The pointer to the head of the Active Delay Daemon List*/
HIDDEN delayd_t *activeDelaydList_h;

/*************************Helper Functions*****************************/

/***********************************************************************
 *Function that returns the delay node previous to the delay node with
 *the specified wake time.
 *RETURNS: a pointer to a delay node
 **********************************************************************/
HIDDEN delayd_t *findPreviousDelayd(int wakeTime){
	
	delayd_t *current = activeDelaydList_h;
	
	/*While it hasn't been found or it hasn't reached the end...*/
	while((current->d_next != NULL) 
				&& (current->d_next->d_wakeTime < wakeTime)){
					
		current = current->d_next;
	}
	return current;
}


/****************Delay Daemon Free List Implementation*****************/

/***********************************************************************
 *Function that adds a delay node that is no longer in use to the Delay 
 *Daemon Free List.
 *RETURNS: N/a
 **********************************************************************/
HIDDEN void freeDelayd(delayd_t* delayd){
	
	/*If the free list is empty...*/
	if(delaydFree_h == NULL){
		delaydFree_h = delayd;
		delaydFree_h->d_next = NULL;
	}
	else{
		delayd->d_next = delaydFree_h;
		delaydFree_h = delayd;
	}
}

/***********************************************************************
 *Function that removes a delay node from the Delay Daemon Free List and
 *returns it.
 *RETURNS: a pointer to a delay daemon node taken from the free list or 
 *NULL if the free list is empty
 **********************************************************************/
HIDDEN delayd_t* allocDelayd(){
	
	delayd_t* retDelayd;
	
	/*If the head of the list is NULL...*/
	if(delaydFree_h == NULL){
		return NULL;
	}
	else{
		retDelayd = delaydFree_h;
		
		/*If the head is the only node in the list...*/
		if(delaydFree_h->d_next == NULL){
			delaydFree_h = NULL;
		}
		else{
			delaydFree_h = delaydFree_h->d_next;
			retDelayd->d_next = NULL;
		}
		/*Wash the dishes*/
		retDelayd->d_next = NULL;
		retDelayd->d_wakeTime = -1;
		retDelayd->d_procID = -1;
	}
	return retDelayd;
}

/***********************************************************************
 *Function that initializes the Active Delay Daemon data structures.
 *RETURNS: N/a
 **********************************************************************/
void initADL(){
	
	int i;
	/*Create a static array of delay daemon nodes*/
	static delayd_t delaydTable[(MAXUSERPROC + 1)];
	
	delaydFree_h = NULL;
	activeDelaydList_h = NULL;
	
	for (i = 0; i < (MAXUSERPROC +1); i++){
		/*Add it to the Delay Daemon Free List*/
		freeDelayd(&(delaydTable[i]));
	}
}

/***************Active Delay Daemon List Implementation****************/

/***********************************************************************
 *Function that checks to see the wake time of the first node in the
 *list.
 *RETURNS: the wakeTime of the head of the list.
 **********************************************************************/
int headDelaydTime(){
	if(activeDelaydList_h == NULL){
		return FAILURE;
	}
	return activeDelaydList_h->d_wakeTime;
}

/***********************************************************************
 *Function inserts a delay node into the Active Delay Daemon List. It
 *inserts it into the appropriate location based on its waketime.
 *RETURNS: if a node was successfully place into the list.
 **********************************************************************/
int insertDelay(int wakeTime, int procID){
	
	delayd_t* currentDelay = NULL;
	
	/*Get a delay daemon node from the free list*/
	delayd_t *newDelay = allocDelayd();
	
	/*If the free list was empty...*/
	if(newDelay == NULL){
		return FALSE;
	}
	
	newDelay->d_wakeTime = wakeTime;
	newDelay->d_procID = procID;
		
	/*If the head of the active list is null...*/
	if(activeDelaydList_h == NULL){
		
		/*Set the head to the new node*/
		activeDelaydList_h = newDelay;
		newDelay->d_next = NULL;
		
		return TRUE;
	}
	
	currentDelay = findPreviousDelayd(wakeTime);
	
	/*Weave in the new node*/
	newDelay->d_next = currentDelay->d_next;
	currentDelay->d_next = newDelay;
	
	return TRUE;
}

/***********************************************************************
 *Function that removes a delay node from the Active Delay Daemon List
 *and returns it to the Delay Daemon Free List if its wake time has 
 *passed.
 *RETURNS: the process ID of the removed node or -1 if no node was 
 *removed.
 **********************************************************************/
int removeDelay(){
	
	int retProcID;
	
	/*Set a pointer to the head of the list*/
	delayd_t* tempDelay = activeDelaydList_h;
	
	/*If the node is not null...*/
	if(tempDelay != NULL){
		
		/*Move the head to the next node*/
		activeDelaydList_h = tempDelay->d_next;
		
		/*Save process ID and cleanup*/
		retProcID = tempDelay->d_procID;
		tempDelay->d_next = NULL;
		freeDelayd(tempDelay);
		
		return retProcID;
	}
	
	/*There was no list, failure*/
	return FAILURE;
}
