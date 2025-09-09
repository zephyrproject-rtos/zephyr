/* Copyright (c) 2018 Laczen
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ZMS: Zephyr Memory Storage
 */

#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <zephyr/fs/zms.h>
#include <zephyr/sys/crc.h>
#include "zms_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fs_zms, CONFIG_ZMS_LOG_LEVEL);

static int zms_prev_ate(struct zms_fs *fs, uint64_t *addr, struct zms_ate *ate);
static int zms_ate_valid(struct zms_fs *fs, const struct zms_ate *entry);
static int zms_get_sector_cycle(struct zms_fs *fs, uint64_t addr, uint8_t *cycle_cnt);
static int zms_get_sector_header(struct zms_fs *fs, uint64_t addr, struct zms_ate *empty_ate,
				 struct zms_ate *close_ate);
static int zms_ate_valid_different_sector(struct zms_fs *fs, const struct zms_ate *entry,
					  uint8_t cycle_cnt);

#ifdef CONFIG_ZMS_LOOKUP_CACHE
BUILD_ASSERT(UINT32_MAX == ZMS_HEAD_ID);
#define ZMS_DONT_KEEP_HISTORY 1

static inline const char *get_fs_name(const struct zms_fs *fs)
{
	return (fs->name) ? fs->name : "?";
}

int zms_get_highest_id_in_use(const struct zms_fs *fs, uint32_t *id)
{
	if (!fs->highest_id_in_use_valid) {
		return -ENOENT;
	}
	if (id) {
		*id = fs->highest_id_in_use;
	}
	return 0;
}

int zms_get_lowest_id_in_use(const struct zms_fs *fs, uint32_t *id)
{
	if (!fs->lowest_id_in_use_valid) {
		return -ENOENT;
	}
	if (id) {
		*id = fs->lowest_id_in_use;
	}
	return 0;
}

int32_t zms_get_num_entries(const struct zms_fs *fs)
{
	return fs->num_valid_ates;
}

static int zms_ate_crc8_check(const struct zms_ate *entry);

static void zms_invalidate_ate(struct zms_ate *ate)
{
	ate->len = 0;
	if (zms_ate_crc8_check(ate)) {
		return; // the existing CRC is already wrong / invalid
	}
	if (ate->crc8) {
		const uint8_t lsb = LSB_GET(ate->crc8);
		const uint8_t orig_crc = ate->crc8;
		ate->crc8 = orig_crc & ~lsb; // make the lowest 1 bit -> 0
		if (zms_ate_crc8_check(ate)) {
			LOG_DBG("wrote invalid crc8 0x%x for id %u (by deleting 0x%x from 0x%x)",
				ate->crc8, ate->id, lsb, orig_crc);
			return;
		} else {
			LOG_ERR("%s: could not invalidate crc8 0x%x for id %u (by deleting 0x%x "
				"from 0x%x)",
				__func__, ate->crc8, ate->id, lsb, orig_crc);
		}
	}
	// It should always be possible to write the all-0 ate
	memset(ate, 0, sizeof(struct zms_ate));
	if (zms_ate_crc8_check(ate)) {
		LOG_DBG("wrote invalid all-0 ate");
		return;
	}
	// This LOG_ERR() is never hit: the all-0 ate is in fact invalid
	LOG_ERR("%s: all-0 ate was marked valid!", __func__);
}

static inline size_t zms_read_cache_pos(uint32_t id)
{
	return id % ZMS_READ_CACHE_SIZE;
}

static void zms_read_cache_update(struct zms_fs *fs, uint32_t id, uint64_t ate_addr,
				  bool exists_now, bool existed_before)
{
	if (id == ZMS_HEAD_ID) {
		return;
	}
	struct zms_read_cache_s *cp = &fs->last_read[zms_read_cache_pos(id)];
	if (exists_now) {
		cp->addr = ate_addr;
		cp->id = id;
		if (!fs->highest_id_in_use_valid || (id > fs->highest_id_in_use)) {
			fs->highest_id_in_use = id;
			fs->highest_id_in_use_valid = true;
		}
		if (!fs->lowest_id_in_use_valid || (id < fs->lowest_id_in_use)) {
			fs->lowest_id_in_use = id;
			fs->lowest_id_in_use_valid = true;
		}
	} else {
		if (cp->id == id) {
			cp->addr = ZMS_LOOKUP_CACHE_NO_ADDR;
		}
		if (fs->highest_id_in_use_valid && (id == fs->highest_id_in_use)) {
			fs->highest_id_in_use_valid = (id > 0) && fs->lowest_id_in_use_valid &&
						      (id > fs->lowest_id_in_use);
			fs->highest_id_in_use = id - 1;
		}
		if (fs->lowest_id_in_use_valid && (id == fs->lowest_id_in_use)) {
			fs->lowest_id_in_use_valid = (id < UINT32_MAX) &&
						     fs->highest_id_in_use_valid &&
						     (id < fs->highest_id_in_use);
			fs->lowest_id_in_use = id + 1;
		}
	}
	if (fs->num_valid_ates >= 0) {
		int8_t d = (exists_now ? 1 : 0) - (existed_before ? 1 : 0);
		fs->num_valid_ates += d;
	}
	bool strange = false;
	if (fs->num_valid_ates > 0) {
		if (!fs->highest_id_in_use_valid || !fs->lowest_id_in_use_valid) {
			strange = true;
		} else if (fs->num_valid_ates >
			   (1 + fs->highest_id_in_use - fs->lowest_id_in_use)) {
			strange = true;
		}
	} else if (fs->num_valid_ates < 0) {
		strange = true;
	} else if (fs->highest_id_in_use_valid || fs->lowest_id_in_use_valid) {
		strange = true;
	}
	if (strange && fs->invalidate_old_ates) {
		LOG_WRN("%s: %s: id: %u, e: %d, b: %d, num_valid_ates: %d, highest_id_in_use: %u "
			"(%svalid), lowest_id_in_use: %u (%svalid)",
			__func__, get_fs_name(fs), id, exists_now, existed_before,
			fs->num_valid_ates, fs->highest_id_in_use,
			fs->highest_id_in_use_valid ? "" : "in", fs->lowest_id_in_use,
			fs->lowest_id_in_use_valid ? "" : "in");
	}
}

static int zms_compute_prev_addr(struct zms_fs *fs, uint64_t *addr);
static int zms_find_ate_with_id(struct zms_fs *fs, uint32_t id, uint64_t start_addr,
				uint64_t end_addr, struct zms_ate *ate, uint64_t *ate_addr,
				int32_t *loop_cnt, int32_t loop_cnt_max);

static int zms_quick_find_ate_with_id(struct zms_fs *fs, uint32_t id, struct zms_ate *ate,
				      uint64_t *ate_addr, int32_t *loop_cnt)
{
#define ID_MATCH_RANGE (10)
	const uint32_t id_min = (id > ID_MATCH_RANGE) ? (id - ID_MATCH_RANGE) : 0;
	const uint32_t id_max =
		(id < UINT32_MAX - ID_MATCH_RANGE) ? (id + ID_MATCH_RANGE) : UINT32_MAX;
	int found = 0;
	int32_t loop_cnt_l = 0;
	if (!loop_cnt) {
		loop_cnt = &loop_cnt_l;
	}
	for (uint32_t i = id; i < (id + ZMS_READ_CACHE_SIZE); ++i) {
		int rc = 0;
		struct zms_read_cache_s *cp = &fs->last_read[zms_read_cache_pos(i)];
		if ((cp->addr == ZMS_LOOKUP_CACHE_NO_ADDR) || (cp->id < id_min) ||
		    (cp->id > id_max)) {
			continue;
		}
		uint64_t start_addr = cp->addr;
		if (id > cp->id) {
			uint32_t d = id - cp->id;
			if (d > ID_MATCH_RANGE) {
				d = ID_MATCH_RANGE;
			}
			for (uint32_t j = 0; j < d; ++j) {
				// TODO: this is a primitive heuristic; implement a real function
				const uint64_t next_addr = start_addr - fs->ate_size;
				uint64_t t = next_addr;
				rc = zms_compute_prev_addr(fs, &t);
				if ((rc < 0) || (t != start_addr)) {
					break;
				}
				start_addr = next_addr;
			}
		}
		found = zms_find_ate_with_id(fs, id, start_addr, start_addr, ate, ate_addr,
					     loop_cnt, *loop_cnt + 2 * ID_MATCH_RANGE);
		if (found) {
			break;
		}
	}
	return found;
}

static inline size_t zms_lookup_cache_pos(uint32_t id)
{
	uint32_t hash;

	/* 32-bit integer hash function found by https://github.com/skeeto/hash-prospector. */
	hash = id;
	hash ^= hash >> 16;
	hash *= 0x7feb352dU;
	hash ^= hash >> 15;
	hash *= 0x846ca68bU;
	hash ^= hash >> 16;

	return hash % CONFIG_ZMS_LOOKUP_CACHE_SIZE;
}

