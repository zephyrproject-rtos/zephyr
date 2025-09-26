/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ring_buffer, LOG_LEVEL_DBG);

static size_t index(size_t p, size_t size)
{
	return p % size;
}

static size_t read_index(const struct ring_buffer *rb)
{
	return index(rb->read_ptr, rb->size);
}

static size_t write_index(const struct ring_buffer *rb)
{
	return index(rb->write_ptr, rb->size);
}

size_t ring_buffer_size(const struct ring_buffer *rb)
{
	return rb->write_ptr - rb->read_ptr;
}

size_t ring_buffer_capacity(const struct ring_buffer *rb)
{
	return rb->size;
}

bool ring_buffer_full(const struct ring_buffer *rb)
{
	return ring_buffer_size(rb) == ring_buffer_capacity(rb);
}

bool ring_buffer_empty(const struct ring_buffer *rb)
{
	return ring_buffer_size(rb) == 0;
}

size_t ring_buffer_space(const struct ring_buffer *rb)
{
	return ring_buffer_capacity(rb) - ring_buffer_size(rb);
}

static size_t max_dma_read_size(const struct ring_buffer *rb)
{
	return MIN(ring_buffer_size(rb), ring_buffer_capacity(rb) - read_index(rb));
}

static size_t max_dma_write_size(const struct ring_buffer *rb)
{
	return MIN(ring_buffer_space(rb), ring_buffer_capacity(rb) - write_index(rb));
}

size_t ring_buffer_write_ptr(const struct ring_buffer *rb, uint8_t **data)
{
	*data = &rb->buffer[write_index(rb)];
	return max_dma_write_size(rb);
}

void ring_buffer_commit(struct ring_buffer *rb, size_t appended)
{
	ring_buffer_index_t diff;
	ring_buffer_index_t cache_write_ptr = rb->write_ptr;

	__ASSERT(appended <= ring_buffer_space(rb), "Trying to commit more data than available space");
	rb->write_ptr = rb->write_ptr + appended;
	if (unlikely(rb->write_ptr < cache_write_ptr)) {
		/* underlying index type overflowed */
		diff = cache_write_ptr - rb->read_ptr;
		rb->write_ptr = index(cache_write_ptr, rb->size) + diff + appended;
		rb->read_ptr = read_index(rb);
	}
}

size_t ring_buffer_read_ptr(const struct ring_buffer *rb, uint8_t **data)
{
	*data = &rb->buffer[read_index(rb)];
	return max_dma_read_size(rb);
}

void ring_buffer_consume(struct ring_buffer *rb, size_t consumed)
{
	__ASSERT(consumed <= ring_buffer_size(rb), "Trying to consume more data than available");
	rb->read_ptr = rb->read_ptr + consumed;
}

size_t ring_buffer_read(struct ring_buffer *rb, uint8_t *data, size_t len)
{
	uint8_t *ref;
	size_t read = 0;
	size_t read_size;

	while (read < len) {
		read_size = MIN(ring_buffer_read_ptr(rb, &ref), len - read);
		if (read_size == 0) {
			break;
		}
		memcpy(&data[read], ref, read_size);
		ring_buffer_consume(rb, read_size);
		read += read_size;
	}
	return read;
}

size_t ring_buffer_write(struct ring_buffer *rb, const uint8_t *data, size_t len)
{
	uint8_t *ref;
	size_t write_size;
	size_t written = 0;

	while (written < len) {
		write_size = MIN(ring_buffer_write_ptr(rb, &ref), len - written);
		if (write_size == 0) {
			break;
		}
		memcpy(ref, &data[written], write_size);
		ring_buffer_commit(rb, write_size);
		written += write_size;
	}

	return written;
}

size_t ring_buffer_peek(struct ring_buffer *rb, uint8_t *data, size_t len)
{
	size_t cache_read_ptr;
	size_t read;

	cache_read_ptr = rb->read_ptr;
	read = ring_buffer_read(rb, data, len);

	rb->read_ptr = cache_read_ptr;

	return read;
}

void ring_buffer_reset(struct ring_buffer *rb)
{
	rb->read_ptr = 0;
	rb->write_ptr = 0;
}

void ring_buffer_init(struct ring_buffer *rb, uint8_t *data, size_t size)
{
	rb->size = size;
	rb->buffer = data;
	__ASSERT(rb->size <= RING_BUFFER_MAX_SIZE, RING_BUFFER_SIZE_ERROR_MSG);
	ring_buffer_reset(rb);
}
