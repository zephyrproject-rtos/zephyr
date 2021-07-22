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

/* Check if indexes did not progress too far (too close to 32-bit wrapping).
 * If so, then reduce all indexes by an arbitrary value.
 */
static void item_indexes_rewind(struct ring_buf *buf)
{
	uint32_t rew;
	uint32_t threshold = ring_buf_get_rewind_threshold();

	if (buf->head < threshold) {
		return;
	}

	rew = buf->size * (threshold / buf->size);

	k_spinlock_key_t key = k_spin_lock(&buf->lock);

	buf->tail -= rew;
	buf->head -= rew;
	k_spin_unlock(&buf->lock, key);
}

/* Check if indexes did not progresses too far (too close to 32-bit wrapping).
 * If so, then rewind all indexes by an arbitrary value. For byte mode temporary
 * indexes must also be reduced.
 */
static void byte_indexes_rewind(struct ring_buf *buf)
{
	uint32_t rew;
	uint32_t threshold = ring_buf_get_rewind_threshold();

	/* Checking head since it is the smallest index. */
	if (buf->head < threshold) {
		return;
	}

	rew = buf->size * (threshold / buf->size);

	k_spinlock_key_t key = k_spin_lock(&buf->lock);

	buf->tail -= rew;
	buf->head -= rew;
	buf->misc.byte_mode.tmp_head -= rew;
	buf->misc.byte_mode.tmp_tail -= rew;
	k_spin_unlock(&buf->lock, key);
}

int ring_buf_item_put(struct ring_buf *buf, uint16_t type, uint8_t value,
		      uint32_t *data, uint8_t size32)
{
	uint32_t i, space, index, rc;

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

		buf->tail = buf->tail + size32 + 1;
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

	if (ring_buf_is_empty(buf)) {
		return -EAGAIN;
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

	buf->head = buf->head + header->length + 1;

	item_indexes_rewind(buf);

	return 0;
}

/** @brief Wraps index if it exceeds the limit.
 *
 * @param val  Value
 * @param max  Max.
 *
 * @return value % max.
 */
static inline uint32_t wrap(uint32_t val, uint32_t max)
{
	return val >= max ? (val - max) : val;
}

uint32_t ring_buf_put_claim(struct ring_buf *buf, uint8_t **data, uint32_t size)
{
	uint32_t space, trail_size, allocated, tmp_trail_mod;

	tmp_trail_mod = mod(buf, buf->misc.byte_mode.tmp_tail);
	space = (buf->head + buf->size) - buf->misc.byte_mode.tmp_tail;
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
	if ((buf->tail + size) > (buf->head + buf->size)) {
		return -EINVAL;
	}

	buf->tail += size;
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

	tmp_head_mod = mod(buf, buf->misc.byte_mode.tmp_head);
	space = buf->tail - buf->misc.byte_mode.tmp_head;
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
	if ((buf->head + size) > buf->tail) {
		return -EINVAL;
	}

	buf->head += size;
	buf->misc.byte_mode.tmp_head = buf->head;

	byte_indexes_rewind(buf);

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
