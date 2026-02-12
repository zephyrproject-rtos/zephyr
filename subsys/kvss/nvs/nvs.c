/*  NVS: non volatile storage in flash
 *
 * Copyright (c) 2018 Laczen
 * Copyright (c) 2026 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/flash.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <zephyr/kvss/nvs.h>
#include <zephyr/sys/crc.h>
#include "nvs_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fs_nvs, CONFIG_NVS_LOG_LEVEL);

#ifdef CONFIG_ZTEST
#define STATIC
#else
#define STATIC static
#endif

static int nvs_prev_ate(struct nvs_fs *fs, uint32_t *addr, struct nvs_ate *ate);
static int nvs_ate_valid(struct nvs_fs *fs, uint16_t entry_addr,
			 const struct nvs_ate *entry);

#ifdef CONFIG_NVS_LOOKUP_CACHE

static inline size_t nvs_lookup_cache_pos(uint16_t id)
{
	uint16_t hash;

	/* 16-bit integer hash function found by https://github.com/skeeto/hash-prospector. */
	hash = id;
	hash ^= hash >> 8;
	hash *= 0x88b5U;
	hash ^= hash >> 7;
	hash *= 0xdb2dU;
	hash ^= hash >> 9;

	return hash % CONFIG_NVS_LOOKUP_CACHE_SIZE;
}

static int nvs_lookup_cache_rebuild(struct nvs_fs *fs)
{
	int rc;
	uint32_t addr, ate_addr;
	uint32_t *cache_entry;
	struct nvs_ate ate;

	memset(fs->lookup_cache, 0xff, sizeof(fs->lookup_cache));
	addr = fs->ate_wra;

	while (true) {
		/* Make a copy of 'addr' as it will be advanced by nvs_pref_ate() */
		ate_addr = addr;
		rc = nvs_prev_ate(fs, &addr, &ate);

		if (rc) {
			return rc;
		}

		cache_entry = &fs->lookup_cache[nvs_lookup_cache_pos(ate.id)];

		if (ate.id != 0xFFFF && *cache_entry == NVS_LOOKUP_CACHE_NO_ADDR &&
		    nvs_ate_valid(fs, ate_addr, &ate)) {
			*cache_entry = ate_addr;
		}

		if (addr == fs->ate_wra) {
			break;
		}
	}

	return 0;
}

static void nvs_lookup_cache_invalidate(struct nvs_fs *fs, uint32_t sector)
{
	uint32_t *cache_entry = fs->lookup_cache;
	uint32_t *const cache_end = &fs->lookup_cache[CONFIG_NVS_LOOKUP_CACHE_SIZE];

	for (; cache_entry < cache_end; ++cache_entry) {
		if ((*cache_entry >> ADDR_SECT_SHIFT) == sector) {
			*cache_entry = NVS_LOOKUP_CACHE_NO_ADDR;
		}
	}
}

#endif /* CONFIG_NVS_LOOKUP_CACHE */

/* basic routines */
/* nvs_al_size returns size aligned to fs->write_block_size */
static inline size_t nvs_al_size(struct nvs_fs *fs, size_t len)
{
	size_t write_block_size = fs->flash_parameters->write_block_size;

	if (write_block_size <= 1U) {
		return len;
	}
	return (len + (write_block_size - 1U)) & ~(write_block_size - 1U);
}
/* end basic routines */

/* flash routines */

/* Write the data described by @p strm to flash at the given NVS address,
 * respecting the flash write block alignment requirements. The write
 * may include a header, primary data, and a tail. All buffers are
 * written as a single contiguous stream.
 */
STATIC int nvs_flash_al_wrt_streams(struct nvs_fs *fs, uint32_t addr,
				    const struct nvs_flash_wrt_stream *strm)
{
	const struct flash_parameters *fp = fs->flash_parameters;
	size_t wbs = fp->write_block_size;
	uint8_t buf[NVS_BLOCK_SIZE];
	size_t stream_idx = 0U;
	size_t full_bytes = 0U;
	size_t buf_fill = 0U;
	size_t copy = 0U;
	off_t offset;
	int rc;

	/* Nothing to write */
	if ((strm->head.len + strm->data.len + strm->tail.len) == 0U) {
		return 0;
	}

	/* Convert NVS address to flash offset */
	offset = fs->offset;
	offset += fs->sector_size * (addr >> ADDR_SECT_SHIFT);
	offset += addr & ADDR_OFFS_MASK;

	/* Logical write stream: head -> data -> tail */
	struct nvs_flash_buf streams[] = {
		strm->head,
		strm->data,
		strm->tail,
	};

	while (stream_idx < ARRAY_SIZE(streams)) {
		if (streams[stream_idx].len == 0U) {
			stream_idx++;
			continue;
		}

		/* Direct write of aligned full blocks */
		if (buf_fill == 0) {
			/* number of full blocks = len & ~(wbs - 1) */
			full_bytes = streams[stream_idx].len & ~(wbs - 1);

			if (full_bytes > 0U) {
				rc = flash_write(fs->flash_device, offset,
						 streams[stream_idx].ptr,
						 full_bytes);
				if (rc) {
					return rc;
				}

				streams[stream_idx].ptr += full_bytes;
				streams[stream_idx].len -= full_bytes;
				offset += full_bytes;
				continue;
			}
		}

		/* Copy to buffer to assemble a full block */
		copy = MIN(wbs - buf_fill, streams[stream_idx].len);
		if (copy > 0U) {
			(void)memcpy(buf + buf_fill, streams[stream_idx].ptr, copy);

			streams[stream_idx].ptr += copy;
			streams[stream_idx].len -= copy;
			buf_fill += copy;
		}

		/* If buffer full, write to flash */
		if (buf_fill == wbs) {
			rc = flash_write(fs->flash_device, offset, buf, wbs);
			if (rc) {
				return rc;
			}

			offset += wbs;
			buf_fill = 0U;
		}
	}


	if (buf_fill > 0U) {
		(void)memset(buf + buf_fill, fp->erase_value, wbs - buf_fill);

		rc = flash_write(fs->flash_device, offset, buf, wbs);
		if (rc) {
			return rc;
		}
	}

	return 0;
}

static int nvs_flash_al_wrt(struct nvs_fs *fs, uint32_t addr, const void *data,
			    size_t len)
{
	struct nvs_flash_wrt_stream strm = {
		.data = {
			.ptr = data,
			.len = len,
		},
	};

	return nvs_flash_al_wrt_streams(fs, addr, &strm);
}

