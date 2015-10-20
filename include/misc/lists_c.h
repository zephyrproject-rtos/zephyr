/* lists_c.h */

/*
 * Copyright (c) 1997-2012 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LISTS_C_H
#define _LISTS_C_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Example code from list insertion etc
 ******************************************************************************/

#include <misc/lists.h>

INLINE void InitList(struct list_head *list)
{
	list->head = (struct list_elem *)&list->tail;
	list->TailPrev = (struct list_elem *)&list->head;
	list->tail = NULL;
}

INLINE unsigned int TestListEmpty(struct list_head *list)
{
	return (list->TailPrev == (struct list_elem *)list);
}

INLINE void RemoveElem(struct list_elem *elem)
{
	struct list_elem *tmpElem;

	tmpElem = elem->next;
	elem = elem->prev;  /* elem is destroyed */
	elem->next = tmpElem;
	tmpElem->prev = elem;
}

INLINE struct list_elem *RemoveHead(struct list_head *list)
{
	struct list_elem *tmpElem;
	struct list_elem *ret_Elem;

	ret_Elem = list->head;
	tmpElem = ret_Elem->next;
	if (tmpElem == NULL) {
		return NULL;  /* empty list */
	}
	list->head = tmpElem;
	tmpElem->prev = (struct list_elem *)&list->head;
	return ret_Elem;
}

INLINE struct list_elem *RemoveTail(struct list_head *list)
{
	struct list_elem *tmpElem, *ret_Elem;

	ret_Elem = list->TailPrev;
	tmpElem = ret_Elem->prev;
	if (tmpElem == NULL) {
		return NULL;  /* empty list */
	}
	list->TailPrev = tmpElem;
	tmpElem->next = (struct list_elem *)&list->tail;
	return ret_Elem;
}

INLINE void AddHead(struct list_head *list, struct list_elem *elem)
{
	struct list_elem *tmpElem;

	tmpElem = list->head;
	list->head = elem;
	tmpElem->prev = elem;
	/* set pointers for the new elem */
	elem->next = tmpElem;
	elem->prev = (struct list_elem *)&list->head;
}

INLINE void AddTail(struct list_head *list, struct list_elem *elem)
{
	struct list_elem *tmpElem;
	struct list_elem *tailElem;

	tmpElem = (struct list_elem *)&list->tail;
	tailElem = tmpElem->prev;
	tailElem->next = elem;
	tmpElem->prev = elem;
	/* set pointers for the new elem */
	elem->next = tmpElem;
	elem->prev = tailElem;
}

/* Insert after predecessor elem */

INLINE void Insert_Elem(struct list_elem *elem, struct list_elem *pred)
{
	struct list_elem *tmpElem;

	tmpElem = pred->next;
	pred->next = elem;
	tmpElem->prev = elem;
	elem->next = tmpElem;
	elem->prev = pred;
}

/* Enqueue in list priority based, after last elem with equal or higher prior */
/* 0 is highest priority */

INLINE void InsertPrio(struct list_head *list, struct list_elem *newelem)
{
	struct list_elem *tmpElem;

	tmpElem = (struct list_elem *)&list->head;
	while ((tmpElem->next != (struct list_elem *)&list->tail) && /* end of list */
	       (tmpElem->priority <= newelem->priority)) {
		tmpElem = tmpElem->next;
	}
	Insert_Elem(newelem, tmpElem);
}

#ifdef __cplusplus
}
#endif

#endif /* _LISTS_C_H */
