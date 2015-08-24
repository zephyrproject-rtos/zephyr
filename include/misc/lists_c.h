/* lists_c.h */

/*
 * Copyright (c) 1997-2012 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LISTS_C_H
#define _LISTS_C_H

#ifdef __cplusplus
extern "C" {
#endif

/**
Example code from list insertion etc
 *******************************************************************************/

#include <misc/lists.h>

INLINE void InitList(struct list_head *list)
{
	list->Head = (struct list_elem *)&list->Tail;
	list->TailPrev = (struct list_elem *)&list->Head;
	list->Tail = NULL;
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

	ret_Elem = list->Head;
	tmpElem = ret_Elem->next;
	if (tmpElem == NULL) {
		return NULL;  /* empty list */
	}
	list->Head = tmpElem;
	tmpElem->prev = (struct list_elem *)&list->Head;
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
	tmpElem->next = (struct list_elem *)&list->Tail;
	return ret_Elem;
}

INLINE void AddHead(struct list_head *list, struct list_elem *elem)
{
	struct list_elem *tmpElem;

	tmpElem = list->Head;
	list->Head = elem;
	tmpElem->prev = elem;
	/* set pointers for the new elem */
	elem->next = tmpElem;
	elem->prev = (struct list_elem *)&list->Head;
}

INLINE void AddTail(struct list_head *list, struct list_elem *elem)
{
	struct list_elem *tmpElem;
	struct list_elem *tailElem;

	tmpElem = (struct list_elem *)&list->Tail;
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

	tmpElem = (struct list_elem *)&list->Head;
	while ((tmpElem->next != (struct list_elem *)&list->Tail) && /* end of list */
	       (tmpElem->priority <= newelem->priority)) {
		tmpElem = tmpElem->next;
	}
	Insert_Elem(newelem, tmpElem);
}

#ifdef __cplusplus
}
#endif

#endif /* _LISTS_C_H */