static int zms_lookup_cache_rebuild(struct zms_fs *fs)
{
	int rc;
	int previous_sector_num = ZMS_INVALID_SECTOR_NUM;
	uint64_t addr;
	uint64_t ate_addr;
	uint64_t *cache_entry;
	uint8_t current_cycle;
	struct zms_ate ate;
	int32_t loop_count = 0;

	memset(fs->lookup_cache, 0xff, sizeof(fs->lookup_cache));
	for (int8_t i = 0; i < ZMS_READ_CACHE_SIZE; ++i) {
		fs->last_read[i].addr = ZMS_LOOKUP_CACHE_NO_ADDR;
		fs->last_read[i].id = UINT32_MAX;
	}
	fs->highest_id_in_use_valid = false;
	fs->lowest_id_in_use_valid = false;
	fs->num_valid_ates = 0;
	fs->highest_id_in_use = UINT32_MAX;
	fs->lowest_id_in_use = UINT32_MAX;

	addr = fs->ate_wra;

	while (true) {
		++loop_count;
		/* Make a copy of 'addr' as it will be advanced by zms_prev_ate() */
		ate_addr = addr;
		rc = zms_prev_ate(fs, &addr, &ate);

		if (rc) {
			return rc;
		}

		cache_entry = &fs->lookup_cache[zms_lookup_cache_pos(ate.id)];

		if (ate.id != ZMS_HEAD_ID) {
			/* read the ate cycle only when we change the sector
			 * or if it is the first read
			 */
			if (SECTOR_NUM(ate_addr) != previous_sector_num) {
				rc = zms_get_sector_cycle(fs, ate_addr, &current_cycle);
				if (rc == -ENOENT) {
					/* sector never used */
					current_cycle = 0;
				} else if (rc) {
					/* bad flash read */
					return rc;
				}
			}
			if (zms_ate_valid_different_sector(fs, &ate, current_cycle)) {
				if (*cache_entry == ZMS_LOOKUP_CACHE_NO_ADDR) {
					*cache_entry = ate_addr;
					LOG_DBG("%s: assigned cache entry %2d: id %10u -> "
						"0x%llx",
						get_fs_name(fs), cache_entry - fs->lookup_cache,
						ate.id, ate_addr);
				}
				zms_read_cache_update(fs, ate.id, ate_addr, true, false);
			}
			previous_sector_num = SECTOR_NUM(ate_addr);
		}

		if (addr == fs->ate_wra) {
			break;
		}
	}
	LOG_DBG("done for fs %s, loop_count: %d", get_fs_name(fs), loop_count);

	return 0;
}

static void zms_lookup_cache_invalidate(struct zms_fs *fs, uint32_t sector)
{
	uint64_t *cache_entry = fs->lookup_cache;
	uint64_t *const cache_end = &fs->lookup_cache[CONFIG_ZMS_LOOKUP_CACHE_SIZE];

	for (; cache_entry < cache_end; ++cache_entry) {
		if (SECTOR_NUM(*cache_entry) == sector) {
			*cache_entry = ZMS_LOOKUP_CACHE_NO_ADDR;
		}
	}
	for (int8_t i = 0; i < ZMS_READ_CACHE_SIZE; ++i) {
		fs->last_read[i].addr = ZMS_LOOKUP_CACHE_NO_ADDR;
		fs->last_read[i].id = UINT32_MAX;
	}
}

#else /* CONFIG_ZMS_LOOKUP_CACHE */

static inline const char *get_fs_name(const struct zms_fs *fs)
{
	ARG_UNUSED(fs);
	return "?";
}

#endif /* CONFIG_ZMS_LOOKUP_CACHE */

/* Helper to compute offset given the address */
static inline off_t zms_addr_to_offset(struct zms_fs *fs, uint64_t addr)
{
	return fs->offset + (fs->sector_size * SECTOR_NUM(addr)) + SECTOR_OFFSET(addr);
}

/* Helper to round down len to the closest multiple of write_block_size  */
static inline size_t zms_round_down_write_block_size(struct zms_fs *fs, size_t len)
{
	return len & ~(fs->flash_parameters->write_block_size - 1U);
}

/* Helper to round up len to multiple of write_block_size */
static inline size_t zms_round_up_write_block_size(struct zms_fs *fs, size_t len)
{
	return (len + (fs->flash_parameters->write_block_size - 1U)) &
	       ~(fs->flash_parameters->write_block_size - 1U);
}

/* zms_al_size returns size aligned to fs->write_block_size */
static inline size_t zms_al_size(struct zms_fs *fs, size_t len)
{
	size_t write_block_size = fs->flash_parameters->write_block_size;

	if (write_block_size <= 1U) {
		return len;
	}

	return zms_round_up_write_block_size(fs, len);
}

/* Helper to get empty ATE address */
static inline uint64_t zms_empty_ate_addr(struct zms_fs *fs, uint64_t addr)
{
	return (addr & ADDR_SECT_MASK) + fs->sector_size - fs->ate_size;
}

/* Helper to get close ATE address */
static inline uint64_t zms_close_ate_addr(struct zms_fs *fs, uint64_t addr)
{
	return (addr & ADDR_SECT_MASK) + fs->sector_size - 2 * fs->ate_size;
}

/* Aligned memory write */
static int zms_flash_al_wrt(struct zms_fs *fs, uint64_t addr, const void *data, size_t len)
{
	const uint8_t *data8 = (const uint8_t *)data;
	int rc = 0;
	off_t offset;
	size_t blen;
	uint8_t buf[ZMS_BLOCK_SIZE];

	if (!len) {
		/* Nothing to write, avoid changing the flash protection */
		return 0;
	}

	offset = zms_addr_to_offset(fs, addr);

	blen = zms_round_down_write_block_size(fs, len);
	if (blen > 0) {
		rc = flash_write(fs->flash_device, offset, data8, blen);
		if (rc) {
			/* flash write error */
			goto end;
		}
		len -= blen;
		offset += blen;
		data8 += blen;
	}
	if (len) {
		memcpy(buf, data8, len);
		(void)memset(buf + len, fs->flash_parameters->erase_value,
			     fs->flash_parameters->write_block_size - len);

		rc = flash_write(fs->flash_device, offset, buf,
				 fs->flash_parameters->write_block_size);
	}

end:
	return rc;
}

/* basic flash read from zms address */
static int zms_flash_rd(struct zms_fs *fs, uint64_t addr, void *data, size_t len)
{
	off_t offset;

	offset = zms_addr_to_offset(fs, addr);

	return flash_read(fs->flash_device, offset, data, len);
}

/* allocation entry write */
static int zms_flash_ate_wrt(struct zms_fs *fs, const struct zms_ate *entry)
{
	int rc;

	rc = zms_flash_al_wrt(fs, fs->ate_wra, entry, sizeof(struct zms_ate));
	if (rc) {
		goto end;
	}
#ifdef CONFIG_ZMS_LOOKUP_CACHE
	/* 0xFFFFFFFF is a special-purpose identifier. Exclude it from the cache */
	if (entry->id != ZMS_HEAD_ID) {
		fs->lookup_cache[zms_lookup_cache_pos(entry->id)] = fs->ate_wra;
	}
#endif
	fs->ate_wra -= zms_al_size(fs, sizeof(struct zms_ate));
end:
	return rc;
}

/* data write */
static int zms_flash_data_wrt(struct zms_fs *fs, const void *data, size_t len)
{
	int rc;

	rc = zms_flash_al_wrt(fs, fs->data_wra, data, len);
	if (rc < 0) {
		return rc;
	}
	fs->data_wra += zms_al_size(fs, len);

	return 0;
}

/* flash ate read */
static int zms_flash_ate_rd(struct zms_fs *fs, uint64_t addr, struct zms_ate *entry)
{
	return zms_flash_rd(fs, addr, entry, sizeof(struct zms_ate));
}

/* zms_flash_block_cmp compares the data in flash at addr to data
 * in blocks of size ZMS_BLOCK_SIZE aligned to fs->write_block_size
 * returns 0 if equal, 1 if not equal, errcode if error
 */
