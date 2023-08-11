/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/cache.h>
#include <zephyr/ipc/icmsg_pbuf.h>
#include <zephyr/sys/byteorder.h>

/* Helper funciton for getting numer of bytes being written to the bufer. */
static uint32_t idx_occupied(uint32_t len, uint32_t wr_idx, uint32_t rd_idx)
{
	/* It is implicitly assumed wr_idx and rd_idx cannot differ by more then len. */
	return (rd_idx > wr_idx) ? (len - (rd_idx - wr_idx)) : (wr_idx - rd_idx);
}

/* Helper function for wrapping the index from the begging if above buffer len. */
static uint32_t idx_wrap(uint32_t len, uint32_t idx)
{
	return (idx >= len) ? (idx % len) : (idx);
}

static int validate_cfg(const struct icmsg_pbuf_cfg *cfg)
{
	/* Validate pointers. */
	if (!cfg || !cfg->rd_idx_loc || !cfg->wr_idx_loc || !cfg->data_loc) {
		return -EINVAL;
	}

	/* Validate pointer alignement. */
	if (!IS_ALIGNED(cfg->rd_idx_loc, MAX(cfg->cache_line_sz, _IDX_SIZE)) ||
	    !IS_ALIGNED(cfg->wr_idx_loc, MAX(cfg->cache_line_sz, _IDX_SIZE)) ||
	    !IS_ALIGNED(cfg->data_loc, _IDX_SIZE)) {
		return -EINVAL;
	}

	/* Validate len. */
	if (cfg->len < _MIN_DATA_LEN || !IS_ALIGNED(cfg->len, _IDX_SIZE)) {
		return -EINVAL;
	}

	/* Validate pointer values. */
	if (!(cfg->rd_idx_loc < cfg->wr_idx_loc) ||
	    !((uint8_t *)cfg->wr_idx_loc < cfg->data_loc) ||
	    !(((uint8_t *)cfg->rd_idx_loc + MAX(_IDX_SIZE, cfg->cache_line_sz)) == (uint8_t *)cfg->wr_idx_loc)) {
		return -EINVAL;
	}

	return 0;
}

int icmsg_pbuf_init(struct icmsg_pbuf *ib)
{
	if (validate_cfg(ib->cfg) != 0) {
		return -EINVAL;
	}

	if (ib->data == NULL) {
		return -EINVAL;
	}

	/* Initialize local copy of indexes. */
	ib->data->wr_idx = 0;
	ib->data->rd_idx = 0;

	/* Clear shared memory. */
	*(ib->cfg->wr_idx_loc) = ib->data->wr_idx;
	*(ib->cfg->rd_idx_loc) = ib->data->rd_idx;

	__sync_synchronize();

	/* Take care cache. */
	sys_cache_data_flush_range((void*)(ib->cfg->wr_idx_loc), sizeof(*(ib->cfg->wr_idx_loc)));
	sys_cache_data_flush_range((void*)(ib->cfg->rd_idx_loc), sizeof(*(ib->cfg->rd_idx_loc)));

	return 0;
}

