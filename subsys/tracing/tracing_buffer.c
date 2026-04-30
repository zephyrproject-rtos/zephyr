/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/util.h>
#include <errno.h>

static struct trace_buffer rb;
static uint8_t tracing_buffer[CONFIG_TRACING_BUFFER_SIZE + 1];
static uint8_t tracing_cmd_buffer[CONFIG_TRACING_CMD_BUFFER_SIZE];

struct buf_area { size_t head, tail, base; };
struct trace_buffer {
	uint8_t *buffer;
	struct buf_area put;
	struct buf_area get;
	size_t size;
};

static size_t area_claim(struct trace_buffer *buf, struct buf_area *area,
		uint8_t **data, size_t size)
{
	size_t head_offset, wrap_size;

	head_offset = area->head - area->base;
	if (unlikely(head_offset >= buf->size)) {
		head_offset -= buf->size;
	}
	wrap_size = buf->size - head_offset;
	size = MIN(size, wrap_size);

	*data = &buf->buffer[head_offset];
	area->head += size;

	return size;
}

static int area_finish(struct trace_buffer *buf, struct buf_area *area, size_t size)
{
	size_t claimed_size, tail_offset;

	claimed_size = area->head - area->tail;
	if (unlikely(size > claimed_size)) {
		return -EINVAL;
	}

	area->tail += size;
	area->head = area->tail;

	tail_offset = area->tail - area->base;
	if (unlikely(tail_offset >= buf->size)) {
		area->base += buf->size;
	}

	return 0;
}

uint32_t tracing_cmd_buffer_alloc(uint8_t **data)
{
	*data = &tracing_cmd_buffer[0];

	return sizeof(tracing_cmd_buffer);
}

uint32_t tracing_buffer_put_claim(uint8_t **data, uint32_t size)
{
	return area_claim(&rb, &rb.put, data, size);
}

int tracing_buffer_put_finish(uint32_t size)
{
	return area_finish(&rb, &rb.put, size);
}

uint32_t tracing_buffer_put(uint8_t *data, uint32_t size)
{
	uint8_t *dst;
	uint32_t partial_size;
	uint32_t total_size = 0U;

	do {
		partial_size = tracing_buffer_put_claim(&dst, size);
		if (partial_size == 0) {
			break;
		}
		memcpy(dst, data, partial_size);
		total_size += partial_size;
		size -= partial_size;
		data += partial_size;
	} while (size != 0);

	tracing_buffer_put_finish(total_size);
	return total_size;
}

uint32_t tracing_buffer_get_claim(uint8_t **data, uint32_t size)
{
	return area_claim(&rb, &rb.get, data, size);
}

int tracing_buffer_get_finish(uint32_t size)
{
	return area_finish(&rb, &rb.get, size);
}

uint32_t tracing_buffer_get(uint8_t *data, uint32_t size)
{
	uint8_t *src;
	uint32_t partial_size;
	uint32_t total_size = 0U;

	do {
		partial_size = tracing_buffer_get_claim(&src, size);
		if (partial_size == 0) {
			break;
		}
		if (data) {
			memcpy(data, src, partial_size);
			data += partial_size;
		}
		total_size += partial_size;
		size -= partial_size;
	} while (size != 0);
	tracing_buffer_get_finish(total_size);

	return total_size;
}

void tracing_buffer_init(void)
{
	memset(&rb, 0, sizeof(rb));
	rb.buffer = tracing_buffer;
	rb.size = sizeof(tracing_buffer);
}

uint32_t tracing_buffer_capacity_get(void)
{
	return rb.size;
}

uint32_t tracing_buffer_space_get(void)
{
	return rb.size - (rb.put.head - rb.get.tail);
}

bool tracing_buffer_is_empty(void)
{
	return rb.get.head == rb.put.tail;
}