/* basic flash read from nvs address */
static int nvs_flash_rd(struct nvs_fs *fs, uint32_t addr, void *data,
			 size_t len)
{
	int rc;
	off_t offset;

	offset = fs->offset;
	offset += fs->sector_size * (addr >> ADDR_SECT_SHIFT);
	offset += addr & ADDR_OFFS_MASK;

	rc = flash_read(fs->flash_device, offset, data, len);
	return rc;
}

/* allocation entry write */
static int nvs_flash_ate_wrt(struct nvs_fs *fs, const struct nvs_ate *entry)
{
	int rc;

	rc = nvs_flash_al_wrt(fs, fs->ate_wra, entry,
			       sizeof(struct nvs_ate));
#ifdef CONFIG_NVS_LOOKUP_CACHE
	/* 0xFFFF is a special-purpose identifier. Exclude it from the cache */
	if (entry->id != 0xFFFF) {
		fs->lookup_cache[nvs_lookup_cache_pos(entry->id)] = fs->ate_wra;
	}
#endif
	fs->ate_wra -= nvs_al_size(fs, sizeof(struct nvs_ate));

	return rc;
}

/* data write */
static int nvs_flash_data_al_wrt(struct nvs_fs *fs,
				 struct nvs_flash_wrt_stream *strm,
				 bool compute_crc)
{
	uint32_t data_crc;
	int rc;

	/* Only add the CRC if required (ignore deletion requests, i.e. when len is 0) */
	if (IS_ENABLED(CONFIG_NVS_DATA_CRC) && compute_crc && (strm->data.len > 0)) {
		data_crc = crc32_ieee(strm->data.ptr, strm->data.len);

		strm->tail.ptr = (void *)&data_crc;
		strm->tail.len = sizeof(data_crc);
	}

	rc = nvs_flash_al_wrt_streams(fs, fs->data_wra, strm);

	fs->data_wra += nvs_al_size(fs, strm->head.len + strm->data.len + strm->tail.len);

	return rc;
}

static int nvs_flash_data_wrt(struct nvs_fs *fs, const void *data, size_t len,
			      bool compute_crc)
{
	struct nvs_flash_wrt_stream strm = {
		.data = {
			.ptr = data,
			.len = len,
		},
	};

	return nvs_flash_data_al_wrt(fs, &strm, compute_crc);
}

/* flash ate read */
static int nvs_flash_ate_rd(struct nvs_fs *fs, uint32_t addr,
			     struct nvs_ate *entry)
{
	return nvs_flash_rd(fs, addr, entry, sizeof(struct nvs_ate));
}

/* end of basic flash routines */

/* advanced flash routines */

/* nvs_flash_block_cmp compares the data in flash at addr to data
 * in blocks of size NVS_BLOCK_SIZE aligned to fs->write_block_size
 * returns 0 if equal, 1 if not equal, errcode if error
 */
static int nvs_flash_block_cmp(struct nvs_fs *fs, uint32_t addr, const void *data,
				size_t len)
{
	const uint8_t *data8 = (const uint8_t *)data;
	int rc;
	size_t bytes_to_cmp, block_size;
	uint8_t buf[NVS_BLOCK_SIZE];

	block_size =
		NVS_BLOCK_SIZE & ~(fs->flash_parameters->write_block_size - 1U);

	while (len) {
		bytes_to_cmp = MIN(block_size, len);
		rc = nvs_flash_rd(fs, addr, buf, bytes_to_cmp);
		if (rc) {
			return rc;
		}
		rc = memcmp(data8, buf, bytes_to_cmp);
		if (rc) {
			return 1;
		}
		len -= bytes_to_cmp;
		addr += bytes_to_cmp;
		data8 += bytes_to_cmp;
	}
	return 0;
}

/* nvs_flash_cmp_const compares the data in flash at addr to a constant
 * value. returns 0 if all data in flash is equal to value, 1 if not equal,
 * errcode if error
 */
static int nvs_flash_cmp_const(struct nvs_fs *fs, uint32_t addr, uint8_t value,
				size_t len)
{
	int rc;
	size_t bytes_to_cmp, block_size;
	uint8_t buf[NVS_BLOCK_SIZE];

	block_size =
		NVS_BLOCK_SIZE & ~(fs->flash_parameters->write_block_size - 1U);

	while (len) {
		bytes_to_cmp = MIN(block_size, len);
		rc = nvs_flash_rd(fs, addr, buf, bytes_to_cmp);
		if (rc) {
			return rc;
		}

		for (size_t i = 0; i < bytes_to_cmp; i++) {
			if (buf[i] != value) {
				return 1;
			}
		}

		len -= bytes_to_cmp;
		addr += bytes_to_cmp;
	}
	return 0;
}

/* flash block move (GC-only helper)
 *
 * Move data starting at @addr to the current data write location during GC.
 *
 * This function writes data in write_block_size-aligned chunks only.
 * If the total length is not aligned to write_block_size, the tail bytes
 * (less than one write block) are read into @buf but NOT written immediately.
 *
 * The number of buffered but unwritten bytes is returned via @ctx->buffer_pos.
 * The caller is responsible for flushing these remaining bytes later
 * (typically before writing the corresponding ATE).
 *
 * Notes:
 * - This function is intended to be used ONLY during GC.
 * - @ctx->buffer_pos is guaranteed to be < write_block_size.
 * - No padding or alignment is added to the data.
 */
static int nvs_flash_block_move(struct nvs_fs *fs, uint32_t addr, struct nvs_block_move_ctx *ctx,
				struct nvs_ate *gc_ate)
{
	size_t wbs = fs->flash_parameters->write_block_size;
	size_t bytes_to_copy, block_size, tail_len;
	size_t len = gc_ate->len;
	int rc;

	/* Include any buffered tail bytes in the length */
	len += ctx->buffer_pos;

	/*
	 * Split the total length into:
	 * - a multiple of write_block_size (block_size)
	 * - a remaining tail smaller than write_block_size (tail_len)
	 */
	tail_len = len & (wbs - 1U);
	block_size = NVS_BLOCK_SIZE & ~(wbs - 1U);
	len -= tail_len;

	/* Update ATE offset to the new data location.
	 * ctx.buffer_pos accounts for any buffered but unwritten
	 * data carried over from previous moves.
	 */
	gc_ate->offset = (uint16_t)((fs->data_wra + ctx->buffer_pos)
				    & ADDR_OFFS_MASK);

