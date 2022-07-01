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
#include <zephyr/sys/byteorder.h>

#define LEN_SZ sizeof(uint32_t)
#define PADDING_MARK 0xFF

#define GET_UTILIZATION(flags) \
	(((flags) >> SPSC_PBUF_UTILIZATION_OFFSET) & BIT_MASK(SPSC_PBUF_UTILIZATION_BITS))

#define SET_UTILIZATION(flags, val) \
	((flags & ~(BIT_MASK(SPSC_PBUF_UTILIZATION_BITS) << \
			     SPSC_PBUF_UTILIZATION_OFFSET)) | \
			((val) << SPSC_PBUF_UTILIZATION_OFFSET))

/*
 * In order to allow allocation of continuous buffers (in zero copy manner) buffer
 * is handling wrapping. When it is detected that request space cannot be allocated
 * at the end of the buffer but it is available at the beginning, a padding must
 * be added. Padding is marked using 0xFF byte. Packet length is stored on 2 bytes
 * but padding marker must be byte long as it is possible that only 1 byte padding
 * is required. In order to distinguish padding marker from length field following
 * measures are taken: Length is stored in big endian (MSB byte first). Maximum
 * packet length is limited to 0XFEFF.
 */

/* Helpers */
static uint32_t idx_occupied(uint32_t len, uint32_t a, uint32_t b)
{
	/* It is implicitly assumed a and b cannot differ by more then len. */
	return (b > a) ? (len - (b - a)) : (a - b);
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

static uint32_t *get_rd_idx_loc(struct spsc_pbuf *pb, uint32_t flags)
{
	return &pb->common.rd_idx;
}

static uint32_t *get_wr_idx_loc(struct spsc_pbuf *pb, uint32_t flags)
{
	if (IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_ALWAYS) ||
	    (IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_FLAG) && (flags & SPSC_PBUF_CACHE))) {
		return &pb->ext.cache.wr_idx;
	}

	return &pb->ext.nocache.wr_idx;
}

static uint8_t *get_data_loc(struct spsc_pbuf *pb, uint32_t flags)
{
	if (IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_ALWAYS) ||
	    (IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_FLAG) && (flags & SPSC_PBUF_CACHE))) {
		return pb->ext.cache.data;
	}

	return pb->ext.nocache.data;
}

static uint32_t get_len(size_t blen, uint32_t flags)
{
	uint32_t len = blen - sizeof(struct spsc_pbuf_common);

	if (IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_ALWAYS) ||
	    (IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_FLAG) && (flags & SPSC_PBUF_CACHE))) {
		return len - sizeof(struct spsc_pbuf_ext_cache);
	}

	return len - sizeof(struct spsc_pbuf_ext_nocache);
}

static bool check_alignment(void *buf, uint32_t flags)
{
	if ((CONFIG_SPSC_PBUF_CACHE_LINE > 0) && (IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_ALWAYS) ||
	    (IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_FLAG) && (flags & SPSC_PBUF_CACHE)))) {
		return ((uintptr_t)buf & (CONFIG_SPSC_PBUF_CACHE_LINE - 1)) == 0;
	}

	return (((uintptr_t)buf & (sizeof(uint32_t) - 1)) == 0) ? true : false;
}

struct spsc_pbuf *spsc_pbuf_init(void *buf, size_t blen, uint32_t flags)
{
	if (!check_alignment(buf, flags)) {
		__ASSERT(false, "Failed to initialize due to memory misalignment");
		return NULL;
	}

	/* blen must be big enough to contain spsc_pbuf struct, byte of data
	 * and message len (2 bytes).
	 */
	struct spsc_pbuf *pb = buf;
	uint32_t *wr_idx_loc = get_wr_idx_loc(pb, flags);

	__ASSERT_NO_MSG(blen > (sizeof(*pb) + LEN_SZ));

	pb->common.len = get_len(blen, flags);
	pb->common.rd_idx = 0;
	pb->common.flags = flags;
	*wr_idx_loc = 0;

	__sync_synchronize();
	cache_wb(&pb->common, sizeof(pb->common), flags);
	cache_wb(wr_idx_loc, sizeof(*wr_idx_loc), flags);

	return pb;
}

