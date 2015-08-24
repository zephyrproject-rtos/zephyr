/* lists.h */

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

#ifndef _LISTS_H
#define _LISTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <microkernel.h>

struct Elem {
	struct Elem *next;
	struct Elem *prev;
	unsigned int priority;
};



struct List {
	struct list_elem *head;
	struct list_elem *tail;
	struct list_elem *TailPrev;
};


#ifndef INLINED
extern void InitList(struct list_head *list);
extern unsigned int TestListEmpty(struct list_head *list);
extern void RemoveElem(struct list_elem *elem);
extern struct list_elem *RemoveHead(struct list_head *list);
extern struct list_elem *RemoveTail(struct list_head *list);
extern void AddHead(struct list_head *list, struct list_elem *elem);
extern void AddTail(struct list_head *list, struct list_elem *elem);
extern void Insert_Elem(struct list_elem *elem, struct list_elem *pred);
extern void InsertPrio(struct list_head *list, struct list_elem *newelem);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _LISTS_H */