	/* Copy and write only write_block_size-aligned data.
	 * Any previously buffered bytes (ctx->buffer_pos) are prepended
	 * to the newly read data to form a full aligned write.
	 */
	while (len) {
		bytes_to_copy = MIN(block_size, len) - ctx->buffer_pos;

		rc = nvs_flash_rd(fs, addr, ctx->buffer + ctx->buffer_pos, bytes_to_copy);
		if (rc) {
			return rc;
		}

		/* Just rewrite the whole record, no need to recompute the CRC as the data
		 * did not change
		 */
		rc = nvs_flash_data_wrt(fs, ctx->buffer, bytes_to_copy + ctx->buffer_pos, false);
		if (rc) {
			return rc;
		}

		len  -= bytes_to_copy + ctx->buffer_pos;
		addr += bytes_to_copy;
		ctx->buffer_pos = 0U;
	}

	/* Read the remaining unaligned tail into buffer.
	 * This data is not written now and will be combined with the next data block.
	 */
	if (tail_len) {
		rc = nvs_flash_rd(fs, addr, ctx->buffer + ctx->buffer_pos,
				  tail_len - ctx->buffer_pos);
		if (rc) {
			return rc;
		}

		ctx->buffer_pos = tail_len;
	}

	return 0;
}

/* erase a sector and verify erase was OK.
 * return 0 if OK, errorcode on error.
 */
static int nvs_flash_erase_sector(struct nvs_fs *fs, uint32_t addr)
{
	int rc;
	off_t offset;

	addr &= ADDR_SECT_MASK;

	offset = fs->offset;
	offset += fs->sector_size * (addr >> ADDR_SECT_SHIFT);

	LOG_DBG("Erasing flash at %lx, len %d", (long int) offset,
		fs->sector_size);

#ifdef CONFIG_NVS_LOOKUP_CACHE
	nvs_lookup_cache_invalidate(fs, addr >> ADDR_SECT_SHIFT);
#endif
	rc = flash_flatten(fs->flash_device, offset, fs->sector_size);

	if (rc) {
		return rc;
	}

	if (nvs_flash_cmp_const(fs, addr, fs->flash_parameters->erase_value,
			fs->sector_size)) {
		rc = -ENXIO;
	}

	return rc;
}

/* crc update on allocation entry */
static void nvs_ate_crc8_update(struct nvs_ate *entry)
{
	uint8_t crc8;

	crc8 = crc8_ccitt(0xff, entry, offsetof(struct nvs_ate, crc8));
	entry->crc8 = crc8;
}

/* crc check on allocation entry
 * returns 0 if OK, 1 on crc fail
 */
static int nvs_ate_crc8_check(const struct nvs_ate *entry)
{
	uint8_t crc8;

	crc8 = crc8_ccitt(0xff, entry, offsetof(struct nvs_ate, crc8));
	if (crc8 == entry->crc8) {
		return 0;
	}
	return 1;
}

/* nvs_ate_cmp_const compares an ATE to a constant value. returns 0 if
 * the whole ATE is equal to value, 1 if not equal.
 */
static int nvs_ate_cmp_const(const struct nvs_ate *entry, uint8_t value)
{
	const uint8_t *data8 = (const uint8_t *)entry;
	int i;

	for (i = 0; i < sizeof(struct nvs_ate); i++) {
		if (data8[i] != value) {
			return 1;
		}
	}

	return 0;
}

/* nvs_ate_valid validates an ate:
 *     return 1 if crc8, offset and length are valid,
 *            0 otherwise
 */
static int nvs_ate_valid(struct nvs_fs *fs, uint16_t entry_addr,
			 const struct nvs_ate *entry)
{
	uint32_t position;

	position = entry->offset + entry->len;

	if ((nvs_ate_crc8_check(entry)) ||
	    (position > (entry_addr & ADDR_OFFS_MASK))) {
		return 0;
	}

	return 1;
}

/* nvs_close_ate_valid validates an sector close ate: a valid sector close ate:
 * - valid ate
 * - len = 0 and id = 0xFFFF
 * - offset points to location at ate multiple from sector size
 * return 1 if valid, 0 otherwise
 */
static int nvs_close_ate_valid(struct nvs_fs *fs, const struct nvs_ate *entry)
{
	size_t ate_size;

	ate_size = nvs_al_size(fs, sizeof(struct nvs_ate));

	if ((!nvs_ate_valid(fs, (fs->sector_size - ate_size), entry)) ||
	    (entry->len != 0U) || (entry->id != 0xFFFF)) {
		return 0;
	}

	if ((fs->sector_size - entry->offset) % ate_size) {
		return 0;
	}

	return 1;
}

/* store an entry in flash */
static int nvs_flash_wrt_entry(struct nvs_fs *fs, uint16_t id, const void *data,
				size_t len)
{
	int rc;
	struct nvs_ate entry;

	entry.id = id;
	entry.offset = (uint16_t)(fs->data_wra & ADDR_OFFS_MASK);
	entry.len = (uint16_t)len;
	entry.part = 0xff;

	rc = nvs_flash_data_wrt(fs, data, len, true);
	if (rc) {
		return rc;
	}

#ifdef CONFIG_NVS_DATA_CRC
	/* No CRC has been added if this is a deletion write request */
	if (len > 0) {
		entry.len += NVS_DATA_CRC_SIZE;
	}
#endif
	nvs_ate_crc8_update(&entry);

	rc = nvs_flash_ate_wrt(fs, &entry);

	return rc;
}
/* end of flash routines */

/* If the closing ate is invalid, its offset cannot be trusted and
 * the last valid ate of the sector should instead try to be recovered by going
 * through all ate's.
 *
 * addr should point to the faulty closing ate and will be updated to the last
 * valid ate. If no valid ate is found it will be left untouched.
 */
static int nvs_recover_last_ate(struct nvs_fs *fs, uint32_t *addr)
{
	uint32_t data_end_addr, ate_end_addr;
	struct nvs_ate end_ate;
	size_t ate_size;
	int rc;

	LOG_DBG("Recovering last ate from sector %d",
		(*addr >> ADDR_SECT_SHIFT));

	ate_size = nvs_al_size(fs, sizeof(struct nvs_ate));

	*addr -= ate_size;
	ate_end_addr = *addr;
	data_end_addr = *addr & ADDR_SECT_MASK;
	while (ate_end_addr > data_end_addr) {
		rc = nvs_flash_ate_rd(fs, ate_end_addr, &end_ate);
		if (rc) {
			return rc;
		}
		if (nvs_ate_valid(fs, ate_end_addr, &end_ate)) {
			/* found a valid ate, update data_end_addr and *addr */
			data_end_addr &= ADDR_SECT_MASK;
			data_end_addr += end_ate.offset + end_ate.len;
			*addr = ate_end_addr;
		}
		ate_end_addr -= ate_size;
	}

	return 0;
}

/* walking through allocation entry list, from newest to oldest entries
 * read ate from addr, modify addr to the previous ate
 */
