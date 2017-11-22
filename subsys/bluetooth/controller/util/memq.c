/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>

#include "memq.h"

inline memq_link_t *memq_peek(memq_link_t *head, memq_link_t *tail, void **mem);

memq_link_t *memq_init(memq_link_t *link, memq_link_t **head, memq_link_t **tail)
{
	/* head and tail pointer to the initial link */
	*head = *tail = link;

	return link;
}

memq_link_t *memq_enqueue(memq_link_t *link, void *mem, memq_link_t **tail)
{
	/* make the current tail link's next point to new link */
	(*tail)->next = link;

	/* assign mem to current tail link's mem */
	(*tail)->mem = mem;

	/* increment the tail! */
	*tail = link;

	return link;
}

memq_link_t *memq_peek(memq_link_t *head, memq_link_t *tail, void **mem)
{
	/* if head and tail are equal, then queue empty */
	if (head == tail) {
		return NULL;
	}

	/* extract the link's mem */
	if (mem) {
		*mem = head->mem;
	}

	return head;
}

memq_link_t *memq_dequeue(memq_link_t *tail, memq_link_t **head, void **mem)
{
	memq_link_t *link;

	/* use memq peek to get the link and mem */
	link = memq_peek(*head, tail, mem);
	if (!link) {
		return link;
	}

	/* increment the head to next link node */
	*head = link->next;

	return link;
}
