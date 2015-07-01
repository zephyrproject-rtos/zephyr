/* dlist.h - doubly-linked list inline implementation */

/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
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

/*
DESCRIPTION
Doubly-linked list implementation.

The lists are expected to be initialized such that both the head and tail
pointers point to the list itself.  Initializing the lists in such a fashion
simplifies the adding and removing of nodes to/from the list.
 */

#ifndef _misc_dlist__h_
#define _misc_dlist__h_

#include <stddef.h>

struct _dnode {
	union {
		struct _dnode *head; /* ptr to head of list (sys_dlist_t) */
		struct _dnode *next; /* ptr to next node    (sys_dnode_t) */
	};
	union {
		struct _dnode *tail; /* ptr to tail of list (sys_dlist_t) */
		struct _dnode *prev; /* ptr to previous node (sys_dnode_t) */
	};
};

typedef struct _dnode sys_dlist_t;
typedef struct _dnode sys_dnode_t;

/**
 * @brief initialize list
 *
 * @param list the doubly-linked list
 *
 * @return None
 */

static inline void sys_dlist_init(sys_dlist_t *list)
{
	list->head = (sys_dnode_t *)list;
	list->tail = (sys_dnode_t *)list;
}

/**
 * @brief check if a node is the list's head
 *
 * @param list the doubly-linked list to operate on
 * @param node the node to check
 *
 * @return 1 if node is the head, 0 otherwise
 */

static inline int sys_dlist_is_head(sys_dlist_t *list, sys_dnode_t *node)
{
	return list->head == node;
}

/**
 * @brief check if a node is the list's tail
 *
 * @param list the doubly-linked list to operate on
 * @param node the node to check
 *
 * @return 1 if node is the tail, 0 otherwise
 */

static inline int sys_dlist_is_tail(sys_dlist_t *list, sys_dnode_t *node)
{
	return list->tail == node;
}

/**
 * @brief check if the list is empty
 *
 * @param list the doubly-linked list to operate on
 *
 * @return 1 if empty, 0 otherwise
 */

static inline int sys_dlist_is_empty(sys_dlist_t *list)
{
	return list->head == list;
}

/**
 * @brief get a reference to the head item in the list
 *
 * @param list the doubly-linked list to operate on
 *
 * @return a pointer to the head element from a node, NULL if list is empty
 */

static inline sys_dnode_t *sys_dlist_peek_head(sys_dlist_t *list)
{
	return sys_dlist_is_empty(list) ? NULL : list->head;
}

/**
 * @brief get a reference to the next item in the list
 *
 * @param list the doubly-linked list to operate on
 * @param node the node from which to get the next element in the list
 *
 * @return a pointer to the next element from a node, NULL if node is the tail
 */

static inline sys_dnode_t *sys_dlist_peek_next(sys_dlist_t *list,
												sys_dnode_t *node)
{
	return node == list->tail ? NULL : node->next;
}

/**
 * @brief add node to tail of list
 *
 * @param list the doubly-linked list to operate on
 * @param node the element to append
 *
 * @return None
 */

static inline void sys_dlist_append(sys_dlist_t *list, sys_dnode_t *node)
{
	node->next = list;
	node->prev = list->tail;

	list->tail->next = node;
	list->tail = node;
}

/**
 * @brief add node to head of list
 *
 * @param list the doubly-linked list to operate on
 * @param node the element to append
 *
 * @return None
 */

static inline void sys_dlist_prepend(sys_dlist_t *list, sys_dnode_t *node)
{
	node->next = list->head;
	node->prev = list;

	list->head->prev = node;
	list->head = node;
}

/**
 * @brief insert node after a node
 *
 * Insert a node after a specified node in a list.
 *
 * @param list the doubly-linked list to operate on
 * @param insert_point the insert point in the list: if NULL, insert at head
 * @param node the element to append
 *
 * @return None
 */

static inline void sys_dlist_insert_after(sys_dlist_t *list,
	sys_dnode_t *insert_point, sys_dnode_t *node)
{
	if (!insert_point) {
		sys_dlist_prepend(list, node);
	} else {
		node->next = insert_point->next;
		node->prev = insert_point;
		insert_point->next->prev = node;
		insert_point->next = node;
	}
}

/**
 * @brief insert node before a node
 *
 * Insert a node before a specified node in a list.
 *
 * @param list the doubly-linked list to operate on
 * @param insert_point the insert point in the list: if NULL, insert at tail
 * @param node the element to insert
 *
 * @return None
 */

static inline void sys_dlist_insert_before(sys_dlist_t *list,
	sys_dnode_t *insert_point, sys_dnode_t *node)
{
	if (!insert_point) {
		sys_dlist_append(list, node);
	} else {
		node->prev = insert_point->prev;
		node->next = insert_point;
		insert_point->prev->next = node;
		insert_point->prev = node;
	}
}

/**
 * @brief insert node at position
 *
 * Insert a node in a location depending on a external condition. The cond()
 * function checks if the node is to be inserted _before_ the current node
 * against which it is checked.
 *
 * @param list the doubly-linked list to operate on
 * @param node the element to insert
 * @param cond a function that determines if the current node is the correct
 *             insert point
 * @param data parameter to cond()
 *
 * @return None
 */

static inline void sys_dlist_insert_at(sys_dlist_t *list, sys_dnode_t *node,
	int (*cond)(sys_dnode_t *, void *), void *data)
{
	if (sys_dlist_is_empty(list)) {
		sys_dlist_append(list, node);
	} else {
		sys_dnode_t *pos = sys_dlist_peek_head(list);

		while (pos && !cond(pos, data)) {
			pos = sys_dlist_peek_next(list, pos);
		}
		sys_dlist_insert_before(list, pos, node);
	}
}

/**
 * @brief remove a specific node from a list
 *
 * The list is implicit from the node. The node must be part of a list.
 *
 * @param node the node to remove
 *
 * @return None
 */

static inline void sys_dlist_remove(sys_dnode_t *node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
}

/**
 * @brief get the first node in a list
 *
 * @param list the doubly-linked list to operate on
 *
 * @return the first node in the list, NULL if list is empty
 */

static inline sys_dnode_t *sys_dlist_get(sys_dlist_t *list)
{
	sys_dnode_t *node;

	if (sys_dlist_is_empty(list)) {
		return NULL;
	}

	node = list->head;
	sys_dlist_remove(node);
	return node;
}

#endif /* _misc_dlist__h_ */
