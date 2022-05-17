/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <string.h>
#include <errno.h>
#include <zephyr/cache.h>
#include <zephyr/sys/spsc_pbuf.h>


/* Helpers */
static uint32_t idx_occupied(uint32_t len, uint32_t a, uint32_t b)
{
	/* It is implicitly assumed a and b cannot differ by more then len. */
	return (b > a) ? (len - (b - a)) : (a - b);
}

static uint32_t idx_cut(uint32_t len, uint32_t idx)
{
	/* It is implicitly assumed a and b cannot differ by more then len. */
	return (idx >= len) ? (idx - len) : (idx);
}

static inline void cache_wb(void *data, size_t len, uint32_t flags)
{
	if (IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_ALWAYS) ||
	    (IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_FLAG) && (flags & SPSC_PBUF_CACHE))) {
		sys_cache_data_range(data, len, K_CACHE_WB);
	}
}

static inline void cache_inv(void *data, size_t len, uint32_t flags)
{
	if (IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_ALWAYS) ||
	    (IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_FLAG) && (flags & SPSC_PBUF_CACHE))) {
		sys_cache_data_range(data, len, K_CACHE_INVD);
	}
}

struct spsc_pbuf *spsc_pbuf_init(void *buf, size_t blen, uint32_t flags)
{
	/* blen must be big enough to contain spsc_pbuf struct, byte of data
	 * and message len (2 bytes).
	 */
	struct spsc_pbuf *pb = buf;

	__ASSERT_NO_MSG(blen > (sizeof(*pb) + sizeof(uint16_t)));

	pb->len = blen - sizeof(*pb);
	pb->wr_idx = 0;
	pb->rd_idx = 0;
	pb->flags = flags;

	__sync_synchronize();
	cache_wb(pb, sizeof(*pb), pb->flags);

	return pb;
}

int spsc_pbuf_write(struct spsc_pbuf *pb, const char *buf, uint16_t len)
{
	/* The length of buffer is immutable - avoid reloading that may happen
	 * due to memory bariers.
	 */
	const uint32_t pblen = pb->len;

	/* rx_idx == wr_idx means the buffer is empty.
	 * Max bytes that can be stored is len - 1.
	 */
	const uint32_t max_len = pblen - 1;

	cache_inv(pb, sizeof(*pb), pb->flags);
	__sync_synchronize();

	uint32_t wr_idx = pb->wr_idx;
	uint32_t rd_idx = pb->rd_idx;

	if (len == 0) {
		/* Incorrect call. */
		return -EINVAL;
	}

	uint32_t avail = max_len - idx_occupied(pblen, wr_idx, rd_idx);

	if ((len + sizeof(len) > avail) ||
	    (len + sizeof(len) > max_len)) {
		/* No free space. */
		return -ENOMEM;
	}

	/* Store info about the message length. */
	pb->data[wr_idx] = (uint8_t)len;
	cache_wb(&pb->data[wr_idx], sizeof(pb->data[wr_idx]), pb->flags);
	wr_idx = idx_cut(pblen, wr_idx + sizeof(pb->data[wr_idx]));

	pb->data[wr_idx] = (uint8_t)(len >> 8);
	cache_wb(&pb->data[wr_idx], sizeof(pb->data[wr_idx]), pb->flags);
	wr_idx = idx_cut(pblen, wr_idx + sizeof(pb->data[wr_idx]));

	/* Write until the end of the buffer. */
	uint32_t sz = MIN(len, pblen - wr_idx);

	memcpy(&pb->data[wr_idx], buf, sz);
	cache_wb(&pb->data[wr_idx], sz, pb->flags);

	if (len > sz) {
		/* Write remaining data at the buffer head. */
		memcpy(&pb->data[0], buf + sz, len - sz);
		cache_wb(&pb->data[0], len - sz, pb->flags);
	}

	/* Update write index - make other side aware data was written. */
	__sync_synchronize();
	wr_idx = idx_cut(pblen, wr_idx + len);
	pb->wr_idx = wr_idx;

	cache_wb(pb, sizeof(*pb), pb->flags);

	return len;
}

int spsc_pbuf_read(struct spsc_pbuf *pb, char *buf, uint16_t len)
{
	/* The length of buffer is immutable - avoid reloading. */
	const uint32_t pblen = pb->len;

	cache_inv(pb, sizeof(*pb), pb->flags);
	__sync_synchronize();

	uint32_t rd_idx = pb->rd_idx;
	uint32_t wr_idx = pb->wr_idx;

	if (rd_idx == wr_idx) {
		/* The buffer is empty. */
		return 0;
	}

	uint32_t bytes_stored = idx_occupied(pblen, wr_idx, rd_idx);

	/* Read message len. */
	cache_inv(&pb->data[rd_idx], sizeof(pb->data[rd_idx]), pb->flags);
	uint16_t mlen = pb->data[rd_idx];

	rd_idx = idx_cut(pblen, rd_idx + sizeof(pb->data[rd_idx]));

	cache_inv(&pb->data[rd_idx], sizeof(pb->data[rd_idx]), pb->flags);
	mlen |= (pb->data[rd_idx] << 8);
	rd_idx = idx_cut(pblen, rd_idx + sizeof(pb->data[rd_idx]));

	if (!buf) {
		return mlen;
	}

	if (len < mlen) {
		return -ENOMEM;
	}

	if (bytes_stored < mlen + sizeof(mlen)) {
		/* Part of message not available. Should not happen. */
		__ASSERT_NO_MSG(false);
		return -EAGAIN;
	}

	len = MIN(len, mlen);

	/* Read up to the end of the buffer. */
	uint32_t sz = MIN(len, pblen - rd_idx);

	cache_inv(&pb->data[rd_idx], sz, pb->flags);
	memcpy(buf, &pb->data[rd_idx], sz);
	if (len > sz) {
		/* Read remaining bytes starting from the buffer head. */
		cache_inv(&pb->data[0], len - sz, pb->flags);
		memcpy(&buf[sz], &pb->data[0], len - sz);
	}

	/* Update read index - make other side aware data was read. */
	__sync_synchronize();
	rd_idx = idx_cut(pblen, rd_idx + len);
	pb->rd_idx = rd_idx;

	cache_wb(pb, sizeof(*pb), pb->flags);

	return len;
}
