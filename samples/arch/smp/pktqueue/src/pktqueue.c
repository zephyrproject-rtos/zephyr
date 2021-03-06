/*
 * Copyright (c) 2020 Synopsys, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pktqueue.h"

/* Put a packet header in a queue defined in argument */
void phdr_desc_enqueue(struct phdr_desc_queue *queue, struct phdr_desc *desc,
			struct k_mutex *mutex)
{
	/* Locking queue */
	k_mutex_lock(mutex, K_FOREVER);

	if (queue->count == 0) {
		queue->head = queue->tail = desc;
	} else {
		queue->tail->next = desc;
		queue->tail = desc;
	}
	queue->count++;
	desc->next = NULL;

	/* Unlocking queue */
	k_mutex_unlock(mutex);
}

/* Take a packet header from queue defined in argument */
struct phdr_desc *phdr_desc_dequeue(struct phdr_desc_queue *queue,
					struct k_mutex *mutex)
{
	struct phdr_desc *return_ptr = NULL;
	/* Locking queue */
	k_mutex_lock(mutex, K_FOREVER);
	if (queue->count != 0) {
		queue->count--;
		return_ptr = queue->head;
		queue->head = queue->head->next;
	}

	/* Unlocking queue */
	k_mutex_unlock(mutex);

	return return_ptr;
}