int spsc_pbuf_alloc(struct spsc_pbuf *pb, uint16_t len, char **buf)
{
	/* Length of the buffer and flags are immutable - avoid reloading. */
	const uint32_t pblen = pb->common.len;
	const uint32_t flags = pb->common.flags;
	uint32_t *rd_idx_loc = get_rd_idx_loc(pb, flags);
	uint32_t *wr_idx_loc = get_wr_idx_loc(pb, flags);
	uint8_t *data_loc = get_data_loc(pb, flags);

	uint32_t space = len + LEN_SZ; /* data + length field */

	if (len == 0 || len > SPSC_PBUF_MAX_LEN) {
		/* Incorrect call. */
		return -EINVAL;
	}

	cache_inv(rd_idx_loc, sizeof(*rd_idx_loc), flags);
	__sync_synchronize();

	uint32_t wr_idx = *wr_idx_loc;
	uint32_t rd_idx = *rd_idx_loc;
	int32_t free_space;

	if (wr_idx >= rd_idx) {
		int32_t remaining = pblen - wr_idx;
		/* If SPSC_PBUF_MAX_LEN is set as length try to allocate maximum
		 * possible packet till wrap or from the beginning.
		 * If len is bigger than SPSC_PBUF_MAX_LEN then try to allocate
		 * maximum packet length even if that results in adding a padding.
		 */
		if (len == SPSC_PBUF_MAX_LEN) {
			/* At least space for 1 byte packet. */
			space = LEN_SZ + 1;
		}

		if (remaining >= space) {
			/* Packet will fit at the end */
			free_space = remaining - ((rd_idx > 0) ? 0 : sizeof(uint32_t));
		} else {
			if (rd_idx > remaining) {
				/* Padding must be added. */
				data_loc[wr_idx] = PADDING_MARK;
				__sync_synchronize();
				cache_wb(&data_loc[wr_idx], sizeof(uint8_t), flags);

				wr_idx = 0;
				*wr_idx_loc = wr_idx;

				free_space = rd_idx - sizeof(uint32_t);
			} else {
				free_space = remaining - (rd_idx > 0 ? 0 : sizeof(uint32_t));
			}
		}
	} else {
		free_space = rd_idx - wr_idx - 1;
	}

	len = MIN(len, MAX(free_space - (int32_t)LEN_SZ, 0));
	*buf = &data_loc[wr_idx + LEN_SZ];

	return len;
}

void spsc_pbuf_commit(struct spsc_pbuf *pb, uint16_t len)
{
	if (len == 0) {
		return;
	}

	/* Length of the buffer and flags are immutable - avoid reloading. */
	const uint32_t pblen = pb->common.len;
	const uint32_t flags = pb->common.flags;
	uint32_t *wr_idx_loc = get_wr_idx_loc(pb, flags);
	uint8_t *data_loc = get_data_loc(pb, flags);

	uint32_t wr_idx = *wr_idx_loc;

	sys_put_be16(len, &data_loc[wr_idx]);
	__sync_synchronize();
	cache_wb(&data_loc[wr_idx], len + LEN_SZ, flags);

	wr_idx += len + LEN_SZ;
	wr_idx = ROUND_UP(wr_idx, sizeof(uint32_t));
	wr_idx = wr_idx == pblen ? 0 : wr_idx;

	*wr_idx_loc = wr_idx;
	__sync_synchronize();
	cache_wb(wr_idx_loc, sizeof(*wr_idx_loc), flags);
}

int spsc_pbuf_write(struct spsc_pbuf *pb, const char *buf, uint16_t len)
{
	char *pbuf;
	int outlen;

	if (len >= SPSC_PBUF_MAX_LEN) {
		return -EINVAL;
	}

	outlen = spsc_pbuf_alloc(pb, len, &pbuf);
	if (outlen != len) {
		return outlen < 0 ? outlen : -ENOMEM;
	}

	memcpy(pbuf, buf, len);

	spsc_pbuf_commit(pb, len);

	return len;
}

