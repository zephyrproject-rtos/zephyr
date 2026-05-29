/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "data_queue.h"

#define DATA_QUEUE_MEMORY_ALIGN sizeof(uint32_t)

struct data_queue_format {
	uint32_t header; /* Required by kernel k_queue_append */
	size_t size;
	uint32_t data[];
};


void data_queue_init(struct data_queue *q, void *mem, size_t bytes)
{
	k_heap_init(&q->h, mem, bytes);
	k_queue_init(&q->q);
}

int data_queue_put(struct data_queue *q, const void *data, size_t bytes, k_timeout_t timeout)
{
	struct data_queue_format *buffer = k_heap_aligned_alloc(
		&q->h,
		DATA_QUEUE_MEMORY_ALIGN,
		bytes + sizeof(struct data_queue_format),
		timeout);

	if (!buffer) {
		return -ENOMEM;
	}
	buffer->size = bytes;
	memcpy(buffer->data, data, bytes);

	k_queue_append(&q->q, buffer);
	return 0;
}

void *data_queue_get(struct data_queue *q, size_t *size, k_timeout_t timeout)
{
	struct data_queue_format *buffer = k_queue_get(&q->q, timeout);

	if (!buffer) {
		return NULL;
	}

	if (size) {
		*size = buffer->size;
	}
	return buffer->data;
}

void data_queue_release(struct data_queue *q, void *data)
{
	struct data_queue_format *buffer = CONTAINER_OF(data, struct data_queue_format, data);

	k_heap_free(&q->h, buffer);
}

int data_queue_is_empty(struct data_queue *q)
{
	return k_queue_is_empty(&q->q);
}
