/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/fifo.h>

void fifo_init(struct fifo *queue, void *data, size_t item_size, size_t item_capacity)
{
	ring_buf_init(&queue->rb, item_size * item_capacity, data);
	queue->item_size = item_size;
}

void fifo_reset(struct fifo *queue)
{
	ring_buf_reset(&queue->rb);
}

bool fifo_empty(const struct fifo *queue)
{
	return ring_buf_is_empty(&queue->rb);
}

size_t fifo_space(const struct fifo *queue)
{
	return ring_buf_space_get(&queue->rb) / queue->item_size;
}

bool fifo_full(const struct fifo *queue)
{
	return fifo_space(queue) == 0;
}

size_t fifo_capacity(const struct fifo *queue)
{
	return ring_buf_capacity_get(&queue->rb) / queue->item_size;
}

size_t fifo_size(const struct fifo *queue)
{
	return ring_buf_size_get(&queue->rb) / queue->item_size;
}

int fifo_put(struct fifo *queue, const void *element)
{
	if (fifo_full(queue)) {
		return -ENOSPC;
	}

	ring_buf_put(&queue->rb, (const uint8_t *)element, queue->item_size);
	return 0;
}

int fifo_get(struct fifo *queue, void *element)
{
	if (fifo_empty(queue)) {
		return -ENODATA;
	}

	ring_buf_get(&queue->rb, (uint8_t *)element, queue->item_size);
	return 0;
}

int fifo_peek(struct fifo *queue, void *data)
{
	if (fifo_empty(queue)) {
		return -ENODATA;
	}

	ring_buf_peek(&queue->rb, (uint8_t *)data, queue->item_size);
	return 0;
}