static int nvs_prev_ate(struct nvs_fs *fs, uint32_t *addr, struct nvs_ate *ate)
{
	int rc;
	struct nvs_ate close_ate;
	size_t ate_size;

	ate_size = nvs_al_size(fs, sizeof(struct nvs_ate));

	rc = nvs_flash_ate_rd(fs, *addr, ate);
	if (rc) {
		return rc;
	}

	*addr += ate_size;
	if (((*addr) & ADDR_OFFS_MASK) != (fs->sector_size - ate_size)) {
		return 0;
	}

	/* last ate in sector, do jump to previous sector */
	if (((*addr) >> ADDR_SECT_SHIFT) == 0U) {
		*addr += ((fs->sector_count - 1) << ADDR_SECT_SHIFT);
	} else {
		*addr -= (1 << ADDR_SECT_SHIFT);
	}

	rc = nvs_flash_ate_rd(fs, *addr, &close_ate);
	if (rc) {
		return rc;
	}

	rc = nvs_ate_cmp_const(&close_ate, fs->flash_parameters->erase_value);
	/* at the end of filesystem */
	if (!rc) {
		*addr = fs->ate_wra;
		return 0;
	}

	/* Update the address if the close ate is valid.
	 */
	if (nvs_close_ate_valid(fs, &close_ate)) {
		(*addr) &= ADDR_SECT_MASK;
		(*addr) += close_ate.offset;
		return 0;
	}

	/* The close_ate was invalid, `lets find out the last valid ate
	 * and point the address to this found ate.
	 *
	 * remark: if there was absolutely no valid data in the sector *addr
	 * is kept at sector_end - 2*ate_size, the next read will contain
	 * invalid data and continue with a sector jump
	 */
	return nvs_recover_last_ate(fs, addr);
}

static void nvs_sector_advance(struct nvs_fs *fs, uint32_t *addr)
{
	*addr += (1 << ADDR_SECT_SHIFT);
	if ((*addr >> ADDR_SECT_SHIFT) == fs->sector_count) {
		*addr -= (fs->sector_count << ADDR_SECT_SHIFT);
	}
}

/* allocation entry close (this closes the current sector) by writing offset
 * of last ate to the sector end.
 */
static int nvs_sector_close(struct nvs_fs *fs)
{
	struct nvs_ate close_ate;
	size_t ate_size;

	ate_size = nvs_al_size(fs, sizeof(struct nvs_ate));

	close_ate.id = 0xFFFF;
	close_ate.len = 0U;
	close_ate.offset = (uint16_t)((fs->ate_wra + ate_size) & ADDR_OFFS_MASK);
	close_ate.part = 0xff;

	fs->ate_wra &= ADDR_SECT_MASK;
	fs->ate_wra += (fs->sector_size - ate_size);

	nvs_ate_crc8_update(&close_ate);

	(void)nvs_flash_ate_wrt(fs, &close_ate);

	nvs_sector_advance(fs, &fs->ate_wra);

	fs->data_wra = fs->ate_wra & ADDR_SECT_MASK;

	return 0;
}

static int nvs_add_gc_done_ate(struct nvs_fs *fs)
{
	struct nvs_ate gc_done_ate;

	LOG_DBG("Adding gc done ate at %x", fs->ate_wra & ADDR_OFFS_MASK);
	gc_done_ate.id = 0xffff;
	gc_done_ate.len = 0U;
	gc_done_ate.part = 0xff;
	gc_done_ate.offset = (uint16_t)(fs->data_wra & ADDR_OFFS_MASK);
	nvs_ate_crc8_update(&gc_done_ate);

	return nvs_flash_ate_wrt(fs, &gc_done_ate);
}

/* Attempt to write a new entry during garbage collection
 * and flush any remaining tail.
 */
static int nvs_gc_flush_and_try_write(struct nvs_fs *fs,
				      struct nvs_block_move_ctx *bm_ctx,
				      struct nvs_gc_write_entry *entry)
{
	struct nvs_flash_wrt_stream strm = {
		.head = {
			.ptr = bm_ctx->buffer,
			.len = bm_ctx->buffer_pos,
		},
	};
	size_t required_space = 0U;
	struct nvs_ate wrt_ate;
	size_t ate_size;
	int rc;

	ate_size = nvs_al_size(fs, sizeof(struct nvs_ate));

	if (entry) {
		required_space = ate_size +
			nvs_al_size(fs, bm_ctx->buffer_pos + entry->len + NVS_DATA_CRC_SIZE);
	}

	if (!entry || (fs->ate_wra < (fs->data_wra + required_space))) {
		/* Not enough space for entry, only flush buffer if needed */
		if (bm_ctx->buffer_pos > 0U) {
			rc = nvs_flash_data_al_wrt(fs, &strm, false);
			if (rc) {
				return rc;
			}
		}

		return 0;
	}

	strm.data.ptr = entry->data;
	strm.data.len = entry->len;

	/* Update ATE offset to the new data location.
	 * bm_ctx->buffer_pos accounts for any buffered but unwritten
	 * data carried over from previous moves.
	 */
	wrt_ate.offset = (uint16_t)((fs->data_wra + bm_ctx->buffer_pos)
				    & ADDR_OFFS_MASK);

	rc = nvs_flash_data_al_wrt(fs, &strm, true);
	if (rc) {
		return rc;
	}

	wrt_ate.id = entry->id;
	wrt_ate.len = entry->len + NVS_DATA_CRC_SIZE;
	wrt_ate.part = 0xff;

	nvs_ate_crc8_update(&wrt_ate);

	rc = nvs_flash_ate_wrt(fs, &wrt_ate);
	if (rc) {
		return rc;
	}

	entry->is_written = true;

	return 0;
}

/* garbage collection: the address ate_wra has been updated to the new sector
 * that has just been started. The data to gc is in the sector after this new
 * sector.
 */
