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
* Assignment1 - Phase 3 - list_movers.c
* List source for functions that move items in lists
*/

#include "list.h"

#include <stdlib.h>
#include <stdio.h>

/* returns the number of items in list.*/
int ListCount(LIST* list)
{
	if(list == NULL)
	{
		printf("ERROR: List was NULL.\n");
		return 0;
	}

	return list->size;
}

/*returns a pointer to the first item in list and makes the first item the current item.*/
void* ListFirst(LIST* list)
{
	if(list == NULL)
	{
		printf("ERROR: List was NULL.\n");
		return NULL;
	}
	if(list->size == 0)
	{
		return NULL;
	}
	
	list->cur = list->head;

	return list->cur->item;
}

/*returns a pointer to the last item in list and makes the last item the current one.*/
void* ListLast(LIST* list)
{
	if(list == NULL)
	{
		printf("ERROR: List was NULL.\n");
		return NULL;
	}
	if(list->size == 0)
	{
		return NULL;
	}

	list->cur = list->tail;

	return list->cur->item;
}

/* advances list's current item by one, and returns a pointer to the new current item.
* If this operation attempts to advances the current item beyond the end of the list, a NULL pointer is returned.*/
void* ListNext(LIST* list)
{
	if(list == NULL)
	{
		printf("ERROR: List was NULL.\n");
		return NULL;
	}
	if(list->size == 0)
	{
		return NULL;
	}	
	if(list->cur->next == NULL)
	{
		return NULL;
	}
	list->cur = list->cur->next;

	return list->cur->item;
}

/* backs up list's current item by one, and returns a pointer to the new current item. 
*If this operation attempts to back up the current item beyond the start of the list, a NULL pointer is returned.*/
void* ListPrev(LIST* list)
{
	if(list == NULL)
	{
		printf("ERROR: List was NULL.\n");
		return NULL;
	}
	if(list->size == 0)
	{
		return NULL;
	}	
	if(list->cur->prev == NULL)
	{
		return NULL;
	}
	list->cur = list->cur->prev;

	return list->cur->item;
}

/* void *ListCurr(list) returns a pointer to the current item in list.*/
void* ListCurr(LIST* list)
{
	if(list == NULL)
	{
		printf("ERROR: List was NULL.\n");
		return NULL;
	}
	if(list->size == 0)
	{
		return NULL;
	}
		
	return list->cur->item;
}

/*adds list2 to the end of list1. The current pointer is set to the current pointer of list1. 
* List2 no longer exists after the operation.*/
void ListConcat(LIST* list1, LIST* list2)
{
	if(list1 == NULL)
	{
		printf("ERROR: List1 was NULL.\n");
	}
	if(list2 == NULL)
	{
		printf("ERROR: List2 was NULL.\n");
	}
	if(list1->size == 0)
	{
		list1 = list2;
		return;
	}
	if(list2->size == 0)
	{
		return;
	}
	
	
	
	list1->tail->next = list2->head;
	list2->head->prev = list1->tail;
	list1->tail = list2->tail;
	list1->size += list2->size;

	list2->head = NULL;
	list2->cur = NULL;
	list2->tail = NULL;
	listPosition--;
	listArray[listPosition] = list2;	
	
	return;
}

/*searches list starting at the current item until the end is reached or a match is found. 
* In this context, a match is determined by the comparator parameter. This parameter is a 
* pointer to a routine that takes as its first argument an item pointer, and as its second
* argument comparisonArg. Comparator returns 0 if the item and comparisonArg don't match 
* (i.e. didn't find it), or 1 if they do (i.e. found it). Exactly what constitutes a match 
* is up to the implementor of comparator. If a match is found, the current pointer is left 
* at the matched item and the pointer to that item is returned. If no match is found, the 
* current pointer is left at the end of the list and a NULL pointer is returned. */
void *ListSearch(LIST* list, int (*comparator)(void *, void *), void* comparisonArg)
{
	if(list == NULL)
	{
		printf("ERROR: List was NULL.\n");
		return NULL;
	}
	if(comparisonArg == NULL)
	{
		printf("ERROR: ComparisonArg was NULL.\n");
		return NULL;
	}

	for(list->cur = list->head; list->cur != NULL; list->cur = list->cur->next)
	{
		if((*comparator)(list->cur->item, comparisonArg))
		{
			return list->cur->item;
		}
	}
	return NULL;
}