static int zms_flash_block_cmp(struct zms_fs *fs, uint64_t addr, const void *data, size_t len)
{
	const uint8_t *data8 = (const uint8_t *)data;
	int rc;
	size_t bytes_to_cmp;
	size_t block_size;
	uint8_t buf[ZMS_BLOCK_SIZE];

	block_size = zms_round_down_write_block_size(fs, ZMS_BLOCK_SIZE);

	while (len) {
		bytes_to_cmp = MIN(block_size, len);
		rc = zms_flash_rd(fs, addr, buf, bytes_to_cmp);
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

/* zms_flash_cmp_const compares the data in flash at addr to a constant
 * value. returns 0 if all data in flash is equal to value, 1 if not equal,
 * errcode if error
 */
static int zms_flash_cmp_const(struct zms_fs *fs, uint64_t addr, uint8_t value, size_t len)
{
	int rc;
	size_t bytes_to_cmp;
	size_t block_size;
	uint8_t cmp[ZMS_BLOCK_SIZE];

	block_size = zms_round_down_write_block_size(fs, ZMS_BLOCK_SIZE);

	(void)memset(cmp, value, block_size);
	while (len) {
		bytes_to_cmp = MIN(block_size, len);
		rc = zms_flash_block_cmp(fs, addr, cmp, bytes_to_cmp);
		if (rc) {
			return rc;
		}
		len -= bytes_to_cmp;
		addr += bytes_to_cmp;
	}
	return 0;
}

/* flash block move: move a block at addr to the current data write location
 * and updates the data write location.
 */
static int zms_flash_block_move(struct zms_fs *fs, uint64_t addr, size_t len)
{
	int rc;
	size_t bytes_to_copy;
	size_t block_size;
	uint8_t buf[ZMS_BLOCK_SIZE];

	block_size = zms_round_down_write_block_size(fs, ZMS_BLOCK_SIZE);

	while (len) {
		bytes_to_copy = MIN(block_size, len);
		rc = zms_flash_rd(fs, addr, buf, bytes_to_copy);
		if (rc) {
			return rc;
		}
		rc = zms_flash_data_wrt(fs, buf, bytes_to_copy);
		if (rc) {
			return rc;
		}
		len -= bytes_to_copy;
		addr += bytes_to_copy;
	}
	return 0;
}

/* erase a sector and verify erase was OK.
 * return 0 if OK, errorcode on error.
 */
static int zms_flash_erase_sector(struct zms_fs *fs, uint64_t addr)
{
	int rc;
	off_t offset;
	bool ebw_required =
		flash_params_get_erase_cap(fs->flash_parameters) & FLASH_ERASE_C_EXPLICIT;

	if (!ebw_required) {
		/* Do nothing for devices that do not have erase capability */
		return 0;
	}

	addr &= ADDR_SECT_MASK;
	offset = zms_addr_to_offset(fs, addr);

	LOG_DBG("%s: erasing flash at offset 0x%lx ( 0x%llx ), len %u", get_fs_name(fs),
		(long)offset, addr, fs->sector_size);

#ifdef CONFIG_ZMS_LOOKUP_CACHE
	zms_lookup_cache_invalidate(fs, SECTOR_NUM(addr));
#endif
	rc = flash_erase(fs->flash_device, offset, fs->sector_size);

	if (rc) {
		return rc;
	}

	if (zms_flash_cmp_const(fs, addr, fs->flash_parameters->erase_value, fs->sector_size)) {
		LOG_ERR("%s: %s: failure while erasing the sector at offset 0x%lx", __func__,
			get_fs_name(fs), (long)offset);
		rc = -ENXIO;
	}

	return rc;
}

/* crc update on allocation entry */
static void zms_ate_crc8_update(struct zms_ate *entry)
{
	uint8_t crc8;

	/* crc8 field is the first element of the structure, do not include it */
	crc8 = crc8_ccitt(0xff, (uint8_t *)entry + SIZEOF_FIELD(struct zms_ate, crc8),
			  sizeof(struct zms_ate) - SIZEOF_FIELD(struct zms_ate, crc8));
	entry->crc8 = crc8;
}

/* crc check on allocation entry
 * returns 0 if OK, 1 on crc fail
 */
static int zms_ate_crc8_check(const struct zms_ate *entry)
{
	uint8_t crc8;

	/* crc8 field is the first element of the structure, do not include it */
	crc8 = crc8_ccitt(0xff, (uint8_t *)entry + SIZEOF_FIELD(struct zms_ate, crc8),
			  sizeof(struct zms_ate) - SIZEOF_FIELD(struct zms_ate, crc8));
	if (crc8 == entry->crc8) {
		return 0;
	}

	return 1;
}

/* zms_ate_valid validates an ate in the current sector by checking if the ate crc is valid
 * and its cycle cnt matches the cycle cnt of the active sector
 *
 * return 1 if ATE is valid,
 *        0 otherwise
 *
 * see: zms_ate_valid_different_sector
 */
static int zms_ate_valid(struct zms_fs *fs, const struct zms_ate *entry)
{
	return zms_ate_valid_different_sector(fs, entry, fs->sector_cycle);
}

/* zms_ate_valid_different_sector validates an ate that is in a different
 * sector than the active one. It takes as argument the cycle_cnt of the
 * sector where the ATE to be validated is stored
 *     return 1 if crc8 and cycle_cnt are valid,
 *            0 otherwise
 */
static int zms_ate_valid_different_sector(struct zms_fs *fs, const struct zms_ate *entry,
					  uint8_t cycle_cnt)
{
	if ((cycle_cnt != entry->cycle_cnt) || zms_ate_crc8_check(entry)) {
		return 0;
	}

	return 1;
}

static inline int zms_get_cycle_on_sector_change(struct zms_fs *fs, uint64_t addr,
						 int previous_sector_num, uint8_t *cycle_cnt)
{
	int rc;

	/* read the ate cycle only when we change the sector
	 * or if it is the first read
	 */
	if (SECTOR_NUM(addr) != previous_sector_num) {
		rc = zms_get_sector_cycle(fs, addr, cycle_cnt);
		if (rc == -ENOENT) {
			/* sector never used */
			*cycle_cnt = 0;
		} else if (rc) {
			/* bad flash read */
			return rc;
		}
	}

	return 0;
}

/* zms_close_ate_valid validates a sector close ate.
 * A valid sector close ate should be:
 * - a valid ate
 * - with len = 0 and id = ZMS_HEAD_ID
 * - and offset points to location at ate multiple from sector size
 * return true if valid, false otherwise
 */
static bool zms_close_ate_valid(struct zms_fs *fs, const struct zms_ate *entry)
{
	return (zms_ate_valid_different_sector(fs, entry, entry->cycle_cnt) && (!entry->len) &&
		(entry->id == ZMS_HEAD_ID) && !((fs->sector_size - entry->offset) % fs->ate_size));
}

/* zms_empty_ate_valid validates an sector empty ate.
 * A valid sector empty ate should be:
 * - a valid ate
 * - with len = 0xffff and id = 0xffffffff
 * return true if valid, false otherwise
 */
static bool zms_empty_ate_valid(struct zms_fs *fs, const struct zms_ate *entry)
{
	return (zms_ate_valid_different_sector(fs, entry, entry->cycle_cnt) &&
		(entry->len == 0xffff) && (entry->id == ZMS_HEAD_ID));
}

/* zms_gc_done_ate_valid validates a garbage collector done ATE
 * Valid gc_done_ate:
 * - valid ate
 * - len = 0
 * - id = 0xffffffff
 * return true if valid, false otherwise
 */
static bool zms_gc_done_ate_valid(struct zms_fs *fs, const struct zms_ate *entry)
{
	return (zms_ate_valid_different_sector(fs, entry, entry->cycle_cnt) && (!entry->len) &&
		(entry->id == ZMS_HEAD_ID));
}

/* Read empty and close ATE of the sector where belongs address "addr" and
 * validates that the sector is closed.
 * retval: 0 if sector is not close
 * retval: 1 is sector is closed
 * retval: < 0 if read of the header failed.
 */
static int zms_validate_closed_sector(struct zms_fs *fs, uint64_t addr, struct zms_ate *empty_ate,
				      struct zms_ate *close_ate)
{
	int rc;

	/* read the header ATEs */
	rc = zms_get_sector_header(fs, addr, empty_ate, close_ate);
	if (rc) {
		return rc;
	}

	if (zms_empty_ate_valid(fs, empty_ate) && zms_close_ate_valid(fs, close_ate) &&
	    (empty_ate->cycle_cnt == close_ate->cycle_cnt)) {
		/* Closed sector validated */
		return 1;
	}

	return 0;
}

/* store an entry in flash */
static int zms_flash_write_entry(struct zms_fs *fs, uint32_t id, const void *data, size_t len)
{
	int rc;
	struct zms_ate entry;

	/* Initialize all members to 0 */
	memset(&entry, 0, sizeof(struct zms_ate));

	entry.id = id;
	entry.len = (uint16_t)len;
	entry.cycle_cnt = fs->sector_cycle;

	if (len > ZMS_DATA_IN_ATE_SIZE) {
		/* only compute CRC if len is greater than 8 bytes */
		if (IS_ENABLED(CONFIG_ZMS_DATA_CRC)) {
			entry.data_crc = crc32_ieee(data, len);
		}
		entry.offset = (uint32_t)SECTOR_OFFSET(fs->data_wra);
	} else if ((len > 0) && (len <= ZMS_DATA_IN_ATE_SIZE)) {
		/* Copy data into entry for small data ( < 8B) */
		memcpy(&entry.data, data, len);
	}

	zms_ate_crc8_update(&entry);

	if (len > ZMS_DATA_IN_ATE_SIZE) {
		rc = zms_flash_data_wrt(fs, data, len);
		if (rc) {
			return rc;
		}
	}

	rc = zms_flash_ate_wrt(fs, &entry);
	if (rc) {
		return rc;
	}

	return 0;
}

/* end of flash routines */

/* Search for the last valid ATE written in a sector and also update data write address
 */
static int zms_recover_last_ate(struct zms_fs *fs, uint64_t *addr, uint64_t *data_wra)
{
	uint64_t data_end_addr;
	uint64_t ate_end_addr;
	struct zms_ate end_ate;
	int rc;

	LOG_DBG("%s: recovering last ate from sector %llu", get_fs_name(fs), SECTOR_NUM(*addr));

	/* skip close and empty ATE */
	*addr -= 2 * fs->ate_size;

	ate_end_addr = *addr;
	data_end_addr = *addr & ADDR_SECT_MASK;
	/* Initialize the data_wra to the first address of the sector */
	*data_wra = data_end_addr;

	while (ate_end_addr > data_end_addr) {
		rc = zms_flash_ate_rd(fs, ate_end_addr, &end_ate);
		if (rc) {
			return rc;
		}
		if (zms_ate_valid(fs, &end_ate)) {
			/* found a valid ate, update data_end_addr and *addr */
			data_end_addr &= ADDR_SECT_MASK;
			if (end_ate.len > ZMS_DATA_IN_ATE_SIZE) {
				data_end_addr += end_ate.offset + zms_al_size(fs, end_ate.len);
				*data_wra = data_end_addr;
			}
			*addr = ate_end_addr;
		}
		ate_end_addr -= fs->ate_size;
	}

	return 0;
}

/* compute previous addr of ATE */
static int zms_compute_prev_addr(struct zms_fs *fs, uint64_t *addr)
{
	int sec_closed;
	struct zms_ate empty_ate;
	struct zms_ate close_ate;

	*addr += fs->ate_size;
	if ((SECTOR_OFFSET(*addr)) != (fs->sector_size - 2 * fs->ate_size)) {
		return 0;
	}

	/* last ate in sector, do jump to previous sector */
	if (SECTOR_NUM(*addr) == 0U) {
		*addr += ((uint64_t)(fs->sector_count - 1) << ADDR_SECT_SHIFT);
	} else {
		*addr -= (1ULL << ADDR_SECT_SHIFT);
	}

	/* verify if the sector is closed */
	sec_closed = zms_validate_closed_sector(fs, *addr, &empty_ate, &close_ate);
	if (sec_closed < 0) {
		return sec_closed;
	}

	/* Non Closed Sector */
	if (!sec_closed) {
		/* at the end of filesystem */
		*addr = fs->ate_wra;
		return 0;
	}

	/* Update the address here because the header ATEs are valid.*/
	(*addr) &= ADDR_SECT_MASK;
	(*addr) += close_ate.offset;

	return 0;
}

/* walking through allocation entry list, from newest to oldest entries
 * read ate from addr, modify addr to the previous ate
 */
static int zms_prev_ate(struct zms_fs *fs, uint64_t *addr, struct zms_ate *ate)
{
	int rc;

	rc = zms_flash_ate_rd(fs, *addr, ate);
	if (rc) {
		return rc;
	}

	return zms_compute_prev_addr(fs, addr);
}

static void zms_sector_advance(struct zms_fs *fs, uint64_t *addr)
{
	*addr += (1ULL << ADDR_SECT_SHIFT);
	if ((*addr >> ADDR_SECT_SHIFT) == fs->sector_count) {
		*addr -= ((uint64_t)fs->sector_count << ADDR_SECT_SHIFT);
	}
}

/* allocation entry close (this closes the current sector) by writing offset
 * of last ate to the sector end.
 */
static int zms_sector_close(struct zms_fs *fs)
{
	int rc;
	struct zms_ate close_ate;
	struct zms_ate garbage_ate;

	close_ate.id = ZMS_HEAD_ID;
	close_ate.len = 0U;
	close_ate.offset = (uint32_t)SECTOR_OFFSET(fs->ate_wra + fs->ate_size);
	close_ate.metadata = 0xffffffff;
	close_ate.cycle_cnt = fs->sector_cycle;

	/* When we close the sector, we must write all non used ATE with
	 * a non valid (Junk) ATE.
	 * This is needed to avoid some corner cases where some ATEs are
	 * not overwritten and become valid when the cycle counter wrap again
	 * to the same cycle counter of the old ATE.
	 * Example :
	 * - An ATE.cycl_cnt == 0 is written as last ATE of the sector
	   - This ATE was never overwritten in the next 255 cycles because of
	     large data size
	   - Next 256th cycle the leading cycle_cnt is 0, this ATE becomes
	     valid even if it is not the case.
	 */
	memset(&garbage_ate, fs->flash_parameters->erase_value, sizeof(garbage_ate));
	while (SECTOR_OFFSET(fs->ate_wra) && (fs->ate_wra >= fs->data_wra)) {
		rc = zms_flash_ate_wrt(fs, &garbage_ate);
		if (rc) {
			return rc;
		}
	}

	fs->ate_wra = zms_close_ate_addr(fs, fs->ate_wra);

	zms_ate_crc8_update(&close_ate);

	(void)zms_flash_ate_wrt(fs, &close_ate);

	zms_sector_advance(fs, &fs->ate_wra);

	rc = zms_get_sector_cycle(fs, fs->ate_wra, &fs->sector_cycle);
	if (rc == -ENOENT) {
		/* sector never used */
		fs->sector_cycle = 0;
	} else if (rc) {
		/* bad flash read */
		return rc;
	}

	fs->data_wra = fs->ate_wra & ADDR_SECT_MASK;

	return 0;
}

static int zms_add_gc_done_ate(struct zms_fs *fs)
{
	struct zms_ate gc_done_ate;

	LOG_DBG("%s: adding gc done ate at %llx", get_fs_name(fs), fs->ate_wra);
	gc_done_ate.id = ZMS_HEAD_ID;
	gc_done_ate.len = 0U;
	gc_done_ate.offset = (uint32_t)SECTOR_OFFSET(fs->data_wra);
	gc_done_ate.metadata = 0xffffffff;
	gc_done_ate.cycle_cnt = fs->sector_cycle;

	zms_ate_crc8_update(&gc_done_ate);

	return zms_flash_ate_wrt(fs, &gc_done_ate);
}

static int zms_add_empty_ate(struct zms_fs *fs, uint64_t addr)
{
	struct zms_ate empty_ate;
	uint8_t cycle_cnt;
	int rc = 0;
	uint64_t previous_ate_wra;

	addr &= ADDR_SECT_MASK;

	LOG_DBG("%s: adding empty ate at %llx", get_fs_name(fs),
		(uint64_t)(addr + fs->sector_size - fs->ate_size));
	empty_ate.id = ZMS_HEAD_ID;
	empty_ate.len = 0xffff;
	empty_ate.offset = 0U;
	empty_ate.metadata =
		FIELD_PREP(ZMS_MAGIC_NUMBER_MASK, ZMS_MAGIC_NUMBER) | ZMS_DEFAULT_VERSION;

	rc = zms_get_sector_cycle(fs, addr, &cycle_cnt);
	if (rc == -ENOENT) {
		/* sector never used */
		cycle_cnt = 0;
	} else if (rc) {
		/* bad flash read */
		return rc;
	}

	/* increase cycle counter */
	empty_ate.cycle_cnt = (cycle_cnt + 1) % BIT(8);
	zms_ate_crc8_update(&empty_ate);

	/* Adding empty ate to this sector changes fs->ate_wra value
	 * Restore the ate_wra of the current sector after this
	 */
	previous_ate_wra = fs->ate_wra;
	fs->ate_wra = zms_empty_ate_addr(fs, addr);
	rc = zms_flash_ate_wrt(fs, &empty_ate);
	if (rc) {
		return rc;
	}
	fs->ate_wra = previous_ate_wra;

	return 0;
}

static int zms_get_sector_cycle(struct zms_fs *fs, uint64_t addr, uint8_t *cycle_cnt)
{
	int rc;
	struct zms_ate empty_ate;
	uint64_t empty_addr;

	empty_addr = zms_empty_ate_addr(fs, addr);

	/* read the cycle counter of the current sector */
	rc = zms_flash_ate_rd(fs, empty_addr, &empty_ate);
	if (rc < 0) {
		/* flash error */
		return rc;
	}

	if (zms_empty_ate_valid(fs, &empty_ate)) {
		*cycle_cnt = empty_ate.cycle_cnt;
		return 0;
	}

	/* there is no empty ATE in this sector */
	return -ENOENT;
}

static int zms_get_sector_header(struct zms_fs *fs, uint64_t addr, struct zms_ate *empty_ate,
				 struct zms_ate *close_ate)
{
	int rc;
	uint64_t close_addr;

	close_addr = zms_close_ate_addr(fs, addr);
	/* read the second ate in the sector to get the close ATE */
	rc = zms_flash_ate_rd(fs, close_addr, close_ate);
	if (rc) {
		return rc;
	}

	/* read the first ate in the sector to get the empty ATE */
	rc = zms_flash_ate_rd(fs, close_addr + fs->ate_size, empty_ate);
	if (rc) {
		return rc;
	}

	return 0;
}

/**
 * @brief Helper to find an ATE using its ID
 *
 * @param fs Pointer to file system
 * @param id Id of the entry to be found
 * @param start_addr Address from where the search will start
 * @param end_addr Address where the search will stop
 * @param ate pointer to the found ATE if it exists
 * @param ate_addr Pointer to the address of the found ATE
 *
 * @retval 0 No ATE is found
 * @retval 1 valid ATE with same ID found
 * @retval < 0 An error happened
 */
static int zms_find_ate_with_id(struct zms_fs *fs, uint32_t id, uint64_t start_addr,
				uint64_t end_addr, struct zms_ate *ate, uint64_t *ate_addr,
				int32_t *loop_cnt, int32_t loop_cnt_max)
{
	int rc;
	int previous_sector_num = ZMS_INVALID_SECTOR_NUM;
	uint64_t wlk_prev_addr = ZMS_LOOKUP_CACHE_NO_ADDR;
	uint64_t wlk_addr;
	int prev_found = 0;
	struct zms_ate wlk_ate;
	uint8_t current_cycle;

	wlk_addr = start_addr;

	bool first_sect_border_seen = false;
	uint32_t end_sect = ZMS_INVALID_SECTOR_NUM;
	int32_t loop_cnt_l = 0;
	if (!loop_cnt) {
		loop_cnt = &loop_cnt_l;
	}
	if (loop_cnt_max && (*loop_cnt >= loop_cnt_max)) {
		return 0;
	}

	do {
		*loop_cnt += 1;
		wlk_prev_addr = wlk_addr;
		rc = zms_prev_ate(fs, &wlk_addr, &wlk_ate);
		if (rc) {
			return rc;
		}
		const int32_t prev_sect = SECTOR_NUM(wlk_prev_addr);
		if (wlk_ate.id == id) {
			/* read the ate cycle only when we change the sector or if it is
			 * the first read ( previous_sector_num == ZMS_INVALID_SECTOR_NUM).
			 */
			rc = zms_get_cycle_on_sector_change(fs, wlk_prev_addr, previous_sector_num,
							    &current_cycle);
			if (rc) {
				return rc;
			}
			if (zms_ate_valid_different_sector(fs, &wlk_ate, current_cycle)) {
				prev_found = 1;
				break;
			}
			previous_sector_num = prev_sect;
		}
		const int32_t current_sect = SECTOR_NUM(wlk_addr);
		if (current_sect != prev_sect) {
			if (first_sect_border_seen) {
				if (current_sect == end_sect) {
					break;
				}
			} else {
				end_sect = current_sect;
				first_sect_border_seen = true;
			}
		}
	} while ((wlk_addr != end_addr) && (!loop_cnt_max || (*loop_cnt < loop_cnt_max)));

	if (prev_found > 0) {
		if (ate) {
			*ate = wlk_ate;
		}
		if (ate_addr) {
			*ate_addr = wlk_prev_addr;
		}
	}
	if (*loop_cnt >= 100000) {
		LOG_DBG("%s: loop_cnt: %d, id: %d, sa: 0x%llx, ea: 0x%llx, atea: 0x%llx, wa: "
			"0x%llx, wpa: 0x%llx, first_sect_border_seen: %d @ 0x%x",
			get_fs_name(fs), *loop_cnt, id, start_addr, end_addr, *ate_addr, wlk_addr,
			wlk_prev_addr, first_sect_border_seen, end_sect);
	}
	return prev_found;
}

/* garbage collection: the address ate_wra has been updated to the new sector
 * that has just been started. The data to gc is in the sector after this new
 * sector.
 */
static int zms_gc(struct zms_fs *fs)
{
	int rc;
	int sec_closed;
	struct zms_ate close_ate;
	struct zms_ate gc_ate;
	struct zms_ate wlk_ate;
	struct zms_ate empty_ate;
	uint64_t sec_addr;
	uint64_t gc_addr;
	uint64_t gc_prev_addr;
	uint64_t wlk_addr;
	uint64_t wlk_prev_addr;
	uint64_t data_addr;
	uint64_t stop_addr;
	uint8_t previous_cycle = 0;

	rc = zms_get_sector_cycle(fs, fs->ate_wra, &fs->sector_cycle);
	if (rc == -ENOENT) {
		/* Erase this new unused sector if needed */
		rc = zms_flash_erase_sector(fs, fs->ate_wra);
		if (rc) {
			return rc;
		}
		/* sector never used */
		rc = zms_add_empty_ate(fs, fs->ate_wra);
		if (rc) {
			return rc;
		}
		/* At this step we are sure that empty ATE exist.
		 * If not, then there is an I/O problem.
		 */
		rc = zms_get_sector_cycle(fs, fs->ate_wra, &fs->sector_cycle);
		if (rc) {
			return rc;
		}
	} else if (rc) {
		/* bad flash read */
		return rc;
	}
	previous_cycle = fs->sector_cycle;

	sec_addr = (fs->ate_wra & ADDR_SECT_MASK);
	zms_sector_advance(fs, &sec_addr);
	gc_addr = sec_addr + fs->sector_size - fs->ate_size;

	/* verify if the sector is closed */
	sec_closed = zms_validate_closed_sector(fs, gc_addr, &empty_ate, &close_ate);
	if (sec_closed < 0) {
		return sec_closed;
	}

	/* if the sector is not closed don't do gc */
	if (!sec_closed) {
		goto gc_done;
	}

	/* update sector_cycle */
	fs->sector_cycle = empty_ate.cycle_cnt;

	/* stop_addr points to the first ATE before the header ATEs */
	stop_addr = gc_addr - 2 * fs->ate_size;
	/* At this step empty & close ATEs are valid.
	 * let's start the GC
	 */
	gc_addr &= ADDR_SECT_MASK;
	gc_addr += close_ate.offset;

	do {
		gc_prev_addr = gc_addr;
		rc = zms_prev_ate(fs, &gc_addr, &gc_ate);
		if (rc) {
			return rc;
		}

		if (!zms_ate_valid(fs, &gc_ate)) {
			continue;
		}
		if (!gc_ate.len) {
			zms_read_cache_update(fs, gc_ate.id, ZMS_LOOKUP_CACHE_NO_ADDR, false, true);
			continue;
		}

#ifdef CONFIG_ZMS_LOOKUP_CACHE
		wlk_addr = fs->lookup_cache[zms_lookup_cache_pos(gc_ate.id)];

		if (wlk_addr == ZMS_LOOKUP_CACHE_NO_ADDR) {
			wlk_addr = fs->ate_wra;
		}
#else
		wlk_addr = fs->ate_wra;
#endif

		/* Initialize the wlk_prev_addr as if no previous ID will be found */
		wlk_prev_addr = gc_prev_addr;
		/* Search for a previous valid ATE with the same ID. If it doesn't exist
		 * then wlk_prev_addr will be equal to gc_prev_addr.
		 */
#ifdef CONFIG_ZMS_LOOKUP_CACHE
		if (fs->invalidate_old_ates) {
			// no need to search: we know there is only max. 1 valid ate per id
			rc = 0;
		} else
#endif
		{
			rc = zms_find_ate_with_id(fs, gc_ate.id, wlk_addr, fs->ate_wra, &wlk_ate,
						  &wlk_prev_addr, NULL, 0);
			if (rc < 0) {
				return rc;
			}
		}

		/* if walk_addr has reached the same address as gc_addr, a copy is
		 * needed unless it is a deleted item.
		 */
		if (wlk_prev_addr == gc_prev_addr) {
			/* copy needed */
			LOG_DBG("%s: moving %d, len %d", get_fs_name(fs), gc_ate.id, gc_ate.len);

			if (gc_ate.len > ZMS_DATA_IN_ATE_SIZE) {
				/* Copy Data only when len > 8
				 * Otherwise, Data is already inside ATE
				 */
				data_addr = (gc_prev_addr & ADDR_SECT_MASK);
				data_addr += gc_ate.offset;
				gc_ate.offset = (uint32_t)SECTOR_OFFSET(fs->data_wra);

				rc = zms_flash_block_move(fs, data_addr, gc_ate.len);
				if (rc) {
					return rc;
				}
			}

			gc_ate.cycle_cnt = previous_cycle;
			zms_ate_crc8_update(&gc_ate);
			zms_read_cache_update(fs, gc_ate.id, fs->ate_wra, true, true);
			rc = zms_flash_ate_wrt(fs, &gc_ate);
			if (rc) {
				return rc;
			}
		}
	} while (gc_prev_addr != stop_addr);

gc_done:

	/* restore the previous sector_cycle */
	fs->sector_cycle = previous_cycle;

	/* Write a GC_done ATE to mark the end of this operation
	 */

	rc = zms_add_gc_done_ate(fs);
	if (rc) {
		return rc;
	}

	/* Erase the GC'ed sector when needed */
	rc = zms_flash_erase_sector(fs, sec_addr);
	if (rc) {
		return rc;
	}

#ifdef CONFIG_ZMS_LOOKUP_CACHE
	zms_lookup_cache_invalidate(fs, sec_addr >> ADDR_SECT_SHIFT);
#endif
	rc = zms_add_empty_ate(fs, sec_addr);

	return rc;
}

int zms_clear(struct zms_fs *fs)
{
	int rc;
	uint64_t addr;

	if (!fs->ready) {
		LOG_ERR("%s: %s: zms not initialized", __func__, get_fs_name(fs));
		return -EACCES;
	}

	k_mutex_lock(&fs->zms_lock, K_FOREVER);
	for (uint32_t i = 0; i < fs->sector_count; i++) {
		addr = (uint64_t)i << ADDR_SECT_SHIFT;
		rc = zms_flash_erase_sector(fs, addr);
		if (rc) {
			goto end;
		}
		rc = zms_add_empty_ate(fs, addr);
		if (rc) {
			goto end;
		}
	}

	/* zms needs to be reinitialized after clearing */
	fs->ready = false;

end:
	k_mutex_unlock(&fs->zms_lock);

	return 0;
}

static int zms_init(struct zms_fs *fs)
{
	int rc;
	int sec_closed;
	struct zms_ate last_ate;
	struct zms_ate first_ate;
	struct zms_ate close_ate;
	struct zms_ate empty_ate;
	uint64_t addr = 0U;
	uint64_t data_wra = 0U;
	uint32_t i;
	uint32_t closed_sectors = 0;
	bool zms_magic_exist = false;

	k_mutex_lock(&fs->zms_lock, K_FOREVER);

	/* step through the sectors to find a open sector following
	 * a closed sector, this is where zms can write.
	 */

	for (i = 0; i < fs->sector_count; i++) {
		addr = zms_close_ate_addr(fs, ((uint64_t)i << ADDR_SECT_SHIFT));

		/* verify if the sector is closed */
		sec_closed = zms_validate_closed_sector(fs, addr, &empty_ate, &close_ate);
		if (sec_closed < 0) {
			rc = sec_closed;
			goto end;
		}
		/* update cycle count */
		fs->sector_cycle = empty_ate.cycle_cnt;

		if (sec_closed == 1) {
			/* closed sector */
			closed_sectors++;
			/* Let's verify that this is a ZMS storage system */
			if (ZMS_GET_MAGIC_NUMBER(empty_ate.metadata) == ZMS_MAGIC_NUMBER) {
				zms_magic_exist = true;
				/* Let's check that we support this ZMS version */
				if (ZMS_GET_VERSION(empty_ate.metadata) != ZMS_DEFAULT_VERSION) {
					LOG_ERR("%s: %s: ZMS Version %lu is not supported",
						__func__, get_fs_name(fs),
						ZMS_GET_VERSION(empty_ate.metadata));
					rc = -EPROTONOSUPPORT;
					goto end;
				}
			}

			zms_sector_advance(fs, &addr);
			/* addr is pointing to the close ATE */
			/* verify if the sector is Open */
			sec_closed = zms_validate_closed_sector(fs, addr, &empty_ate, &close_ate);
			if (sec_closed < 0) {
				rc = sec_closed;
				goto end;
			}
			/* update cycle count */
			fs->sector_cycle = empty_ate.cycle_cnt;

			if (!sec_closed) {
				/* We found an Open sector following a closed one */
				break;
			}
		}
	}
	/* all sectors are closed, and zms magic number not found. This is not a zms fs */
	if ((closed_sectors == fs->sector_count) && !zms_magic_exist) {
		rc = -ENOTSUP;
		goto end;
	}
	/* TODO: add a recovery mechanism here if the ZMS magic number exist but all
	 * sectors are closed
	 */

	if (i == fs->sector_count) {
		/* none of the sectors were closed, which means that the first
		 * sector is the one in use, except if there are only 2 sectors.
		 * Let's check if the last sector has valid ATEs otherwise set
		 * the open sector to the first one.
		 */
		rc = zms_flash_ate_rd(fs, addr - fs->ate_size, &first_ate);
		if (rc) {
			goto end;
		}
		if (!zms_ate_valid(fs, &first_ate)) {
			zms_sector_advance(fs, &addr);
		}
		rc = zms_get_sector_header(fs, addr, &empty_ate, &close_ate);
		if (rc) {
			goto end;
		}

		if (zms_empty_ate_valid(fs, &empty_ate)) {
			/* Empty ATE is valid, let's verify that this is a ZMS storage system */
			if (ZMS_GET_MAGIC_NUMBER(empty_ate.metadata) == ZMS_MAGIC_NUMBER) {
				zms_magic_exist = true;
				/* Let's check the version */
				if (ZMS_GET_VERSION(empty_ate.metadata) != ZMS_DEFAULT_VERSION) {
					LOG_ERR("%s: %s: ZMS Version %lu is not supported",
						__func__, get_fs_name(fs),
						ZMS_GET_VERSION(empty_ate.metadata));
					rc = -EPROTONOSUPPORT;
					goto end;
				}
			}
		} else {
			rc = zms_flash_erase_sector(fs, addr);
			if (rc) {
				goto end;
			}
			rc = zms_add_empty_ate(fs, addr);
			if (rc) {
				goto end;
			}
		}
		rc = zms_get_sector_cycle(fs, addr, &fs->sector_cycle);
		if (rc == -ENOENT) {
			/* sector never used */
			fs->sector_cycle = 0;
		} else if (rc) {
			/* bad flash read */
			goto end;
		}
	}

	/* addr contains address of closing ate in the most recent sector,
	 * search for the last valid ate using the recover_last_ate routine
	 * and also update the data_wra
	 */
	rc = zms_recover_last_ate(fs, &addr, &data_wra);
	if (rc) {
		goto end;
	}

	/* addr contains address of the last valid ate in the most recent sector
	 * data_wra contains the data write address of the current sector
	 */
	fs->ate_wra = addr;
	fs->data_wra = data_wra;

	/* fs->ate_wra should point to the next available entry. This is normally
	 * the next position after the one found by the recovery function.
	 * Let's verify that it doesn't contain any valid ATE, otherwise search for
	 * an empty position
	 */
	while (fs->ate_wra >= fs->data_wra) {
		rc = zms_flash_ate_rd(fs, fs->ate_wra, &last_ate);
		if (rc) {
			goto end;
		}
		if (!zms_ate_valid(fs, &last_ate)) {
			/* found empty location */
			break;
		}

		/* ate on the last position within the sector is
		 * reserved for deletion an entry
		 */
		if ((fs->ate_wra == fs->data_wra) && last_ate.len) {
			/* not a delete ate */
			rc = -ESPIPE;
			goto end;
		}

		fs->ate_wra -= fs->ate_size;
	}

	/* The sector after the write sector is either empty with a valid empty ATE (regular case)
	 * or it has never been used or it is a closed sector (GC didn't finish)
	 * If it is a closed sector we must look for a valid GC done ATE in the current write
	 * sector, if it is missing, we need to restart gc because it has been interrupted.
	 * If no valid empty ATE is found then it has never been used. Just erase it by adding
	 * a valid empty ATE.
	 * When gc needs to be restarted, first erase the sector by adding an empty
	 * ATE otherwise the data might not fit into the sector.
	 */
	addr = zms_close_ate_addr(fs, fs->ate_wra);
	zms_sector_advance(fs, &addr);

	/* verify if the sector is closed */
	sec_closed = zms_validate_closed_sector(fs, addr, &empty_ate, &close_ate);
	if (sec_closed < 0) {
		rc = sec_closed;
		goto end;
	}

	if (sec_closed == 1) {
		/* The sector after fs->ate_wrt is closed.
		 * Look for a marker (gc_done_ate) that indicates that gc was finished.
		 */
		bool gc_done_marker = false;
		struct zms_ate gc_done_ate;

		fs->sector_cycle = empty_ate.cycle_cnt;
		addr = fs->ate_wra + fs->ate_size;
		while (SECTOR_OFFSET(addr) < (fs->sector_size - 2 * fs->ate_size)) {
			rc = zms_flash_ate_rd(fs, addr, &gc_done_ate);
			if (rc) {
				goto end;
			}

			if (zms_gc_done_ate_valid(fs, &gc_done_ate)) {
				break;
			}
			addr += fs->ate_size;
		}

		if (gc_done_marker) {
			/* erase the next sector */
			LOG_INF("%s: %s: GC Done marker found", __func__, get_fs_name(fs));
			addr = fs->ate_wra & ADDR_SECT_MASK;
			zms_sector_advance(fs, &addr);
			rc = zms_flash_erase_sector(fs, addr);
			if (rc < 0) {
				goto end;
			}
			rc = zms_add_empty_ate(fs, addr);
			goto end;
		}
		LOG_INF("%s: %s: no GC Done marker found: restarting gc", __func__,
			get_fs_name(fs));
		rc = zms_flash_erase_sector(fs, fs->ate_wra);
		if (rc) {
			goto end;
		}
		rc = zms_add_empty_ate(fs, fs->ate_wra);
		if (rc) {
			goto end;
		}

		/* Let's point to the first writable position */
		fs->ate_wra &= ADDR_SECT_MASK;
		fs->ate_wra += (fs->sector_size - 3 * fs->ate_size);
		fs->data_wra = (fs->ate_wra & ADDR_SECT_MASK);
#ifdef CONFIG_ZMS_LOOKUP_CACHE
		/**
		 * At this point, the lookup cache wasn't built but the gc function need to use it.
		 * So, temporarily, we set the lookup cache to the end of the fs.
		 * The cache will be rebuilt afterwards
		 **/
		for (i = 0; i < CONFIG_ZMS_LOOKUP_CACHE_SIZE; i++) {
			fs->lookup_cache[i] = fs->ate_wra;
		}
#endif
		rc = zms_gc(fs);
		goto end;
	}

end:
#ifdef CONFIG_ZMS_LOOKUP_CACHE
	if (!rc) {
		rc = zms_lookup_cache_rebuild(fs);
	}
#endif
	/* If the sector is empty add a gc done ate to avoid having insufficient
	 * space when doing gc.
	 */
	if ((!rc) && (SECTOR_OFFSET(fs->ate_wra) == (fs->sector_size - 3 * fs->ate_size))) {
		rc = zms_add_gc_done_ate(fs);
	}
	k_mutex_unlock(&fs->zms_lock);

	return rc;
}

int zms_mount(struct zms_fs *fs)
{
	int rc;
	struct flash_pages_info info;
	size_t write_block_size;

	k_mutex_init(&fs->zms_lock);

	fs->flash_parameters = flash_get_parameters(fs->flash_device);
	if (fs->flash_parameters == NULL) {
		LOG_ERR("%s: %s: could not obtain flash parameters", __func__, get_fs_name(fs));
		return -EINVAL;
	}

	fs->ate_size = zms_al_size(fs, sizeof(struct zms_ate));
	write_block_size = fs->flash_parameters->write_block_size;

	/* check that the write block size is supported */
	if (write_block_size > ZMS_BLOCK_SIZE || write_block_size == 0) {
		LOG_ERR("%s: %s: unsupported write block size", __func__, get_fs_name(fs));
		return -EINVAL;
	}

	/* When the device need erase operations before write let's check that
	 * sector size is a multiple of pagesize
	 */
	if (flash_params_get_erase_cap(fs->flash_parameters) & FLASH_ERASE_C_EXPLICIT) {
		rc = flash_get_page_info_by_offs(fs->flash_device, fs->offset, &info);
		if (rc) {
			LOG_ERR("%s: %s: unable to get page info", __func__, get_fs_name(fs));
			return -EINVAL;
		}
		if (!fs->sector_size || fs->sector_size % info.size) {
			LOG_ERR("%s: %s: invalid sector size", __func__, get_fs_name(fs));
			return -EINVAL;
		}
	}

	/* we need at least 5 aligned ATEs size as the minimum sector size
	 * 1 close ATE, 1 empty ATE, 1 GC done ATE, 1 Delete ATE, 1 ID/Value ATE
	 */
	if (fs->sector_size < ZMS_MIN_ATE_NUM * fs->ate_size) {
		LOG_ERR("%s: %s: invalid sector size, should be at least %zu", __func__,
			get_fs_name(fs), ZMS_MIN_ATE_NUM * fs->ate_size);
	}

	/* check the number of sectors, it should be at least 2 */
	if (fs->sector_count < 2) {
		LOG_ERR("%s: %s: configuration error - sector count below minimum requirement (2)",
			__func__, get_fs_name(fs));
		return -EINVAL;
	}

	rc = zms_init(fs);

	if (rc) {
		return rc;
	}

	/* zms is ready for use */
	fs->ready = true;

	LOG_INF("%s: %s: %u Sectors of %u bytes", __func__, get_fs_name(fs), fs->sector_count,
		fs->sector_size);
	LOG_INF("%s: %s: alloc wra: %llu, %llx", __func__, get_fs_name(fs), SECTOR_NUM(fs->ate_wra),
		SECTOR_OFFSET(fs->ate_wra));
	LOG_INF("%s: %s: data wra: %llu, %llx", __func__, get_fs_name(fs), SECTOR_NUM(fs->data_wra),
		SECTOR_OFFSET(fs->data_wra));

	return 0;
}

ssize_t zms_write(struct zms_fs *fs, uint32_t id, const void *data, size_t len)
{
	int rc;
	size_t data_size;
	uint64_t wlk_addr;
	uint64_t rd_addr;
	uint32_t gc_count;
	uint32_t required_space = 0U; /* no space, appropriate for delete ate */
	int32_t loop_count = 0;

	if (!fs->ready) {
		LOG_ERR("%s: %s: zms not initialized", __func__, get_fs_name(fs));
		return -EACCES;
	}

	data_size = zms_al_size(fs, len);

	/* The maximum data size is sector size - 5 ate
	 * where: 1 ate for data, 1 ate for sector close, 1 ate for empty,
	 * 1 ate for gc done, and 1 ate to always allow a delete.
	 * We cannot also store more than 64 KB of data
	 */
	if ((len > (fs->sector_size - 5 * fs->ate_size)) || (len > UINT16_MAX) ||
	    ((len > 0) && (data == NULL))) {
		return -EINVAL;
	}

	/* find latest entry with same id */
#ifdef CONFIG_ZMS_LOOKUP_CACHE
	wlk_addr = fs->lookup_cache[zms_lookup_cache_pos(id)];

	if (wlk_addr == ZMS_LOOKUP_CACHE_NO_ADDR) {
		wlk_addr = fs->ate_wra;
	}
#else
	wlk_addr = fs->ate_wra;
#endif
	rd_addr = wlk_addr;

#ifdef CONFIG_ZMS_NO_DOUBLE_WRITE
	/* Search for a previous valid ATE with the same ID */
	struct zms_ate wlk_ate;
	int prev_found = -1;
	if (fs->invalidate_old_ates && fs->highest_id_in_use_valid &&
	    (fs->highest_id_in_use < id)) {
		prev_found = 0;
	} else if (fs->invalidate_old_ates && fs->lowest_id_in_use_valid &&
		   (fs->lowest_id_in_use > id)) {
		prev_found = 0;
	} else {
		prev_found = zms_find_ate_with_id(fs, id, wlk_addr, fs->ate_wra, &wlk_ate, &rd_addr,
						  &loop_count, 0);
	}
	if (prev_found < 0) {
		return prev_found;
	}

	if (prev_found) {
#ifdef ZMS_DONT_KEEP_HISTORY
		const uint64_t ate_addr = rd_addr;
#endif
		/* previous entry found */
		if (len > ZMS_DATA_IN_ATE_SIZE) {
			rd_addr &= ADDR_SECT_MASK;
			rd_addr += wlk_ate.offset;
		}

		if (len == 0) {
			/* do not try to compare with empty data */
			if (wlk_ate.len == 0U) {
				/* skip delete entry as it is already the
				 * last one
				 */
				zms_read_cache_update(fs, id, rd_addr, true, true);
				return 0;
			}
#ifdef ZMS_DONT_KEEP_HISTORY
			else {
				if (fs->invalidate_old_ates) {
					zms_invalidate_ate(&wlk_ate);
					rc = zms_flash_al_wrt(fs, ate_addr, &wlk_ate,
							      sizeof(wlk_ate));
					if (rc < 0) {
						return rc;
					}
					// Between invalidating an old entry in flash and until
					// completing writing the new entry, there is a time where
					// concurrent readers see an unexpected -ENOENT.
					// TODO: we could try to protect this using a read mutex,
					// but I consider that too expensive atm.
				}
			}
#endif
		} else if (len == wlk_ate.len) {
			/* do not try to compare if lengths are not equal */
			/* compare the data and if equal return 0 */
			if (len <= ZMS_DATA_IN_ATE_SIZE) {
				rc = memcmp(&wlk_ate.data, data, len);
				if (!rc) {
					return 0;
				}
#ifdef ZMS_DONT_KEEP_HISTORY
				else if (fs->invalidate_old_ates) {
					zms_invalidate_ate(&wlk_ate);
					rc = zms_flash_al_wrt(fs, ate_addr, &wlk_ate,
							      sizeof(wlk_ate));
					if (rc < 0) {
						return rc;
					}
				}
#endif
			} else {
				rc = zms_flash_block_cmp(fs, rd_addr, data, len);
				if (rc <= 0) {
					return rc;
				}
#ifdef ZMS_DONT_KEEP_HISTORY
				else if (fs->invalidate_old_ates) {
					zms_invalidate_ate(&wlk_ate);
					rc = zms_flash_al_wrt(fs, ate_addr, &wlk_ate,
							      sizeof(wlk_ate));
					if (rc < 0) {
						return rc;
					}
				}
#endif
			}
		}
	} else {
		/* skip delete entry for non-existing entry */
		if (len == 0) {
			zms_read_cache_update(fs, id, ZMS_LOOKUP_CACHE_NO_ADDR, false, false);
			return 0;
		}
	}
#endif

	/* calculate required space if the entry contains data */
	if (data_size) {
		/* Leave space for delete ate */
		if (len > ZMS_DATA_IN_ATE_SIZE) {
			required_space = data_size + fs->ate_size;
		} else {
			required_space = fs->ate_size;
		}
	}

	k_mutex_lock(&fs->zms_lock, K_FOREVER);

	gc_count = 0;
	while (1) {
		if (gc_count == fs->sector_count) {
			/* gc'ed all sectors, no extra space will be created
			 * by extra gc.
			 */
			rc = -ENOSPC;
			goto end;
		}

		/* We need to make sure that we leave the ATE at address 0x0 of the sector
		 * empty (even for delete ATE). Otherwise, the fs->ate_wra will be decremented
		 * after this write by ate_size and it will underflow.
		 * So the first position of a sector (fs->ate_wra = 0x0) is forbidden for ATEs
		 * and the second position could be written only be a delete ATE.
		 */
		if ((SECTOR_OFFSET(fs->ate_wra)) &&
		    (fs->ate_wra >= (fs->data_wra + required_space)) &&
		    (SECTOR_OFFSET(fs->ate_wra - fs->ate_size) || !len)) {
			rc = zms_flash_write_entry(fs, id, data, len);
			if (rc) {
				goto end;
			}
			break;
		}
		rc = zms_sector_close(fs);
		if (rc) {
			LOG_ERR("%s: %s: failed to close the sector, returned = %d", __func__,
				get_fs_name(fs), rc);
			goto end;
		}
		rc = zms_gc(fs);
		if (rc) {
			LOG_ERR("%s: %s: garbage collection failed, returned = %d", __func__,
				get_fs_name(fs), rc);
			goto end;
		}
		gc_count++;
	}
	rc = len;
#ifdef CONFIG_ZMS_LOOKUP_CACHE
	zms_read_cache_update(fs, id, fs->ate_wra, true, prev_found > 0);
	LOG_DBG("%s: id: %u, len: %d, ate search loops: %d, gc loops: %d (max. %d), "
		"num_valid_ates: %d, lowest id: %u, highest id: %u",
		get_fs_name(fs), id, len, loop_count, gc_count, fs->sector_count,
		fs->num_valid_ates, fs->lowest_id_in_use, fs->highest_id_in_use);
#endif

end:
	k_mutex_unlock(&fs->zms_lock);
	return rc;
}

int zms_delete(struct zms_fs *fs, uint32_t id)
{
	return zms_write(fs, id, NULL, 0);
}

ssize_t zms_read_hist(struct zms_fs *fs, uint32_t id, void *data, size_t len, uint32_t cnt)
{
	int rc;
	int prev_found = 0;
	uint64_t wlk_addr;
	uint64_t end_addr;
	uint64_t rd_addr = 0;
	uint64_t wlk_prev_addr = 0;
	uint32_t cnt_his;
	struct zms_ate wlk_ate;
	int32_t loop_count = 0;
#ifdef CONFIG_ZMS_DATA_CRC
	uint32_t computed_data_crc;
#endif

	if (!fs->ready) {
		LOG_ERR("%s: %s: zms not initialized", __func__, get_fs_name(fs));
		return -EACCES;
	}

	cnt_his = 0U;

#ifdef CONFIG_ZMS_LOOKUP_CACHE
	wlk_addr = ZMS_LOOKUP_CACHE_NO_ADDR;
	if (fs->invalidate_old_ates) {
		if (fs->highest_id_in_use_valid && (id > fs->highest_id_in_use)) {
			return -ENOENT;
		}
		if (fs->lowest_id_in_use_valid && (id < fs->lowest_id_in_use)) {
			return -ENOENT;
		}
		rc = zms_quick_find_ate_with_id(fs, id, NULL, &wlk_addr, &loop_count);
		if (rc < 0) {
			return rc;
		}
	}
	if (wlk_addr == ZMS_LOOKUP_CACHE_NO_ADDR) {
		wlk_addr = fs->lookup_cache[zms_lookup_cache_pos(id)];
		if (wlk_addr == ZMS_LOOKUP_CACHE_NO_ADDR) {
			rc = -ENOENT;
			goto err;
		}
		end_addr = fs->ate_wra;
	} else {
		end_addr = wlk_addr;
	}
#else
	wlk_addr = fs->ate_wra;
	end_addr = wlk_addr;
#endif

	while (cnt_his <= cnt) {
		wlk_prev_addr = wlk_addr;
		/* Search for a previous valid ATE with the same ID */
		prev_found = zms_find_ate_with_id(fs, id, wlk_addr, end_addr, &wlk_ate,
						  &wlk_prev_addr, &loop_count, 0);
		if (prev_found < 0) {
			return prev_found;
		}
		if (prev_found) {
			cnt_his++;
			/* wlk_prev_addr contain the ATE address of the previous found ATE. */
			rd_addr = wlk_prev_addr;
			/*
			 * compute the previous ATE address in case we need to start
			 * the research again.
			 */
			rc = zms_compute_prev_addr(fs, &wlk_prev_addr);
			if (rc) {
				return rc;
			}
			/* wlk_addr will be the start research address in the next loop */
			wlk_addr = wlk_prev_addr;
		} else {
			break;
		}
	}

#ifdef CONFIG_ZMS_LOOKUP_CACHE
	LOG_DBG("%s: id: %u, cnt: %d, addr: 0x%llx, ate loops: %d", get_fs_name(fs), id, cnt,
		wlk_addr, loop_count);
	if (cnt == 0) {
		zms_read_cache_update(fs, id, rd_addr, prev_found > 0, true);
	}
#endif

	if (((!prev_found) || (wlk_ate.id != id)) || (wlk_ate.len == 0U) || (cnt_his < cnt)) {
		return -ENOENT;
	}

	if (wlk_ate.len <= ZMS_DATA_IN_ATE_SIZE) {
		/* data is stored in the ATE */
		if (data) {
			memcpy(data, &wlk_ate.data, MIN(len, wlk_ate.len));
		}
	} else {
		rd_addr &= ADDR_SECT_MASK;
		rd_addr += wlk_ate.offset;
		/* do not read or copy data if pointer is NULL */
		if (data) {
			rc = zms_flash_rd(fs, rd_addr, data, MIN(len, wlk_ate.len));
			if (rc) {
				goto err;
			}
		}
#ifdef CONFIG_ZMS_DATA_CRC
		/* Do not compute CRC for partial reads as CRC won't match */
		if (len >= wlk_ate.len) {
			computed_data_crc = crc32_ieee(data, wlk_ate.len);
			if (computed_data_crc != wlk_ate.data_crc) {
				LOG_ERR("%s: %s: invalid data CRC: ATE_CRC=0x%08X, "
					"computed_data_crc=0x%08X",
					__func__, get_fs_name(fs), wlk_ate.data_crc,
					computed_data_crc);
				return -EIO;
			}
		}
#endif
	}

	return wlk_ate.len;

err:
	return rc;
}

ssize_t zms_read(struct zms_fs *fs, uint32_t id, void *data, size_t len)
{
	int rc;

	rc = zms_read_hist(fs, id, data, len, 0);
	if (rc < 0) {
		return rc;
	}

	/* returns the minimum between ATE data length and requested length */
	return MIN(rc, len);
}

ssize_t zms_get_data_length(struct zms_fs *fs, uint32_t id)
{
	int rc;

	rc = zms_read_hist(fs, id, NULL, 0, 0);

	return rc;
}

ssize_t zms_calc_free_space(struct zms_fs *fs)
{
	int rc;
	int previous_sector_num = ZMS_INVALID_SECTOR_NUM;
	int prev_found = 0;
	int sec_closed;
	struct zms_ate step_ate;
	struct zms_ate wlk_ate;
	struct zms_ate empty_ate;
	struct zms_ate close_ate;
	uint64_t step_addr;
	uint64_t wlk_addr;
	uint64_t step_prev_addr;
	uint64_t wlk_prev_addr;
	uint64_t data_wra = 0U;
	uint8_t current_cycle;
	ssize_t free_space = 0;
	const uint32_t second_to_last_offset = (2 * fs->ate_size);

	if (!fs->ready) {
		LOG_ERR("%s: %s: zms not initialized", __func__, get_fs_name(fs));
		return -EACCES;
	}

	/*
	 * There is always a closing ATE , an empty ATE, a GC_done ATE and a reserved ATE for
	 * deletion in each sector.
	 * And there is always one reserved Sector for garbage collection operations
	 */
	free_space = (fs->sector_count - 1) * (fs->sector_size - 4 * fs->ate_size);

	step_addr = fs->ate_wra;

	do {
		step_prev_addr = step_addr;
		rc = zms_prev_ate(fs, &step_addr, &step_ate);
		if (rc) {
			return rc;
		}

		/* When changing the sector let's get the new cycle counter */
		rc = zms_get_cycle_on_sector_change(fs, step_prev_addr, previous_sector_num,
						    &current_cycle);
		if (rc) {
			return rc;
		}
		previous_sector_num = SECTOR_NUM(step_prev_addr);

		/* Invalid and deleted ATEs are free spaces.
		 * Header ATEs are already retrieved from free space
		 */
		if (!zms_ate_valid_different_sector(fs, &step_ate, current_cycle) ||
		    (step_ate.id == ZMS_HEAD_ID) || (step_ate.len == 0)) {
			continue;
		}

		wlk_addr = step_addr;
		/* Try to find if there is a previous valid ATE with same ID */
		prev_found = zms_find_ate_with_id(fs, step_ate.id, wlk_addr, step_addr, &wlk_ate,
						  &wlk_prev_addr, NULL, 0);
		if (prev_found < 0) {
			return prev_found;
		}

		/* If no previous ATE is found, then this is a valid ATE that cannot be
		 * Garbage Collected
		 */
		if (!prev_found || (wlk_prev_addr == step_prev_addr)) {
			if (step_ate.len > ZMS_DATA_IN_ATE_SIZE) {
				free_space -= zms_al_size(fs, step_ate.len);
			}
			free_space -= fs->ate_size;
		}
	} while (step_addr != fs->ate_wra);

	/* we must keep the sector_cycle before we start looking into special cases */
	current_cycle = fs->sector_cycle;

	/* Let's look now for special cases where some sectors have only ATEs with
	 * small data size.
	 */

	for (int i = 0; i < fs->sector_count; i++) {
		step_addr = zms_close_ate_addr(fs, ((uint64_t)i << ADDR_SECT_SHIFT));

		/* verify if the sector is closed */
		sec_closed = zms_validate_closed_sector(fs, step_addr, &empty_ate, &close_ate);
		if (sec_closed < 0) {
			return sec_closed;
		}

		/* If the sector is closed and its offset is pointing to a position less than the
		 * 3rd to last ATE position in a sector, it means that we need to leave the second
		 * to last ATE empty.
		 */
		if ((sec_closed == 1) && (close_ate.offset <= second_to_last_offset)) {
			free_space -= fs->ate_size;
		} else if (!sec_closed) {
			/* sector is open, let's recover the last ATE */
			fs->sector_cycle = empty_ate.cycle_cnt;
			rc = zms_recover_last_ate(fs, &step_addr, &data_wra);
			if (rc) {
				return rc;
			}
			if (SECTOR_OFFSET(step_addr) <= second_to_last_offset) {
				free_space -= fs->ate_size;
			}
		}
	}
	/* restore sector cycle */
	fs->sector_cycle = current_cycle;

	return free_space;
}

size_t zms_active_sector_free_space(struct zms_fs *fs)
{
	if (!fs->ready) {
		LOG_ERR("%s: %s: ZMS not initialized", __func__, get_fs_name(fs));
		return -EACCES;
	}

	return fs->ate_wra - fs->data_wra - fs->ate_size;
}

int zms_sector_use_next(struct zms_fs *fs)
{
	int ret;

	if (!fs->ready) {
		LOG_ERR("%s: %s: ZMS not initialized", __func__, get_fs_name(fs));
		return -EACCES;
	}

	k_mutex_lock(&fs->zms_lock, K_FOREVER);

	ret = zms_sector_close(fs);
	if (ret != 0) {
		goto end;
	}

	ret = zms_gc(fs);

end:
	k_mutex_unlock(&fs->zms_lock);
	return ret;
}