static int nvs_gc(struct nvs_fs *fs, struct nvs_gc_write_entry *entry)
{
	int rc;
	struct nvs_ate close_ate, gc_ate, wlk_ate;
	uint32_t sec_addr, gc_addr, gc_prev_addr, wlk_addr, wlk_prev_addr,
	      data_addr, stop_addr;
	struct nvs_block_move_ctx ctx = {
		.buffer_pos = 0U,
	};
	size_t ate_size;

	ate_size = nvs_al_size(fs, sizeof(struct nvs_ate));

	sec_addr = (fs->ate_wra & ADDR_SECT_MASK);
	nvs_sector_advance(fs, &sec_addr);
	gc_addr = sec_addr + fs->sector_size - ate_size;

	/* if the sector is not closed don't do gc */
	rc = nvs_flash_ate_rd(fs, gc_addr, &close_ate);
	if (rc < 0) {
		/* flash error */
		return rc;
	}

	rc = nvs_ate_cmp_const(&close_ate, fs->flash_parameters->erase_value);
	if (!rc) {
		goto gc_done;
	}

	stop_addr = gc_addr - ate_size;

	if (nvs_close_ate_valid(fs, &close_ate)) {
		gc_addr &= ADDR_SECT_MASK;
		gc_addr += close_ate.offset;
	} else {
		rc = nvs_recover_last_ate(fs, &gc_addr);
		if (rc) {
			return rc;
		}
	}

	do {
		gc_prev_addr = gc_addr;
		rc = nvs_prev_ate(fs, &gc_addr, &gc_ate);
		if (rc) {
			return rc;
		}

		if (!nvs_ate_valid(fs, gc_prev_addr, &gc_ate)) {
			continue;
		}

#ifdef CONFIG_NVS_LOOKUP_CACHE
		wlk_addr = fs->lookup_cache[nvs_lookup_cache_pos(gc_ate.id)];

		if (wlk_addr == NVS_LOOKUP_CACHE_NO_ADDR) {
			wlk_addr = fs->ate_wra;
		}
#else
		wlk_addr = fs->ate_wra;
#endif
		do {
			wlk_prev_addr = wlk_addr;
			rc = nvs_prev_ate(fs, &wlk_addr, &wlk_ate);
			if (rc) {
				return rc;
			}
			/* if ate with same id is reached we might need to copy.
			 * only consider valid wlk_ate's. Something wrong might
			 * have been written that has the same ate but is
			 * invalid, don't consider these as a match.
			 */
			if ((wlk_ate.id == gc_ate.id) &&
			    (nvs_ate_valid(fs, wlk_prev_addr, &wlk_ate))) {
				break;
			}
		} while (wlk_addr != fs->ate_wra);

		/* If the walk cursor reaches the same entry as the GC cursor,
		 * the data must be moved unless this entry represents a deleted item
		 * (len == 0).
		 *
		 * During GC, data is compacted and rewritten to the current data
		 * write location. Data may be packed back-to-back without alignment
		 * padding; write alignment is handled internally by buffering.
		 */
		if ((wlk_prev_addr == gc_prev_addr) && gc_ate.len) {
			/* If we have a matching entry already in the sector being GC'd:
			 * - Check that the entry ID matches the current GC ATE
			 * - Check that the existing entry's data + CRC fits within the GC
			 *   ATE length
			 *
			 * Entry matches the GC target and fits in the allocation.
			 * Do not write it now; it will be written later during the final GC
			 * flush.
			 */
			if (entry && (entry->id == gc_ate.id) &&
			    ((entry->len + NVS_DATA_CRC_SIZE) <= gc_ate.len)) {
				LOG_DBG("Skipping entry id %d, len %d; will write later",
					 gc_ate.id, gc_ate.len);
				continue;
			}

			/* copy needed */
			LOG_DBG("Moving %d, len %d", gc_ate.id, gc_ate.len);

			data_addr = (gc_prev_addr & ADDR_SECT_MASK);
			data_addr += gc_ate.offset;

			/* Move the data to the new location.
			 * Data is written in write_block_size-aligned chunks only.
			 * Any remaining unaligned bytes are buffered in ctx.buffer and
			 * reported back via ctx.buffer_pos.
			 */
			rc = nvs_flash_block_move(fs, data_addr, &ctx, &gc_ate);
			if (rc) {
				return rc;
			}

			nvs_ate_crc8_update(&gc_ate);

			rc = nvs_flash_ate_wrt(fs, &gc_ate);
			if (rc) {
				return rc;
			}
		}
	} while (gc_prev_addr != stop_addr);

	rc = nvs_gc_flush_and_try_write(fs, &ctx, entry);
	if (rc) {
		return rc;
	}

gc_done:

	/* Make it possible to detect that gc has finished by writing a
	 * gc done ate to the sector. In the field we might have nvs systems
	 * that do not have sufficient space to add this ate, so for these
	 * situations avoid adding the gc done ate.
	 */

	if (fs->ate_wra >= (fs->data_wra + ate_size)) {
		rc = nvs_add_gc_done_ate(fs);
		if (rc) {
			return rc;
		}
	}

	/* Erase the gc'ed sector */
	rc = nvs_flash_erase_sector(fs, sec_addr);

	return rc;
}