uint16_t spsc_pbuf_claim(struct spsc_pbuf *pb, char **buf)
{
	/* Length of the buffer and flags are immutable - avoid reloading. */
	const uint32_t pblen = pb->common.len;
	const uint32_t flags = pb->common.flags;
	uint32_t *rd_idx_loc = get_rd_idx_loc(pb, flags);
	uint32_t *wr_idx_loc = get_wr_idx_loc(pb, flags);
	uint8_t *data_loc = get_data_loc(pb, flags);

	cache_inv(wr_idx_loc, sizeof(*wr_idx_loc), flags);
	__sync_synchronize();

	uint32_t wr_idx = *wr_idx_loc;
	uint32_t rd_idx = *rd_idx_loc;

	if (rd_idx == wr_idx) {
		return 0;
	}

	uint32_t bytes_stored = idx_occupied(pblen, wr_idx, rd_idx);

	/* Utilization is calculated at claiming to handle cache case when flags
	 * and rd_idx is in the same cache line thus it should be modified only
	 * by the consumer.
	 */
	if (IS_ENABLED(CONFIG_SPSC_PBUF_UTILIZATION) && (bytes_stored > GET_UTILIZATION(flags))) {
		__ASSERT_NO_MSG(bytes_stored <= BIT_MASK(SPSC_PBUF_UTILIZATION_BITS));
		pb->common.flags = SET_UTILIZATION(flags, bytes_stored);
		__sync_synchronize();
		cache_wb(&pb->common.flags, sizeof(pb->common.flags), flags);
	}

	/* Read message len. */
	uint16_t len;

	cache_inv(&data_loc[rd_idx], LEN_SZ, flags);
	if (data_loc[rd_idx] == PADDING_MARK) {
		rd_idx = 0;
		*rd_idx_loc = rd_idx;
		__sync_synchronize();
		cache_wb(rd_idx_loc, sizeof(*rd_idx_loc), flags);
		/* After reading padding we may find out that buffer is empty. */
		if (rd_idx == wr_idx) {
			return 0;
		}

		cache_inv(&data_loc[rd_idx], sizeof(len), flags);
	}

	len = sys_get_be16(&data_loc[rd_idx]);

	(void)bytes_stored;
	__ASSERT_NO_MSG(bytes_stored >= (len + LEN_SZ));

	cache_inv(&data_loc[rd_idx + LEN_SZ], len, flags);
	*buf = &data_loc[rd_idx + LEN_SZ];

	return len;
}

void spsc_pbuf_free(struct spsc_pbuf *pb, uint16_t len)
{
	/* Length of the buffer and flags are immutable - avoid reloading. */
	const uint32_t pblen = pb->common.len;
	const uint32_t flags = pb->common.flags;
	uint32_t *rd_idx_loc = get_rd_idx_loc(pb, flags);
	uint16_t rd_idx = *rd_idx_loc + len + LEN_SZ;
	uint8_t *data_loc = get_data_loc(pb, flags);

	rd_idx = ROUND_UP(rd_idx, sizeof(uint32_t));
	cache_inv(&data_loc[rd_idx], sizeof(uint8_t), flags);
	/* Handle wrapping or the fact that next packet is a padding. */
	if (rd_idx == pblen || data_loc[rd_idx] == PADDING_MARK) {
		rd_idx = 0;
	}

	*rd_idx_loc = rd_idx;
	__sync_synchronize();
	cache_wb(&rd_idx_loc, sizeof(*rd_idx_loc), flags);
}

int spsc_pbuf_read(struct spsc_pbuf *pb, char *buf, uint16_t len)
{
	char *pkt;
	uint16_t plen = spsc_pbuf_claim(pb, &pkt);

	if (plen == 0) {
		return 0;
	}

	if (buf == NULL) {
		return plen;
	}

	if (len < plen) {
		return -ENOMEM;
	}

	memcpy(buf, pkt, plen);

	spsc_pbuf_free(pb, plen);

	return plen;
}

int spsc_pbuf_get_utilization(struct spsc_pbuf *pb)
{
	if (!IS_ENABLED(CONFIG_SPSC_PBUF_UTILIZATION)) {
		return -ENOTSUP;
	}

	cache_inv(&pb->common.flags, sizeof(pb->common.flags), pb->common.flags);
	__sync_synchronize();

	return GET_UTILIZATION(pb->common.flags);
}
