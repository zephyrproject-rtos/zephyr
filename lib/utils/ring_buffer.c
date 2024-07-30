/* ring_buffer.c: Simple ring buffer API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/ring_buffer.h>
#include <string.h>

uint32_t ring_buf_put_claim(struct ring_buf *buf, uint8_t **data, uint32_t size)
{
	uint32_t free_space, wrap_size;
	int32_t base;

	base = buf->put_base;
	wrap_size = buf->put_head - base;
	if (unlikely(wrap_size >= buf->size)) {
		/* put_base is not yet adjusted */
		wrap_size -= buf->size;
		base += buf->size;
	}
	wrap_size = buf->size - wrap_size;

	free_space = ring_buf_space_get(buf);
	size = MIN(size, free_space);
	size = MIN(size, wrap_size);

	*data = &buf->buffer[buf->put_head - base];
	buf->put_head += size;

	return size;
}

int ring_buf_put_finish(struct ring_buf *buf, uint32_t size)
{
	uint32_t finish_space, wrap_size;

	finish_space = buf->put_head - buf->put_tail;
	if (unlikely(size > finish_space)) {
		return -EINVAL;
	}

	buf->put_tail += size;
	buf->put_head = buf->put_tail;

	wrap_size = buf->put_tail - buf->put_base;
	if (unlikely(wrap_size >= buf->size)) {
		/* we wrapped: adjust put_base */
		buf->put_base += buf->size;
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
		memcpy(dst, data, partial_size);
		total_size += partial_size;
		size -= partial_size;
		data += partial_size;
	} while (size && partial_size);

	err = ring_buf_put_finish(buf, total_size);
	__ASSERT_NO_MSG(err == 0);
	ARG_UNUSED(err);

	return total_size;
}

uint32_t ring_buf_get_claim(struct ring_buf *buf, uint8_t **data, uint32_t size)
{
	uint32_t available_size, wrap_size;
	int32_t base;

	base = buf->get_base;
	wrap_size = buf->get_head - base;
	if (unlikely(wrap_size >= buf->size)) {
		/* get_base is not yet adjusted */
		wrap_size -= buf->size;
		base += buf->size;
	}
	wrap_size = buf->size - wrap_size;

	available_size = ring_buf_size_get(buf);
	size = MIN(size, available_size);
	size = MIN(size, wrap_size);

	*data = &buf->buffer[buf->get_head - base];
	buf->get_head += size;

	return size;
}

int ring_buf_get_finish(struct ring_buf *buf, uint32_t size)
{
	uint32_t finish_space, wrap_size;

	finish_space = buf->get_head - buf->get_tail;
	if (unlikely(size > finish_space)) {
		return -EINVAL;
	}

	buf->get_tail += size;
	buf->get_head = buf->get_tail;

	wrap_size = buf->get_tail - buf->get_base;
	if (unlikely(wrap_size >= buf->size)) {
		/* we wrapped: adjust get_base */
		buf->get_base += buf->size;
	}

	return 0;
}

uint32_t ring_buf_get(struct ring_buf *buf, uint8_t *data, uint32_t size)
{
	uint8_t *src;
	uint32_t partial_size;
	uint32_t total_size = 0U;
	int err;

	do {
		partial_size = ring_buf_get_claim(buf, &src, size);
		if (data) {
			memcpy(data, src, partial_size);
			data += partial_size;
		}
		total_size += partial_size;
		size -= partial_size;
	} while (size && partial_size);

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

	size = MIN(size, ring_buf_size_get(buf));

	do {
		partial_size = ring_buf_get_claim(buf, &src, size);
		__ASSERT_NO_MSG(data != NULL);
		memcpy(data, src, partial_size);
		data += partial_size;
		total_size += partial_size;
		size -= partial_size;
	} while (size && partial_size);

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
		memcpy(dst, data, partial_size);
		size -= partial_size;
		total_size += partial_size;
		data += partial_size;
	} while (size && partial_size);
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
		if (data) {
			memcpy(data, src, partial_size);
			data += partial_size;
		}
		total_size += partial_size;
		size -= partial_size;
	} while (size && partial_size);

	err = ring_buf_get_finish(buf, total_size);
	__ASSERT_NO_MSG(err == 0);
	ARG_UNUSED(err);

	return 0;
}