static int nvs_startup(struct nvs_fs *fs)
{
	int rc;
	struct nvs_ate last_ate;
	size_t ate_size, empty_len;
	/* Initialize addr to 0 for the case fs->sector_count == 0. This
	 * should never happen as this is verified in nvs_mount() but both
	 * Coverity and GCC believe the contrary.
	 */
	uint32_t addr = 0U;
	uint16_t i, closed_sectors = 0;
	uint8_t erase_value = fs->flash_parameters->erase_value;

	k_mutex_lock(&fs->nvs_lock, K_FOREVER);

	ate_size = nvs_al_size(fs, sizeof(struct nvs_ate));
	/* step through the sectors to find a open sector following
	 * a closed sector, this is where NVS can write.
	 */
	for (i = 0; i < fs->sector_count; i++) {
		addr = (i << ADDR_SECT_SHIFT) +
		       (uint16_t)(fs->sector_size - ate_size);
		rc = nvs_flash_cmp_const(fs, addr, erase_value,
					 sizeof(struct nvs_ate));
		if (rc) {
			/* closed sector */
			closed_sectors++;
			nvs_sector_advance(fs, &addr);
			rc = nvs_flash_cmp_const(fs, addr, erase_value,
						 sizeof(struct nvs_ate));
			if (!rc) {
				/* open sector */
				break;
			}
		}
	}
	/* all sectors are closed, this is not a nvs fs or irreparably corrupted */
	if (closed_sectors == fs->sector_count) {
#ifdef CONFIG_NVS_INIT_BAD_MEMORY_REGION
		LOG_WRN("All sectors closed, erasing all sectors...");
		rc = flash_flatten(fs->flash_device, fs->offset,
				   fs->sector_size * fs->sector_count);
		if (rc) {
			goto end;
		}

		i = fs->sector_count;
		addr = ((fs->sector_count - 1) << ADDR_SECT_SHIFT) +
		       (uint16_t)(fs->sector_size - ate_size);
#else
		rc = -EDEADLK;
		goto end;
#endif
	}

	if (i == fs->sector_count) {
		/* none of the sectors where closed, in most cases we can set
		 * the address to the first sector, except when there are only
		 * two sectors. Then we can only set it to the first sector if
		 * the last sector contains no ate's. So we check this first
		 */
		rc = nvs_flash_cmp_const(fs, addr - ate_size, erase_value,
				sizeof(struct nvs_ate));
		if (!rc) {
			/* empty ate */
			nvs_sector_advance(fs, &addr);
		}
	}

	/* addr contains address of closing ate in the most recent sector,
	 * search for the last valid ate using the recover_last_ate routine
	 */

	rc = nvs_recover_last_ate(fs, &addr);
	if (rc) {
		goto end;
	}

	/* addr contains address of the last valid ate in the most recent sector
	 * search for the first ate containing all cells erased, in the process
	 * also update fs->data_wra.
	 */
	fs->ate_wra = addr;
	fs->data_wra = addr & ADDR_SECT_MASK;

	while (fs->ate_wra >= fs->data_wra) {
		rc = nvs_flash_ate_rd(fs, fs->ate_wra, &last_ate);
		if (rc) {
			goto end;
		}

		rc = nvs_ate_cmp_const(&last_ate, erase_value);

		if (!rc) {
			/* found ff empty location */
			break;
		}

		if (nvs_ate_valid(fs, fs->ate_wra, &last_ate)) {
			/* complete write of ate was performed */
			fs->data_wra = addr & ADDR_SECT_MASK;
			/* Align the data write address to the current
			 * write block size so that it is possible to write to
			 * the sector even if the block size has changed after
			 * a software upgrade (unless the physical ATE size
			 * will change)."
			 */
			fs->data_wra += nvs_al_size(fs, last_ate.offset + last_ate.len);

			/* ate on the last position within the sector is
			 * reserved for deletion an entry
			 */
			if (fs->ate_wra == fs->data_wra && last_ate.len) {
				/* not a delete ate */
				rc = -ESPIPE;
				goto end;
			}
		}

		fs->ate_wra -= ate_size;
	}

	/* if the sector after the write sector is not empty gc was interrupted
	 * we might need to restart gc if it has not yet finished. Otherwise
	 * just erase the sector.
	 * When gc needs to be restarted, first erase the sector otherwise the
	 * data might not fit into the sector.
	 */
	addr = fs->ate_wra & ADDR_SECT_MASK;
	nvs_sector_advance(fs, &addr);
	rc = nvs_flash_cmp_const(fs, addr, erase_value, fs->sector_size);
	if (rc < 0) {
		goto end;
	}
	if (rc) {
		/* the sector after fs->ate_wrt is not empty, look for a marker
		 * (gc_done_ate) that indicates that gc was finished.
		 */
		bool gc_done_marker = false;
		struct nvs_ate gc_done_ate;

		addr = fs->ate_wra + ate_size;
		while ((addr & ADDR_OFFS_MASK) < (fs->sector_size - ate_size)) {
			rc = nvs_flash_ate_rd(fs, addr, &gc_done_ate);
			if (rc) {
				goto end;
			}
			if (nvs_ate_valid(fs, addr, &gc_done_ate) &&
			    (gc_done_ate.id == 0xffff) &&
			    (gc_done_ate.len == 0U)) {
				gc_done_marker = true;
				break;
			}
			addr += ate_size;
		}

		if (gc_done_marker) {
			/* erase the next sector */
			LOG_INF("GC Done marker found");
			addr = fs->ate_wra & ADDR_SECT_MASK;
			nvs_sector_advance(fs, &addr);
			rc = nvs_flash_erase_sector(fs, addr);
			goto end;
		}
		LOG_INF("No GC Done marker found: restarting gc");
		rc = nvs_flash_erase_sector(fs, fs->ate_wra);
		if (rc) {
			goto end;
		}
		fs->ate_wra &= ADDR_SECT_MASK;
		fs->ate_wra += (fs->sector_size - 2 * ate_size);
		fs->data_wra = (fs->ate_wra & ADDR_SECT_MASK);
#ifdef CONFIG_NVS_LOOKUP_CACHE
		/**
		 * At this point, the lookup cache wasn't built but the gc function need to use it.
		 * So, temporarily, we set the lookup cache to the end of the fs.
		 * The cache will be rebuilt afterwards
		 **/
		for (i = 0; i < CONFIG_NVS_LOOKUP_CACHE_SIZE; i++) {
			fs->lookup_cache[i] = fs->ate_wra;
		}
#endif
		rc = nvs_gc(fs, NULL);
		goto end;
	}

	/* possible data write after last ate write, update data_wra */
	while (fs->ate_wra > fs->data_wra) {
		empty_len = fs->ate_wra - fs->data_wra;

		rc = nvs_flash_cmp_const(fs, fs->data_wra, erase_value,
				empty_len);
		if (rc < 0) {
			goto end;
		}
		if (!rc) {
			break;
		}

		fs->data_wra += fs->flash_parameters->write_block_size;

		/*
		 * If the loop exits because data_wra has caught up with ate_wra,
		 * this does not indicate an error, but that no erased space
		 * remains for further data writes in this sector.
		 *
		 * In this case, the sector is logically full and will be closed
		 * by the next write, which will also trigger garbage collection.
		 *
		 * Therefore, ensure rc is set to 0 so startup does not fail
		 * due to the absence of writable space.
		 */
		rc = 0;
	}

	/* If the ate_wra is pointing to the first ate write location in a
	 * sector and data_wra is not 0, erase the sector as it contains no
	 * valid data (this also avoids closing a sector without any data).
	 */
	if (((fs->ate_wra + 2 * ate_size) == fs->sector_size) &&
	    (fs->data_wra != (fs->ate_wra & ADDR_SECT_MASK))) {
		rc = nvs_flash_erase_sector(fs, fs->ate_wra);
		if (rc) {
			goto end;
		}
		fs->data_wra = fs->ate_wra & ADDR_SECT_MASK;
	}

end:

#ifdef CONFIG_NVS_LOOKUP_CACHE
	if (!rc) {
		rc = nvs_lookup_cache_rebuild(fs);
	}
#endif
	/* If the sector is empty add a gc done ate to avoid having insufficient
	 * space when doing gc.
	 */
	if ((!rc) && ((fs->ate_wra & ADDR_OFFS_MASK) ==
		      (fs->sector_size - 2 * ate_size))) {

		rc = nvs_add_gc_done_ate(fs);
	}
	k_mutex_unlock(&fs->nvs_lock);
	return rc;
}

