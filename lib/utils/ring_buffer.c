/* ring_buffer.c: Simple ring buffer API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/ring_buffer.h>
#include <string.h>

uint32_t ring_buf_area_claim(struct ring_buf *buf, struct ring_buf_index *ring,
			     uint8_t **data, uint32_t size)
{
	ring_buf_idx_t head_offset, wrap_size;

	head_offset = ring->head - ring->base;
	if (unlikely(head_offset >= buf->size)) {
		/* ring->base is not yet adjusted */
		head_offset -= buf->size;
	}
	wrap_size = buf->size - head_offset;
	size = MIN(size, wrap_size);

	*data = &buf->buffer[head_offset];
	ring->head += size;

	return size;
}

int ring_buf_area_finish(struct ring_buf *buf, struct ring_buf_index *ring,
			 uint32_t size)
{
	ring_buf_idx_t claimed_size, tail_offset;

	claimed_size = ring->head - ring->tail;
	if (unlikely(size > claimed_size)) {
		return -EINVAL;
	}

	ring->tail += size;
	ring->head = ring->tail;

	tail_offset = ring->tail - ring->base;
	if (unlikely(tail_offset >= buf->size)) {
		/* we wrapped: adjust ring->base */
		ring->base += buf->size;
	}

	return 0;
}

uint32_t ring_buf_put(struct ring_buf *buf, const uint8_t *data, uint32_t size)
{
	uint8_t *dst;
	uint32_t partial_size;
	uint32_t total_size = 0U;
	int err;

	do {
		partial_size = ring_buf_put_claim(buf, &dst, size);
		if (partial_size == 0) {
			break;
		}
		memcpy(dst, data, partial_size);
		total_size += partial_size;
		size -= partial_size;
		data += partial_size;
	} while (size != 0);

	err = ring_buf_put_finish(buf, total_size);
	__ASSERT_NO_MSG(err == 0);
	ARG_UNUSED(err);

	return total_size;
}

uint32_t ring_buf_get(struct ring_buf *buf, uint8_t *data, uint32_t size)
{
	uint8_t *src;
	uint32_t partial_size;
	uint32_t total_size = 0U;
	int err;

	do {
		partial_size = ring_buf_get_claim(buf, &src, size);
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

	err = ring_buf_get_finish(buf, total_size);
	__ASSERT_NO_MSG(err == 0);
	ARG_UNUSED(err);

	return total_size;
}

uint32_t ring_buf_peek(struct ring_buf *buf, uint8_t *data, uint32_t size)
{
	uint8_t *src;
	uint32_t partial_size;
	uint32_t total_size = 0U;
	int err;

	do {
		partial_size = ring_buf_get_claim(buf, &src, size);
		if (partial_size == 0) {
			break;
		}
		__ASSERT_NO_MSG(data != NULL);
		memcpy(data, src, partial_size);
		data += partial_size;
		total_size += partial_size;
		size -= partial_size;
	} while (size != 0);

	/* effectively unclaim total_size bytes */
	err = ring_buf_get_finish(buf, 0);
	__ASSERT_NO_MSG(err == 0);
	ARG_UNUSED(err);

	return total_size;
}

/**
 * Internal data structure for a buffer header.
 *
 * We want all of this to fit in a single uint32_t. Every item stored in the
 * ring buffer will be one of these headers plus any extra data supplied
 */
struct ring_element {
	uint32_t  type   :16; /**< Application-specific */
	uint32_t  length :8;  /**< length in 32-bit chunks */
	uint32_t  value  :8;  /**< Room for small integral values */
};

int ring_buf_item_put(struct ring_buf *buf, uint16_t type, uint8_t value,
		      uint32_t *data32, uint8_t size32)
{
	uint8_t *dst, *data = (uint8_t *)data32;
	struct ring_element *header;
	uint32_t space, size, partial_size, total_size;
	int err;

	space = ring_buf_space_get(buf);
	size = size32 * 4;
	if (size + sizeof(struct ring_element) > space) {
		return -EMSGSIZE;
	}

	err = ring_buf_put_claim(buf, &dst, sizeof(struct ring_element));
	__ASSERT_NO_MSG(err == sizeof(struct ring_element));

	header = (struct ring_element *)dst;
	header->type = type;
	header->length = size32;
	header->value = value;
	total_size = sizeof(struct ring_element);

	do {
		partial_size = ring_buf_put_claim(buf, &dst, size);
		if (partial_size == 0) {
			break;
		}
		memcpy(dst, data, partial_size);
		size -= partial_size;
		total_size += partial_size;
		data += partial_size;
	} while (size != 0);
	__ASSERT_NO_MSG(size == 0);

	err = ring_buf_put_finish(buf, total_size);
	__ASSERT_NO_MSG(err == 0);
	ARG_UNUSED(err);

	return 0;
}

int ring_buf_item_get(struct ring_buf *buf, uint16_t *type, uint8_t *value,
		      uint32_t *data32, uint8_t *size32)
{
	uint8_t *src, *data = (uint8_t *)data32;
	struct ring_element *header;
	uint32_t size, partial_size, total_size;
	int err;

	if (ring_buf_is_empty(buf)) {
		return -EAGAIN;
	}

	err = ring_buf_get_claim(buf, &src, sizeof(struct ring_element));
	__ASSERT_NO_MSG(err == sizeof(struct ring_element));

	header = (struct ring_element *)src;

	if (data && (header->length > *size32)) {
		*size32 = header->length;
		ring_buf_get_finish(buf, 0);
		return -EMSGSIZE;
	}

	*size32 = header->length;
	*type = header->type;
	*value = header->value;
	total_size = sizeof(struct ring_element);

	size = *size32 * 4;

	do {
		partial_size = ring_buf_get_claim(buf, &src, size);
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

	err = ring_buf_get_finish(buf, total_size);
	__ASSERT_NO_MSG(err == 0);
	ARG_UNUSED(err);

	return 0;
}
