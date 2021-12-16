/* ring_buffer.c: Simple ring buffer API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/ring_buffer.h>
#include <string.h>

/* LCOV_EXCL_START */
/* The weak function used to allow overwriting it in the test and trigger
 * rewinding earlier.
 */
uint32_t __weak ring_buf_get_rewind_threshold(void)
{
	return RING_BUFFER_MAX_SIZE;
}
/* LCOV_EXCL_STOP */

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

static uint32_t mod(struct ring_buf *buf, uint32_t val)
{
	return likely(buf->mask) ? val & buf->mask : val % buf->size;
}

static uint32_t get_rewind_value(uint32_t buf_size, uint32_t threshold)
{
	/* Rewind value is rounded to buffer size and decreased by buffer_size.
	 * This is done to ensure that there will be no negative numbers after
	 * subtraction. That could happen because tail is rewinded first and
	 * head (which follows tail) is rewinded on next getting.
	 */
	return buf_size * (threshold / buf_size - 1);
}

int ring_buf_is_empty(struct ring_buf *buf)
{
	uint32_t tail = buf->tail;
	uint32_t head = buf->head;

	if (tail < head) {
		tail += get_rewind_value(buf->size,
					 ring_buf_get_rewind_threshold());
	}

	return (head == tail);
}

uint32_t ring_buf_size_get(struct ring_buf *buf)
{
	uint32_t tail = buf->tail;
	uint32_t head = buf->head;

	if (tail < head) {
		tail += get_rewind_value(buf->size,
					 ring_buf_get_rewind_threshold());
	}

	return tail - head;
}

uint32_t ring_buf_space_get(struct ring_buf *buf)
{
	return buf->size - ring_buf_size_get(buf);
}

int ring_buf_item_put(struct ring_buf *buf, uint16_t type, uint8_t value,
		      uint32_t *data, uint8_t size32)
{
	uint32_t i, space, index, rc;
	uint32_t threshold = ring_buf_get_rewind_threshold();
	uint32_t rew;

	space = ring_buf_space_get(buf);
	if (space >= (size32 + 1)) {
		struct ring_element *header =
		    (struct ring_element *)&buf->buf.buf32[mod(buf, buf->tail)];

		header->type = type;
		header->length = size32;
		header->value = value;

		if (likely(buf->mask)) {
			for (i = 0U; i < size32; ++i) {
				index = (i + buf->tail + 1) & buf->mask;
				buf->buf.buf32[index] = data[i];
			}
		} else {
			for (i = 0U; i < size32; ++i) {
				index = (i + buf->tail + 1) % buf->size;
				buf->buf.buf32[index] = data[i];
			}
		}

		/* Check if indexes shall be rewound. */
		if (buf->tail > threshold) {
			rew = get_rewind_value(buf->size, threshold);
		} else {
			rew = 0;
		}

		buf->tail = buf->tail + (size32 + 1 - rew);
		rc = 0U;
	} else {
		buf->misc.item_mode.dropped_put_count++;
		rc = -EMSGSIZE;
	}

	return rc;
}

int ring_buf_item_get(struct ring_buf *buf, uint16_t *type, uint8_t *value,
		      uint32_t *data, uint8_t *size32)
{
	struct ring_element *header;
	uint32_t i, index;
	uint32_t tail = buf->tail;
	uint32_t rew;

	/* Tail is always ahead, if it is not, it's only because it got rewound. */
	if (tail < buf->head) {
		/* Locally undo rewind to get tail aligned with head. */
		rew = get_rewind_value(buf->size,
				       ring_buf_get_rewind_threshold());
		tail += rew;
	} else if (ring_buf_is_empty(buf)) {
		return -EAGAIN;
	} else {
		rew = 0;
	}

	header = (struct ring_element *) &buf->buf.buf32[mod(buf, buf->head)];

	if (data && (header->length > *size32)) {
		*size32 = header->length;
		return -EMSGSIZE;
	}

	*size32 = header->length;
	*type = header->type;
	*value = header->value;

	if (data) {
		if (likely(buf->mask)) {
			for (i = 0U; i < header->length; ++i) {
				index = (i + buf->head + 1) & buf->mask;
				data[i] = buf->buf.buf32[index];
			}
		} else {
			for (i = 0U; i < header->length; ++i) {
				index = (i + buf->head + 1) % buf->size;
				data[i] = buf->buf.buf32[index];
			}
		}
	}

	/* Include potential rewinding */
	buf->head = buf->head + header->length + 1 - rew;

	return 0;
}

