/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEMQ_H_
#define _MEMQ_H_

struct _memq_link {
	struct _memq_link *next;
	void              *mem;
};

typedef struct _memq_link memq_link_t;

memq_link_t *memq_init(memq_link_t *link, memq_link_t **head,
		       memq_link_t **tail);
memq_link_t *memq_enqueue(memq_link_t *link, void *mem, memq_link_t **tail);
memq_link_t *memq_peek(memq_link_t *head, memq_link_t *tail, void **mem);
memq_link_t *memq_dequeue(memq_link_t *tail, memq_link_t **head, void **mem);

#endif /* _MEMQ_H_ */
