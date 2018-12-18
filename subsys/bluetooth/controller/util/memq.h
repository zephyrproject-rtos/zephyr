/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief   Element of a memory-queue
 * @details Elements form a linked list, and as payload carries a pointer
 */
struct _memq_link {
	struct _memq_link *next; /* permit chaining */
	void              *mem;  /* payload */
};

typedef struct _memq_link memq_link_t;

#define MEMQ_DECLARE(name) \
	struct { \
		memq_link_t *head; \
		memq_link_t *tail; \
	} memq_##name

memq_link_t *memq_init(memq_link_t *link, memq_link_t **head,
		       memq_link_t **tail);

#define MEMQ_INIT(name, link) \
	memq_init(link, &memq_##name.head, &memq_##name.tail)

memq_link_t *memq_deinit(memq_link_t **head, memq_link_t **tail);
memq_link_t *memq_enqueue(memq_link_t *link, void *mem, memq_link_t **tail);
memq_link_t *memq_peek(memq_link_t *head, memq_link_t *tail, void **mem);
memq_link_t *memq_dequeue(memq_link_t *tail, memq_link_t **head, void **mem);