int icmsg_pbuf_write(struct icmsg_pbuf *ib, const char *data, uint16_t len)
{
	if (ib == NULL || len == 0 || data == NULL) {
		/* Incorrect call. */
		return -EINVAL;
	}

	/* Invalidate rd_idx only, local wr_idx is used to increase buffer security. */
	sys_cache_data_invd_range((void*)(ib->cfg->rd_idx_loc), sizeof(*(ib->cfg->rd_idx_loc)));
	__sync_synchronize();

	uint8_t *const data_loc = ib->cfg->data_loc;
	const uint32_t blen = ib->cfg->len;
	uint32_t rd_idx = *(ib->cfg->rd_idx_loc);
	uint32_t wr_idx = ib->data->wr_idx;

	/* wr_idx must always be aligned. */
	__ASSERT_NO_MSG(IS_ALIGNED(wr_idx, _IDX_SIZE));
	/* rd_idx shall always be aligned, but its value is received from the reader.
	 * Can not assert.
	 */
	if (!IS_ALIGNED(rd_idx, _IDX_SIZE)) {
		return -EINVAL;
	}

	uint32_t free_space = blen - idx_occupied(blen, wr_idx, rd_idx) - _IDX_SIZE;

	/* Packet length, data + packet length size. */
	uint32_t plen = len + ICMSG_PBUF_PACKET_LEN_SZ;

	/* Check if packet will fit into the buffer. */
	if (free_space < plen) {
		return -ENOMEM;
	}

	/* Write packet len. It is allowed, because shared wr_idx value is updated at the very end. */
	sys_put_be16(len, &data_loc[wr_idx]);
	__sync_synchronize();
	sys_cache_data_flush_range(&data_loc[wr_idx], ICMSG_PBUF_PACKET_LEN_SZ);

	wr_idx = idx_wrap(blen, wr_idx + ICMSG_PBUF_PACKET_LEN_SZ);

	/* Write until end of the buffer, if data will be wrapped. */
	uint32_t tail = MIN(len, blen - wr_idx);

	memcpy(&data_loc[wr_idx], data, tail);
	sys_cache_data_flush_range(&data_loc[wr_idx], tail);

	if (len > tail) {
		/* Copy remaining data to buffer front. */
		memcpy(&data_loc[0], data + tail, len - tail);
		sys_cache_data_flush_range(&data_loc[0], len - tail);
	}

	wr_idx = idx_wrap(blen, ROUND_UP(wr_idx + len, _IDX_SIZE));
	/* Update wr_idx. */
	ib->data->wr_idx = wr_idx;
	*(ib->cfg->wr_idx_loc) = wr_idx;
	__sync_synchronize();
	sys_cache_data_flush_range((void *)ib->cfg->wr_idx_loc, sizeof(*(ib->cfg->wr_idx_loc)));

	return len;
}

int icmsg_pbuf_read(struct icmsg_pbuf *ib, char *buf, uint16_t len)
{
	if (ib == NULL) {
		/* Incorrect call. */
		return -EINVAL;
	}

	/* Invalidate wr_idx only, local rd_idx is used to increase buffer security. */
	sys_cache_data_invd_range((void*)(ib->cfg->wr_idx_loc), sizeof(*(ib->cfg->wr_idx_loc)));
	__sync_synchronize();

	uint8_t *const data_loc = ib->cfg->data_loc;
	const uint32_t blen = ib->cfg->len;
	uint32_t wr_idx = *(ib->cfg->wr_idx_loc);
	uint32_t rd_idx = ib->data->rd_idx;

	/* rd_idx must always be aligned. */
	__ASSERT_NO_MSG(IS_ALIGNED(rd_idx, _IDX_SIZE));
	/* wr_idx shall always be aligned, but its value is received from the writer.
	 * Can not assert.
	 */
	if (!IS_ALIGNED(wr_idx, _IDX_SIZE)) {
		return -EINVAL;
	}

	if (rd_idx == wr_idx) {
		/* Buffer is empty. */
		return 0;
	}

	/* Get packet len.*/
	sys_cache_data_invd_range(&data_loc[rd_idx], ICMSG_PBUF_PACKET_LEN_SZ);
	uint16_t plen = sys_get_be16(&data_loc[rd_idx]);

	if (!buf) {
		return (int)plen;
	}

	if (plen < len) {
		return -ENOMEM;
	}

	uint32_t occupied_space = idx_occupied(blen, wr_idx, rd_idx);

	if (occupied_space < plen + ICMSG_PBUF_PACKET_LEN_SZ) {
		/* This should never happen. */
		return -EAGAIN;
	}

	rd_idx = idx_wrap(blen, rd_idx + ICMSG_PBUF_PACKET_LEN_SZ);

	/* Packet will fit into provided buffer, truncate len if provided len is
	 * bigger than necessary.
	 */
	len = MIN(plen, len);

	/* Read until end of the buffer, if data are wrapped. */
	uint32_t tail = MIN(blen - rd_idx, len);

	sys_cache_data_invd_range(&data_loc[rd_idx], tail);
	memcpy(buf, &ib->cfg->data_loc, tail);

	if (len > tail) {
		sys_cache_data_invd_range(&data_loc[0], len - tail);
		memcpy(&buf[tail], &ib->cfg->data_loc[0], len - tail);
	}

	/* Update rd_idx. */
	rd_idx = idx_wrap(blen, ROUND_UP(rd_idx + len, _IDX_SIZE));

	ib->data->rd_idx = rd_idx;
	*(ib->cfg->rd_idx_loc) = rd_idx;
	__sync_synchronize();
	sys_cache_data_flush_range((void *)ib->cfg->rd_idx_loc, sizeof(*(ib->cfg->rd_idx_loc)));

	return len;
}