int nvs_clear(struct nvs_fs *fs)
{
	int rc;
	uint32_t addr;

	if (!fs->ready) {
		LOG_ERR("NVS not initialized");
		return -EACCES;
	}

	for (uint16_t i = 0; i < fs->sector_count; i++) {
		addr = i << ADDR_SECT_SHIFT;
		rc = nvs_flash_erase_sector(fs, addr);
		if (rc) {
			return rc;
		}
	}

	/* nvs needs to be reinitialized after clearing */
	fs->ready = false;

	return 0;
}

int nvs_mount(struct nvs_fs *fs)
{
	int rc;
	struct flash_pages_info info;
	size_t write_block_size;

	k_mutex_init(&fs->nvs_lock);

	fs->flash_parameters = flash_get_parameters(fs->flash_device);
	if (fs->flash_parameters == NULL) {
		LOG_ERR("Could not obtain flash parameters");
		return -EINVAL;
	}

	write_block_size = flash_get_write_block_size(fs->flash_device);

	/* check that the write block size is supported */
	if (write_block_size > NVS_BLOCK_SIZE || write_block_size == 0) {
		LOG_ERR("Unsupported write block size");
		return -EINVAL;
	}

	/* check that sector size is not greater than max */
	if (fs->sector_size > NVS_MAX_SECTOR_SIZE) {
		LOG_ERR("Sector size %u too large, maximum is %zu",
			fs->sector_size, NVS_MAX_SECTOR_SIZE);
		return -EINVAL;
	}

	/* check that sector size is a multiple of pagesize */
	rc = flash_get_page_info_by_offs(fs->flash_device, fs->offset, &info);
	if (rc) {
		LOG_ERR("Unable to get page info");
		return -EINVAL;
	}
	if (!fs->sector_size || fs->sector_size % info.size) {
		LOG_ERR("Invalid sector size");
		return -EINVAL;
	}

	/* check the number of sectors, it should be at least 2 */
	if (fs->sector_count < 2) {
		LOG_ERR("Configuration error - sector count");
		return -EINVAL;
	}

	rc = nvs_startup(fs);
	if (rc) {
		return rc;
	}

	/* nvs is ready for use */
	fs->ready = true;

	LOG_INF("%d Sectors of %d bytes", fs->sector_count, fs->sector_size);
	LOG_INF("alloc wra: %d, %x",
		(fs->ate_wra >> ADDR_SECT_SHIFT),
		(fs->ate_wra & ADDR_OFFS_MASK));
	LOG_INF("data wra: %d, %x",
		(fs->data_wra >> ADDR_SECT_SHIFT),
		(fs->data_wra & ADDR_OFFS_MASK));

	return 0;
}

ssize_t nvs_write(struct nvs_fs *fs, uint16_t id, const void *data, size_t len)
{
	int rc, gc_count;
	struct nvs_gc_write_entry wrt_entry;
	size_t ate_size, data_size;
	struct nvs_ate wlk_ate;
	uint32_t wlk_addr, rd_addr;
	uint16_t required_space = 0U; /* no space, appropriate for delete ate */
	bool prev_found = false;

	if (!fs->ready) {
		LOG_ERR("NVS not initialized");
		return -EACCES;
	}

	ate_size = nvs_al_size(fs, sizeof(struct nvs_ate));
	data_size = nvs_al_size(fs, len ? len + NVS_DATA_CRC_SIZE : 0U);

	/* The maximum data size is sector size - 4 ate
	 * where: 1 ate for data, 1 ate for sector close, 1 ate for gc done,
	 * and 1 ate to always allow a delete.
	 * Also take into account the data CRC that is appended at the end of the data field,
	 * if any.
	 */
	if ((data_size > (fs->sector_size - 4 * ate_size)) ||
	    ((len > 0) && (data == NULL))) {
		return -EINVAL;
	}

	/* find latest entry with same id */
#ifdef CONFIG_NVS_LOOKUP_CACHE
	wlk_addr = fs->lookup_cache[nvs_lookup_cache_pos(id)];

	if (wlk_addr == NVS_LOOKUP_CACHE_NO_ADDR) {
		goto no_cached_entry;
	}
#else
	wlk_addr = fs->ate_wra;
#endif
	rd_addr = wlk_addr;

	while (1) {
		rd_addr = wlk_addr;
		rc = nvs_prev_ate(fs, &wlk_addr, &wlk_ate);
		if (rc) {
			return rc;
		}
		if ((wlk_ate.id == id) &&
		    (nvs_ate_valid(fs, rd_addr, &wlk_ate))) {
			prev_found = true;
			break;
		}
		if (wlk_addr == fs->ate_wra) {
			break;
		}
	}

#ifdef CONFIG_NVS_LOOKUP_CACHE
no_cached_entry:
#endif

	if (prev_found) {
		/* previous entry found */
		rd_addr &= ADDR_SECT_MASK;
		rd_addr += wlk_ate.offset;

		if (len == 0) {
			/* do not try to compare with empty data */
			if (wlk_ate.len == 0U) {
				/* skip delete entry as it is already the
				 * last one
				 */
				return 0;
			}
		} else if (len + NVS_DATA_CRC_SIZE == wlk_ate.len) {
			/* do not try to compare if lengths are not equal */
			/* compare the data and if equal return 0 */
			/* note: data CRC is not taken into account here, as it has not yet been
			 * appended to the data buffer
			 */
			rc = nvs_flash_block_cmp(fs, rd_addr, data, len);
			if (rc <= 0) {
				return rc;
			}
		}
	} else {
		/* skip delete entry for non-existing entry */
		if (len == 0) {
			return 0;
		}
	}

	/* calculate required space if the entry contains data */
	if (data_size) {
		/* Leave space for delete ate */
		required_space = data_size + ate_size;
	}

	k_mutex_lock(&fs->nvs_lock, K_FOREVER);

	gc_count = 0;
	while (1) {
		if (gc_count == fs->sector_count) {
			/* gc'ed all sectors, no extra space will be created
			 * by extra gc.
			 */
			rc = -ENOSPC;
			goto end;
		}

		/* ATEs grow backwards within a sector. In delete-only scenarios,
		 * a sector may contain only delete ATEs and no data entries.
		 * Prevent ATE writes at current start of sector to avoid crossing
		 * into the previous sector.
		 */
		if (fs->ate_wra >= (fs->data_wra + required_space) &&
		    (fs->ate_wra & ADDR_OFFS_MASK) != 0) {

			rc = nvs_flash_wrt_entry(fs, id, data, len);
			if (rc) {
				goto end;
			}
			break;
		}

		rc = nvs_sector_close(fs);
		if (rc) {
			goto end;
		}

		/* Initialize pending write request for GC processing */
		if (gc_count == 0) {
			wrt_entry.id = id;
			wrt_entry.data = data;
			wrt_entry.len = len;
			wrt_entry.is_written = false;
		}

		rc = nvs_gc(fs, &wrt_entry);
		if (rc) {
			goto end;
		}
		gc_count++;

		/* Exit if the entry has been written during GC */
		if (wrt_entry.is_written) {
			break;
		}
	}
	rc = len;
end:
	k_mutex_unlock(&fs->nvs_lock);
	return rc;
}

