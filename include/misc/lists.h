/* lists.h */

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