uint32_t ring_buf_put_claim(struct ring_buf *buf, uint8_t **data, uint32_t size)
{
	uint32_t space, trail_size, allocated, tmp_trail_mod;
	uint32_t head = buf->head;
	uint32_t tmp_tail = buf->misc.byte_mode.tmp_tail;

	if (buf->misc.byte_mode.tmp_tail < head) {
		/* Head is already rewinded but tail is not */
		tmp_tail += get_rewind_value(buf->size, ring_buf_get_rewind_threshold());
	}

	tmp_trail_mod = mod(buf, buf->misc.byte_mode.tmp_tail);
	space = (head + buf->size) - tmp_tail;
	trail_size = buf->size - tmp_trail_mod;

	/* Limit requested size to available size. */
	size = MIN(size, space);

	trail_size = buf->size - (tmp_trail_mod);

	/* Limit allocated size to trail size. */
	allocated = MIN(trail_size, size);
	*data = &buf->buf.buf8[tmp_trail_mod];

	buf->misc.byte_mode.tmp_tail =
		buf->misc.byte_mode.tmp_tail + allocated;

	return allocated;
}

int ring_buf_put_finish(struct ring_buf *buf, uint32_t size)
{
	uint32_t rew;
	uint32_t threshold = ring_buf_get_rewind_threshold();

	if ((buf->tail + size) > (buf->head + buf->size)) {
		return -EINVAL;
	}

	/* Check if indexes shall be rewind. */
	if (buf->tail > threshold) {
		rew = get_rewind_value(buf->size, threshold);
	} else {
		rew = 0;
	}

	buf->tail += (size - rew);
	buf->misc.byte_mode.tmp_tail = buf->tail;

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

	return total_size;
}

uint32_t ring_buf_get_claim(struct ring_buf *buf, uint8_t **data, uint32_t size)
{
	uint32_t space, granted_size, trail_size, tmp_head_mod;
	uint32_t tail = buf->tail;

	/* Tail is always ahead, if it is not, it's only because it got rewinded. */
	if (tail < buf->misc.byte_mode.tmp_head) {
		/* Locally, increment it to pre-rewind value */
		tail += get_rewind_value(buf->size,
					 ring_buf_get_rewind_threshold());
	}

	tmp_head_mod = mod(buf, buf->misc.byte_mode.tmp_head);
	space = tail - buf->misc.byte_mode.tmp_head;
	trail_size = buf->size - tmp_head_mod;

	/* Limit requested size to available size. */
	granted_size = MIN(size, space);

	/* Limit allocated size to trail size. */
	granted_size = MIN(trail_size, granted_size);

	*data = &buf->buf.buf8[tmp_head_mod];
	buf->misc.byte_mode.tmp_head += granted_size;

	return granted_size;
}

int ring_buf_get_finish(struct ring_buf *buf, uint32_t size)
{
	uint32_t tail = buf->tail;
	uint32_t rew;

	/* Tail is always ahead, if it is not, it's only because it got rewinded. */
	if (tail < buf->misc.byte_mode.tmp_head) {
		/* tail was rewinded. Locally, increment it to pre-rewind value */
		rew = get_rewind_value(buf->size,
				       ring_buf_get_rewind_threshold());
		tail += rew;
	} else {
		rew = 0;
	}

	if ((buf->head + size) > tail) {
		return -EINVAL;
	}

	/* Include potential rewinding. */
	buf->head += (size - rew);
	buf->misc.byte_mode.tmp_head = buf->head;

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

	return total_size;
}
