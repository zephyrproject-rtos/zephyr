/* dlist.c - doubly linked list routines */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
This module implements various routines for manipulating linked lists.
The lists are expected to be initialized such that both the head and tail
pointers point to the list itself.  Initializing the lists in such a fashion
simplifies the adding and removing of nodes to/from the list.

\NOMANUAL
*/

/* includes */

#include <stddef.h>
#include <misc/dlist.h>

/*******************************************************************************
*
* dlistAdd - add node to tail of list
*
* This routine adds a node to the tail of the specified list.  It is assumed
* that the node is not part of any linked list before this routine is called.
*
* RETURNS: N/A
*/

void dlistAdd(
	dlist_t *pList, /* pointer to list to which to append node */
	dnode_t *pNode  /* pointer to node to append */
	)
{
	pNode->next = pList;
	pNode->prev = pList->tail;

	pList->tail->next = pNode;
	pList->tail = pNode;
}

/*******************************************************************************
*
* dlistRemove - remove a node from the list
*
* This routine removes a node from the list on which it resides.  An explicit
* pointer to the list is not required due to the way the node and list are
* structured and initialized.
*
* RETURNS: N/A
*/

void dlistRemove(dnode_t *pNode /* pointer to node to remove */
			       )
{
	pNode->prev->next = pNode->next;
	pNode->next->prev = pNode->prev;
}

/*******************************************************************************
*
* dlistGet - get the node from the head of the list
*
* This routine removes the node from the head of the list and then returns
* a pointer to it.
*
* RETURNS: pointer to node
*/

dnode_t *dlistGet(
	dlist_t *pList /* pointer to list from which to get the item */
	)
{
	dnode_t *pNode = pList->head;

	if (pNode == pList) {
		return NULL;
	}

	dlistRemove(pNode); /* remove from <pList> */

	return pNode;
}
