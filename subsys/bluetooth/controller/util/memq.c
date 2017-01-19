/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

inline void *memq_peek(void *tail, void *head, void **mem);

void *memq_init(void *link, void **head, void **tail)
{
	/* head and tail pointer to the initial link node */
	*head = *tail = link;

	return link;
}

void *memq_enqueue(void *mem, void *link, void **tail)
{
	/* make the current tail link node point to new link node */
	*((void **)*tail) = link;

	/* assign mem to current tail link node */
	*((void **)*tail + 1) = mem;

	/* increment the tail! */
	*tail = link;

	return link;
}

void *memq_peek(void *tail, void *head, void **mem)
{
	void *link;

	/* if head and tail are equal, then queue empty */
	if (head == tail) {
		return 0;
	}

	/* pick the head link node */
	link = head;

	/* extract the element node */
	if (mem) {
		*mem = *((void **)link + 1);
	}

	return link;
}

void *memq_dequeue(void *tail, void **head, void **mem)
{
	void *link;

	/* use memq peek to get the link and mem */
	link = memq_peek(tail, *head, mem);

	/* increment the head to next link node */
	*head = *((void **)link);

	return link;
}

uint32_t memq_ut(void)
{
	void *head;
	void *tail;
	void *link;
	void *link_0[2];
	void *link_1[2];

	link = memq_init(&link_0[0], &head, &tail);
	if ((link != &link_0[0]) || (head != &link_0[0])
	    || (tail != &link_0[0])) {
		return 1;
	}

	link = memq_dequeue(tail, &head, 0);
	if ((link) || (head != &link_0[0])) {
		return 2;
	}

	link = memq_enqueue(0, &link_1[0], &tail);
	if ((link != &link_1[0]) || (tail != &link_1[0])) {
		return 3;
	}

	link = memq_dequeue(tail, &head, 0);
	if ((link != &link_0[0]) || (tail != &link_1[0])
	    || (head != &link_1[0])) {
		return 4;
	}

	return 0;
}
