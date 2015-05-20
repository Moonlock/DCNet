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
* Assignment1 - Phase 3 - list_removers.c
* List source for functions that remove items from lists
*/

#include "list.h"

#include <stdlib.h>
#include <stdio.h>

/*Return current item and take it out of list. 
* Make the next item the current one.*/
void *ListRemove(LIST* list)
{
	void* item;
	NODE* temp;

	if(list == NULL)
	{
		printf("Error: List was NULL.\n");
		return NULL;
	}
	if(list->size == 0)/*list->cur == NULL)*/
	{
		printf("ERROR: List was empty.");
		return NULL;
	}

	item = list->cur->item;
	temp = list->cur;

	if(list->cur == list->head)
	{
		list->head = list->head->next;
	}
	else
	{
		list->cur->prev->next = list->cur->next;
	}

	if(list->cur == list->tail)
	{
		list->tail = list->tail->prev;
		list->cur = list->cur->prev;
	}
	else
	{
		list->cur->next->prev = list->cur->prev;
		list->cur = list->cur->next;
	}

	temp->next = NULL;
	temp->prev = NULL;

	list->size--;
	nodePosition--;
	nodeArray[nodePosition] = temp;

	return item;
}

/*delete list. itemFree is a pointer to a routine that frees an item. 
* It should be invoked (within ListFree) as: (*itemFree)(itemToBeFreed)*/
void ListFree(LIST* list, void (*itemFree)(void*))
{
	NODE* temp;

	if(list == NULL)
	{
		printf("ERROR: List was NULL.");
	}

	list->cur = list->head;
	for(temp = list->head; temp != NULL; temp = list->cur)
	{
		list->cur = list->cur->next;
		(*itemFree)(temp->item);
		temp->prev = NULL;
		temp->next = NULL;
		
		nodePosition--;
		nodeArray[nodePosition] = temp;
	}
	
	list->size = 0;
	list->head = NULL;
	list->cur = NULL;
	list->tail = NULL;
	listPosition--;
	listArray[listPosition] = list;	
	
	return;
}

/*Return last item and take it out of list. Make the new last item the current one.*/
void *ListTrim(LIST* list)
{
	void* item;

	if(list == NULL)
	{
		printf("ERROR: List was NULL.");
		return NULL;
	}
	if(list->cur == NULL)
	{
		printf("ERROR: List was empty.");
		return NULL;
	}
	
	item = list->tail->item;

	if(list->head == list->tail)
	{
		list->head = NULL;
	}
	
	list->size--;
	nodePosition--;
	nodeArray[nodePosition] = list->tail;

	list->cur = list->tail->prev;
	list->tail->next = NULL;
	list->tail->prev = NULL;
	list->tail = list->cur;
	
	return item;
}
