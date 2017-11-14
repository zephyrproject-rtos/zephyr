#ifndef __ALT_LIST_H__
#define __ALT_LIST_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/

/******************************************************************************
*                                                                             *
* THIS IS A LIBRARY READ-ONLY SOURCE FILE. DO NOT EDIT.                       *
*                                                                             *
******************************************************************************/

#include "alt_types.h"

/*
 * alt_llist.h defines structures and functions for use in manipulating linked
 * lists. A list is considered to be constructed from a chain of objects of
 * type alt_llist, with one object being defined to be the head element. 
 *
 * A list is considered to be empty if it only contains the head element.
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * alt_llist is the structure used to represent an element within a linked
 * list.
 */

typedef struct alt_llist_s alt_llist;

struct alt_llist_s {
  alt_llist* next;     /* Pointer to the next element in the list. */
  alt_llist* previous; /* Pointer to the previous element in the list. */
};

/*
 * ALT_LLIST_HEAD is a macro that can be used to create the head of a new 
 * linked list. This is named "head". The head element is initialised to 
 * represent an empty list.  
 */

#define ALT_LLIST_HEAD(head) alt_llist head = {&head, &head}

/*
 * ALT_LLIST_ENTRY is a macro used to define an uninitialised linked list
 * entry. This is used to reserve space in structure initialisation for
 * structures that inherit form alt_llist.
 */

#define ALT_LLIST_ENTRY {0, 0}

/*
 * alt_llist_insert() insert adds the linked list entry "entry" as the 
 * first entry in the linked list "list". "list" is the list head element.  
 */

static ALT_INLINE void ALT_ALWAYS_INLINE alt_llist_insert(alt_llist* list, 
                alt_llist* entry)
{
  entry->previous = list;
  entry->next     = list->next;

  list->next->previous = entry;
  list->next           = entry;
}

/*
 * alt_llist_remove() is called to remove an element from a linked list. The
 * input argument is the element to remove.
 */
     
static ALT_INLINE void ALT_ALWAYS_INLINE alt_llist_remove(alt_llist* entry)
{
  entry->next->previous = entry->previous;
  entry->previous->next = entry->next;

  /* 
   * Set the entry to point to itself, so that any further calls to
   * alt_llist_remove() are harmless.
   */

  entry->previous = entry;
  entry->next     = entry;
} 

#ifdef __cplusplus
}
#endif
 
#endif /* __ALT_LLIST_H__ */ 