int nvs_delete(struct nvs_fs *fs, uint16_t id)
{
	return nvs_write(fs, id, NULL, 0);
}

ssize_t nvs_read_hist(struct nvs_fs *fs, uint16_t id, void *data, size_t len,
		      uint16_t cnt)
{
	int rc;
	uint32_t wlk_addr, rd_addr;
	uint16_t cnt_his;
	struct nvs_ate wlk_ate;
	size_t ate_size;
#ifdef CONFIG_NVS_DATA_CRC
	uint32_t read_data_crc, computed_data_crc;
#endif

	if (!fs->ready) {
		LOG_ERR("NVS not initialized");
		return -EACCES;
	}

	ate_size = nvs_al_size(fs, sizeof(struct nvs_ate));

	if (len > (fs->sector_size - 2 * ate_size)) {
		return -EINVAL;
	}

	cnt_his = 0U;

#ifdef CONFIG_NVS_LOOKUP_CACHE
	wlk_addr = fs->lookup_cache[nvs_lookup_cache_pos(id)];

	if (wlk_addr == NVS_LOOKUP_CACHE_NO_ADDR) {
		rc = -ENOENT;
		goto err;
	}
#else
	wlk_addr = fs->ate_wra;
#endif
	rd_addr = wlk_addr;

	while (cnt_his <= cnt) {
		rd_addr = wlk_addr;
		rc = nvs_prev_ate(fs, &wlk_addr, &wlk_ate);
		if (rc) {
			goto err;
		}
		if ((wlk_ate.id == id) &&
		    (nvs_ate_valid(fs, rd_addr, &wlk_ate))) {
			cnt_his++;
		}
		if (wlk_addr == fs->ate_wra) {
			break;
		}
	}

	if (((wlk_addr == fs->ate_wra) && (wlk_ate.id != id)) ||
	    (wlk_ate.len == 0U) || (cnt_his < cnt)) {
		return -ENOENT;
	}

#ifdef CONFIG_NVS_DATA_CRC
	/* When data CRC is enabled, there should be at least the CRC stored in the data field */
	if (wlk_ate.len < NVS_DATA_CRC_SIZE) {
		return -ENOENT;
	}
#endif

	rd_addr &= ADDR_SECT_MASK;
	rd_addr += wlk_ate.offset;
	rc = nvs_flash_rd(fs, rd_addr, data, MIN(len, wlk_ate.len - NVS_DATA_CRC_SIZE));
	if (rc) {
		goto err;
	}

	/* Check data CRC (only if the whole element data has been read) */
#ifdef CONFIG_NVS_DATA_CRC
	if (len >= (wlk_ate.len - NVS_DATA_CRC_SIZE)) {
		rd_addr += wlk_ate.len - NVS_DATA_CRC_SIZE;
		rc = nvs_flash_rd(fs, rd_addr, &read_data_crc, sizeof(read_data_crc));
		if (rc) {
			goto err;
		}

		computed_data_crc = crc32_ieee(data, wlk_ate.len - NVS_DATA_CRC_SIZE);
		if (read_data_crc != computed_data_crc) {
			LOG_ERR("Invalid data CRC: read_data_crc=0x%08X, computed_data_crc=0x%08X",
				read_data_crc, computed_data_crc);
			rc = -EIO;
			goto err;
		}
	}
#endif

	return wlk_ate.len - NVS_DATA_CRC_SIZE;

err:
	return rc;
}

ssize_t nvs_read(struct nvs_fs *fs, uint16_t id, void *data, size_t len)
{
	int rc;

	rc = nvs_read_hist(fs, id, data, len, 0);
	return rc;
}

ssize_t nvs_calc_free_space(struct nvs_fs *fs)
{
	uint32_t step_prev_addr, step_addr, wlk_addr;
	struct nvs_ate step_ate, wlk_ate;
	size_t ate_size, free_space;
	int rc;

	if (!fs->ready) {
		LOG_ERR("NVS not initialized");
		return -EACCES;
	}

	ate_size = nvs_al_size(fs, sizeof(struct nvs_ate));

	/*
	 * There is always a closing ATE and a reserved ATE for
	 * deletion in each sector.
	 * Take into account one less sector because it is reserved for the
	 * garbage collection.
	 */
	free_space = (fs->sector_count - 1) * (fs->sector_size - (2 * ate_size));

	step_addr = fs->ate_wra;

	while (1) {
		step_prev_addr = step_addr;
		rc = nvs_prev_ate(fs, &step_addr, &step_ate);
		if (rc) {
			return rc;
		}

		wlk_addr = fs->ate_wra;

		while (1) {
			rc = nvs_prev_ate(fs, &wlk_addr, &wlk_ate);
			if (rc) {
				return rc;
			}
			if ((wlk_ate.id == step_ate.id) ||
			    (wlk_addr == fs->ate_wra)) {
				break;
			}
		}

		if (nvs_ate_valid(fs, step_prev_addr, &step_ate)) {
			/* Take into account the GC done ATE if it is present */
			if (step_ate.len == 0) {
				if (step_ate.id == 0xFFFF) {
					free_space -= ate_size;
				}
			} else if (wlk_addr == step_addr) {
				/* count needed */
				free_space -= nvs_al_size(fs, step_ate.len);
				free_space -= ate_size;
			}
		}

		if (step_addr == fs->ate_wra) {
			break;
		}
	}
	return free_space;
}

size_t nvs_sector_max_data_size(struct nvs_fs *fs)
{
	size_t ate_size;

	if (!fs->ready) {
		LOG_ERR("NVS not initialized");
		return -EACCES;
	}

	ate_size = nvs_al_size(fs, sizeof(struct nvs_ate));

	return fs->ate_wra - fs->data_wra - ate_size - NVS_DATA_CRC_SIZE;
}

int nvs_sector_use_next(struct nvs_fs *fs)
{
	int ret;

	if (!fs->ready) {
		LOG_ERR("NVS not initialized");
		return -EACCES;
	}

	k_mutex_lock(&fs->nvs_lock, K_FOREVER);

	ret = nvs_sector_close(fs);
	if (ret != 0) {
		goto end;
	}

	ret = nvs_gc(fs, NULL);

end:
	k_mutex_unlock(&fs->nvs_lock);
	return ret;
}
