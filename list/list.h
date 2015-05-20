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
* Assignment2 - Phase 1 - list.h
* Header file containing prototypes and structure defs
* for NODE and LIST
*/

#ifndef LIST_H
#define LIST_H
#define NUMBER_OF_NODES 1024
#define NUMBER_OF_LISTS 128

/*structure definitions for node and List*/
typedef struct node NODE;
typedef struct list LIST;

struct node {
	NODE* prev;
	void* item;
	NODE* next;
};

struct list {
	NODE* head;
	NODE* tail;
	NODE* cur;
	int size;
};

NODE* nodeArray[NUMBER_OF_NODES];
LIST* listArray[NUMBER_OF_LISTS];
int nodePosition;
int listPosition;



/* Initializes list and node arrays. */
/*void static Init(void);
*/
/* Adds item to list.  List must be empty. */
/*void static AddToEmpty(LIST* list, void* item);
*/
/* makes a new, empty list, and returns its reference on success.
* Returns a NULL pointer on failure.*/
LIST *ListCreate();

/*returns the number of items in list.*/
int ListCount(LIST* list);

/*returns a pointer to the first item in list and makes the first item the current item.*/
void* ListFirst(LIST* list);

/*returns a pointer to the last item in list and makes the last item the current one.*/
void* ListLast(LIST* list);

/* advances list's current item by one, and returns a pointer to the new current item.
* If this operation attempts to advances the current item beyond the end of the list, a NULL pointer is returned.*/
void* ListNext(LIST* list);

/*backs up list's current item by one, and returns a pointer to the new current item. 
*If this operation attempts to back up the current item beyond the start of the list, a NULL pointer is returned.*/
void* ListPrev(LIST* list);

/* void *ListCurr(list) returns a pointer to the current item in list.*/
void* ListCurr(LIST* list);

/*adds the new item to list directly after the current item, and makes item the current item.
* If the current pointer is at the end of the list, the item is added at the end. Returns 0 on success, -1 on failure.*/
int ListAdd(LIST* list, void* item);

/*adds item to list directly before the current item, and makes the new item the current one.
* If the current pointer is at the start of the list, the item is added at the start.  
* Returns 0 on success, -1 on failure.*/
int ListInsert(LIST* list, void* item);

/*adds item to the end of list, and makes the new item the current one. 
* Returns 0 on success, -1 on failure.*/
int ListAppend(LIST* list, void* item);

/*adds item to the front of list, and makes the new item the current one. 
* Returns 0 on success, -1 on failure.*/
int ListPrepend(LIST* list, void* item);

/*Return current item and take it out of list. 
* Make the next item the current one.*/
void *ListRemove(LIST* list);

/*adds list2 to the end of list1. The current pointer is set to the current pointer of list1. 
* List2 no longer exists after the operation.*/
void ListConcat(LIST* list1, LIST* list2);

/*delete list. itemFree is a pointer to a routine that frees an item. 
* It should be invoked (within ListFree) as: (*itemFree)(itemToBeFreed)*/
void ListFree(LIST* list, void (*itemFree)(void*));

/*Return last item and take it out of list. Make the new last item the current one.*/
void *ListTrim(LIST* list);

/*searches list starting at the current item until the end is reached or a match is found. 
* In this context, a match is determined by the comparator parameter. This parameter is a 
* pointer to a routine that takes as its first argument an item pointer, and as its second
* argument comparisonArg. Comparator returns 0 if the item and comparisonArg don't match 
* (i.e. didn't find it), or 1 if they do (i.e. found it). Exactly what constitutes a match 
* is up to the implementor of comparator. If a match is found, the current pointer is left 
* at the matched item and the pointer to that item is returned. If no match is found, the 
* current pointer is left at the end of the list and a NULL pointer is returned. */
void *ListSearch(LIST* list, int (*comparator)(void *, void *), void* comparisonArg);

/* Return the number of times item appears in list. */
int ListCountItem(LIST* list, int (*comparator)(void *, void *), void* item);

/* Return the item at index.  Make this item the current item. If no item exists at the 
 * given index, the current item is set to the last item. */
void* ListGetByIndex(LIST* list, int index);

/* Splits list into two lists.  The current item becomes the first item in the new list.  
 * The current item of the new list is set to the first item in the list, and the current item 
 * of the old list is set to the last item in that list. */
LIST* ListSplit(LIST* list);

#endif /*LIST_H*/
