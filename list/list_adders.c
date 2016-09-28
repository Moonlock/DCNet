/*group27
* Joel Farthing
* jff638
* 11064798
*
* Angela Stankowski
* ans723
* 11119985
*
* CMPT332 
* Assignment2 - Phase 1 - list_adders.c
* List source for functions that add to lists
*/

#include "list.h"

#include <stdlib.h>
#include <stdio.h>


/* makes a new, empty list, and returns its reference on success.
* Returns a NULL pointer on failure.*/
LIST *ListCreate()
{
	LIST* newList;
	LIST* lists;
	NODE* nodes;
	int i;

	if(listArray[0] == NULL)
	{
		lists = (LIST*)malloc(NUMBER_OF_NODES*sizeof(LIST));
		nodes = (NODE*)malloc(NUMBER_OF_NODES*sizeof(NODE));

		listPosition = 0;
		nodePosition = 0;


		for (i = 0; i < NUMBER_OF_NODES; i++)
		{
			nodeArray[i] = &nodes[i];
		}
		for (i = 0; i < NUMBER_OF_LISTS; i++)
		{
			listArray[i] = &lists[i];
		}
	}	
	if(listPosition >= NUMBER_OF_LISTS)
	{
		printf("ERROR: No free lists.\n");
		return NULL;
	}
	
	newList = listArray[listPosition];
	newList->head = NULL;
	newList->tail = NULL;
	newList->cur = NULL;
	newList->size = 0;
	listPosition++;
	
	return newList;
}

/*adds the new item to list directly after the current item, and makes item the current item.
* If the current pointer is at the end of the list, the item is added at the end. Returns 0 on success, -1 on failure.*/
int ListAdd(LIST* list, void* item)
{
	NODE* newNode;

	if(list == NULL)
	{
		printf("ERROR: List was NULL.\n");
		return -1;
	}
	if(nodePosition >= NUMBER_OF_NODES)
	{
		printf("ERROR: No free nodes.\n");
		return -1;
	}
	
	
	newNode = nodeArray[nodePosition];
	newNode->item = item;
	nodePosition++;
	
	if (list->size == 0)
	{
		list->head = newNode;
		list->tail = newNode;
		list->cur = newNode;	

		list->size++;
		return 0;
	}
	else if (list->cur == list->tail)	/* Current pointer at end of list. */ 
	{
		list->tail->next = newNode;	
		newNode->prev = list->tail;
		list->tail = newNode;
		list->cur = newNode;
	
		list->size++;		
		return 0;
	}

	list->cur->next->prev = newNode;
	newNode->next = list->cur->next;
	list->cur->next = newNode;
	newNode->prev = list->cur;
	list->cur = newNode;
	
	list->size++;
	
	return 0;
}

/*adds item to list directly before the current item, and makes the new item the current one.
* If the current pointer is at the start of the list, the item is added at the start.  
* Returns 0 on success, -1 on failure.*/
int ListInsert(LIST* list, void* item)
{
	NODE* newNode;
	
	if(list == NULL)
	{
		printf("ERROR: List was NULL.\n");
		return -1;

	}
	if(nodePosition >= NUMBER_OF_NODES)
	{
		printf("ERROR: No free nodes.\n");
		return -1;
	}


	newNode = nodeArray[nodePosition];
	newNode->item = item;
	nodePosition++;

	if (list->size == 0)
	{
		list->head = newNode;
		list->tail = newNode;
		list->cur = newNode;	

		list->size++;
		return 0;
	}
	else if (list->cur == list->head)	/* Current pointer at start of list. */ 
	{
		list->head->prev = newNode;	
		newNode->next = list->head;
		list->head = newNode;
		list->cur = newNode;
		
		list->size++;
		return 0;
	}
	list->cur->prev->next = newNode;
	newNode->prev = list->cur->prev;
	list->cur->prev = newNode;
	newNode->next = list->cur;
	list->cur = newNode;

	list->size++;

	return 0;
}

/*adds item to the end of list, and makes the new item the current one. 
* Returns 0 on success, -1 on failure.*/
int ListAppend(LIST* list, void* item)
{
	NODE* newNode;
	
	if(list == NULL)
	{
		printf("ERROR: List was NULL.\n");
		return -1;

	}	
	if(nodePosition >= NUMBER_OF_NODES)
	{
		printf("ERROR: No free nodes.\n");
		return -1;
	}


	newNode = nodeArray[nodePosition];
	newNode->item = item;
	nodePosition++;

	if (list->size == 0)
	{
		list->head = newNode;
		list->tail = newNode;
		list->cur = newNode;	

		list->size++;
		return 0;
	}

	list->tail->next = newNode;	
	newNode->prev = list->tail;
	list->tail = newNode;
	list->cur = newNode;
	
	list->size++;

	return 0;
}

/*adds item to the front of list, and makes the new item the current one. 
* Returns 0 on success, -1 on failure.*/
int ListPrepend(LIST* list, void* item)
{
	NODE* newNode;
	
	if(list == NULL)
	{
		printf("ERROR: List was NULL.\n");
		return -1;

	}	
	if(nodePosition >= NUMBER_OF_NODES)
	{
		printf("ERROR: No free nodes.\n");
		return -1;
	}
	

	newNode = nodeArray[nodePosition];
	newNode->item = item;
	nodePosition++;

	if (list->size == 0)
	{
		list->head = newNode;
		list->tail = newNode;
		list->cur = newNode;	

		list->size++;
		return 0;
	}
	
	list->head->prev = newNode;	
	newNode->next = list->head;
	list->head = newNode;
	list->cur = newNode;
		
	list->size++;

	return 0;
}
