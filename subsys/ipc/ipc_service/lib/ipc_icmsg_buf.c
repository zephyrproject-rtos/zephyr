/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <string.h>
#include <errno.h>
#include <zephyr/cache.h>
#include <zephyr/ipc/ipc_icmsg_buf.h>


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

struct icmsg_buf *icmsg_buf_init(void *buf, size_t blen)
{
	/* blen must be big enough to contain icmsg_buf struct, byte of data
	 * and message len (2 bytes).
	 */
	struct icmsg_buf *ib = buf;

	__ASSERT_NO_MSG(blen > (sizeof(*ib) + sizeof(uint16_t)));

	ib->len = blen - sizeof(*ib);
	ib->wr_idx = 0;
	ib->rd_idx = 0;

	__sync_synchronize();
	sys_cache_data_range(ib, sizeof(*ib), K_CACHE_WB);

	return ib;
}

int icmsg_buf_write(struct icmsg_buf *ib, const char *buf, uint16_t len)
{
	/* The length of buffer is immutable - avoid reloading that may happen
	 * due to memory bariers.
	 */
	const uint32_t iblen = ib->len;

	/* rx_idx == wr_idx means the buffer is empty.
	 * Max bytes that can be stored is len - 1.
	 */
	const uint32_t max_len = iblen - 1;

	sys_cache_data_range(ib, sizeof(*ib), K_CACHE_INVD);
	__sync_synchronize();

	uint32_t wr_idx = ib->wr_idx;
	uint32_t rd_idx = ib->rd_idx;

	if (len == 0) {
		/* Incorrect call. */
		return -EINVAL;
	}

	uint32_t avail = max_len - idx_occupied(iblen, wr_idx, rd_idx);

	if ((len + sizeof(len) > avail) ||
	    (len + sizeof(len) > max_len)) {
		/* No free space. */
		return -ENOMEM;
	}

	/* Store info about the message length. */
	ib->data[wr_idx] = (uint8_t)len;
	sys_cache_data_range(&ib->data[wr_idx], sizeof(ib->data[wr_idx]), K_CACHE_WB);
	wr_idx = idx_cut(iblen, wr_idx + sizeof(ib->data[wr_idx]));

	ib->data[wr_idx] = (uint8_t)(len >> 8);
	sys_cache_data_range(&ib->data[wr_idx], sizeof(ib->data[wr_idx]), K_CACHE_WB);
	wr_idx = idx_cut(iblen, wr_idx + sizeof(ib->data[wr_idx]));

	/* Write until the end of the buffer. */
	uint32_t sz = MIN(len, iblen - wr_idx);

	memcpy(&ib->data[wr_idx], buf, sz);
	sys_cache_data_range(&ib->data[wr_idx], sz, K_CACHE_WB);

	if (len > sz) {
		/* Write remaining data at the buffer head. */
		memcpy(&ib->data[0], buf + sz, len - sz);
		sys_cache_data_range(&ib->data[0], len - sz, K_CACHE_WB);
	}

	/* Update write index - make other side aware data was written. */
	__sync_synchronize();
	wr_idx = idx_cut(iblen, wr_idx + len);
	ib->wr_idx = wr_idx;

	sys_cache_data_range(ib, sizeof(*ib), K_CACHE_WB);

	return len;
}

int icmsg_buf_read(struct icmsg_buf *ib, char *buf, uint16_t len)
{
	/* The length of buffer is immutable - avoid reloading. */
	const uint32_t iblen = ib->len;

	sys_cache_data_range(ib, sizeof(*ib), K_CACHE_INVD);
	__sync_synchronize();

	uint32_t rd_idx = ib->rd_idx;
	uint32_t wr_idx = ib->wr_idx;

	if (rd_idx == wr_idx) {
		/* The buffer is empty. */
		return 0;
	}

	uint32_t bytes_stored = idx_occupied(iblen, wr_idx, rd_idx);

	/* Read message len. */
	sys_cache_data_range(&ib->data[rd_idx], sizeof(ib->data[rd_idx]), K_CACHE_INVD);
	uint16_t mlen = ib->data[rd_idx];

	rd_idx = idx_cut(iblen, rd_idx + sizeof(ib->data[rd_idx]));

	sys_cache_data_range(&ib->data[rd_idx], sizeof(ib->data[rd_idx]), K_CACHE_INVD);
	mlen |= (ib->data[rd_idx] << 8);
	rd_idx = idx_cut(iblen, rd_idx + sizeof(ib->data[rd_idx]));

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
	uint32_t sz = MIN(len, iblen - rd_idx);

	sys_cache_data_range(&ib->data[rd_idx], sz, K_CACHE_INVD);
	memcpy(buf, &ib->data[rd_idx], sz);
	if (len > sz) {
		/* Read remaining bytes starting from the buffer head. */
		sys_cache_data_range(&ib->data[0], len - sz, K_CACHE_INVD);
		memcpy(&buf[sz], &ib->data[0], len - sz);
	}

	/* Update read index - make other side aware data was read. */
	__sync_synchronize();
	rd_idx = idx_cut(iblen, rd_idx + len);
	ib->rd_idx = rd_idx;

	sys_cache_data_range(ib, sizeof(*ib), K_CACHE_WB);

	return len;
}
