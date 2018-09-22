/*  NVS: non volatile storage in flash
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <flash.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <nvs/nvs.h>
#include <crc8.h>
#include "nvs_priv.h"

/* basic routines */
/* _nvs_al_size returns size aligned to fs->write_block_size */
static inline size_t _nvs_al_size(struct nvs_fs *fs, size_t len)
{
	if (fs->write_block_size <= 1) {
		return len;
	}
	return (len + (fs->write_block_size - 1)) & ~(fs->write_block_size - 1);
}
/* end basic routines */

/* flash routines */
/* basic aligned flash write to nvs address */
static int _nvs_flash_al_wrt(struct nvs_fs *fs, u32_t addr, const void *data,
		      size_t len)
{
	int rc = 0;
	off_t offset;
	size_t blen;
	u8_t buf[fs->write_block_size];

	if (!len) {
		/* Nothing to write, avoid changing the flash protection */
		return 0;
	}

	offset = fs->offset;
	offset += fs->sector_size * (addr >> ADDR_SECT_SHIFT);
	offset += addr & ADDR_OFFS_MASK;

	rc = flash_write_protection_set(fs->flash_device, 0);
	if (rc) {
		/* flash protection set error */
		return rc;
	}
	blen = len & ~(fs->write_block_size - 1);
	if (blen > 0) {
		rc = flash_write(fs->flash_device, offset, data, blen);
		if (rc) {
			/* flash write error */
			goto end;
		}
		len -= blen;
		offset += blen;
		data += blen;
	}
	if (len) {
		memcpy(buf, data, len);
		(void)memset(buf + len, 0xff, fs->write_block_size - len);
		rc = flash_write(fs->flash_device, offset, buf,
				 fs->write_block_size);
		if (rc) {
			/* flash write error */
			goto end;
		}
	}

end:
	(void) flash_write_protection_set(fs->flash_device, 1);
	return rc;
}

/* basic flash read from nvs address */
static int _nvs_flash_rd(struct nvs_fs *fs, u32_t addr, void *data,
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
static int _nvs_flash_ate_wrt(struct nvs_fs *fs, const struct nvs_ate *entry)
{
	int rc;

	rc = _nvs_flash_al_wrt(fs, fs->ate_wra, entry,
			       sizeof(struct nvs_ate));
	fs->ate_wra -= _nvs_al_size(fs, sizeof(struct nvs_ate));

	return rc;
}

/* data write */
static int _nvs_flash_data_wrt(struct nvs_fs *fs, const void *data, size_t len)
{
	int rc;

	rc = _nvs_flash_al_wrt(fs, fs->data_wra, data, len);
	fs->data_wra += _nvs_al_size(fs, len);

	return rc;
}

/* flash ate read */
static int _nvs_flash_ate_rd(struct nvs_fs *fs, u32_t addr,
			     struct nvs_ate *entry)
{
	return _nvs_flash_rd(fs, addr, entry, sizeof(struct nvs_ate));
}

/* end of basic flash routines */

/* advanced flash routines */

/* _nvs_flash_block_cmp compares the data in flash at addr to data
 * in blocks of size NVS_BLOCK_SIZE aligned to fs->write_block_size
 * returns 0 if equal, 1 if not equal, errcode if error
 */
static int _nvs_flash_block_cmp(struct nvs_fs *fs, u32_t addr, const void *data,
			 size_t len)
{
	int rc;
	size_t bytes_to_cmp, block_size;
	u8_t buf[NVS_BLOCK_SIZE];

	block_size = NVS_BLOCK_SIZE & ~(fs->write_block_size - 1);
	while (len) {
		bytes_to_cmp = min(block_size, len);
		rc = _nvs_flash_rd(fs, addr, buf, bytes_to_cmp);
		if (rc) {
			return rc;
		}
		rc = memcmp(data, buf, bytes_to_cmp);
		if (rc) {
			return 1;
		}
		len -= bytes_to_cmp;
		addr += bytes_to_cmp;
		data += bytes_to_cmp;
	}
	return 0;
}

/* _nvs_flash_cmp_const compares the data in flash at addr to a constant
 * value. returns 0 if all data in flash is equal to value, 1 if not equal,
 * errcode if error
 */
static int _nvs_flash_cmp_const(struct nvs_fs *fs, u32_t addr, u8_t value,
			       size_t len)
{
	int rc;
	size_t bytes_to_cmp, block_size;
	u8_t cmp[NVS_BLOCK_SIZE];

	block_size = NVS_BLOCK_SIZE & ~(fs->write_block_size - 1);
	(void)memset(cmp, value, block_size);
	while (len) {
		bytes_to_cmp = min(block_size, len);
		rc = _nvs_flash_block_cmp(fs, addr, cmp, bytes_to_cmp);
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
static int _nvs_flash_block_move(struct nvs_fs *fs, u32_t addr, size_t len)
{
	int rc;
	size_t bytes_to_copy, block_size;
	u8_t buf[NVS_BLOCK_SIZE];

	block_size = NVS_BLOCK_SIZE & ~(fs->write_block_size - 1);

	while (len) {
		bytes_to_copy =	min(block_size, len);
		rc = _nvs_flash_rd(fs, addr, buf, bytes_to_copy);
		if (rc) {
			return rc;
		}
		rc = _nvs_flash_data_wrt(fs, buf, bytes_to_copy);
		if (rc) {
			return rc;
		}
		len -= bytes_to_copy;
		addr += bytes_to_copy;
	}
	return 0;
}

/* erase a sector by first checking it is used and then erasing if required
 * return 0 if OK, errorcode on error.
 */
static int _nvs_flash_erase_sector(struct nvs_fs *fs, u32_t addr)
{
	int rc;
	off_t offset;

	addr &= ADDR_SECT_MASK;
	rc = _nvs_flash_cmp_const(fs, addr, 0xff, fs->sector_size);
	if (rc <= 0) {
		/* flash error or empty sector */
		return rc;
	}

	offset = fs->offset;
	offset += fs->sector_size * (addr >> ADDR_SECT_SHIFT);

	rc = flash_write_protection_set(fs->flash_device, 0);
	if (rc) {
		/* flash protection set error */
		return rc;
	}
	SYS_LOG_DBG("Erasing flash at %"PRIx32", len %d",
		    offset, fs->sector_size);
	rc = flash_erase(fs->flash_device, offset, fs->sector_size);
	if (rc) {
		/* flash erase error */
		return rc;
	}
	(void) flash_write_protection_set(fs->flash_device, 1);
	return 0;
}

/* crc update on allocation entry */
static void _nvs_ate_crc8_update(struct nvs_ate *entry)
{
	u8_t crc8;

	crc8 = crc8_ccitt(0xff, entry, offsetof(struct nvs_ate, crc8));
	entry->crc8 = crc8;
}

/* crc check on allocation entry
 * returns 0 if OK, 1 on crc fail
 */
static int _nvs_ate_crc8_check(const struct nvs_ate *entry)
{
	u8_t crc8;

	crc8 = crc8_ccitt(0xff, entry, offsetof(struct nvs_ate, crc8));
	if (crc8 == entry->crc8) {
		return 0;
	}
	return 1;
}

/* store an entry in flash */
static int _nvs_flash_wrt_entry(struct nvs_fs *fs, u16_t id, const void *data,
			size_t len)
{
	int rc;
	struct nvs_ate entry;
	size_t ate_size;

	ate_size = _nvs_al_size(fs, sizeof(struct nvs_ate));

	entry.id = id;
	entry.offset = (u16_t)(fs->data_wra & ADDR_OFFS_MASK);
	entry.len = (u16_t)len;
	entry.part = 0xff;

	_nvs_ate_crc8_update(&entry);

	rc = _nvs_flash_data_wrt(fs, data, len);
	if (rc) {
		return rc;
	}
	rc = _nvs_flash_ate_wrt(fs, &entry);
	if (rc) {
		return rc;
	}

	if (len != 0) {
		fs->free_space -= _nvs_al_size(fs, len);
		fs->free_space -= ate_size;
	}
	return 0;
}
/* end of flash routines */

/* walking through allocation entry list, from newest to oldest entries
 * read ate from addr, modify addr to the previous ate
 */
static int _nvs_prev_ate(struct nvs_fs *fs, u32_t *addr, struct nvs_ate *ate)
{
	int rc;
	size_t ate_size;

	ate_size = _nvs_al_size(fs, sizeof(struct nvs_ate));

	rc = _nvs_flash_ate_rd(fs, *addr, ate);
	if (rc) {
		return rc;
	}

	while (1) {
		*addr += ate_size;
		if (((*addr) & ADDR_OFFS_MASK) != fs->sector_size)  {
			break;
		}
		/* last ate in sector, do jump to previous sector */
		if (((*addr) >> ADDR_SECT_SHIFT) == 0) {
			*addr += (fs->sector_count << ADDR_SECT_SHIFT);
		}
		(*addr) -= (1 << ADDR_SECT_SHIFT);
		while (1) {
			*addr -= ate_size;
			rc = _nvs_flash_cmp_const(fs, *addr, 0xff, ate_size);
			if (!rc) {
				break;
			}
			rc = _nvs_flash_cmp_const(fs, *addr, 0x00, ate_size);
			if (!rc) {
				break;
			}
			/* stop at the end of the sector in case it is not
			   closed properly */
			if (((*addr) & ADDR_OFFS_MASK) == 0) {
				break;
			}
		}
		/* stop after stepping through all sectors */
		if (*addr == fs->ate_wra) {
			break;
		}
	}
	return 0;
}

static void _nvs_sector_advance(struct nvs_fs *fs, u32_t *addr)
{
	*addr += (1 << ADDR_SECT_SHIFT);
	if ((*addr >> ADDR_SECT_SHIFT) == fs->sector_count) {
		*addr -= (fs->sector_count << ADDR_SECT_SHIFT);
	}
}

/* allocation entry close (this closes the current sector) by writing all
 * zeros at fs->ate_wra.
 */
static int _nvs_sector_close(struct nvs_fs *fs)
{
	int rc;
	u8_t buf[sizeof(struct nvs_ate)];
	size_t ate_size;

	ate_size = sizeof(struct nvs_ate);

	(void)memset(buf, 0, ate_size);

	rc = _nvs_flash_al_wrt(fs, fs->ate_wra, buf, ate_size);
	if (rc) {
		return rc;
	}

	fs->ate_wra &= ADDR_SECT_MASK;
	fs->ate_wra += (fs->sector_size - ate_size);
	_nvs_sector_advance(fs, &fs->ate_wra);

	fs->data_wra = fs->ate_wra & ADDR_SECT_MASK;

	return 0;
}


/* garbage collection: the address ate_wra has been updated to the new sector
 * that has just been started. The data to gc is in the sector after this new
 * sector.
 */
static int _nvs_gc(struct nvs_fs *fs)
{
	int rc;
	struct nvs_ate gc_ate, wlk_ate;
	u32_t gc_addr, wlk_addr, wlk_prev_addr, data_addr, sec_addr;
	size_t ate_size;

	ate_size = _nvs_al_size(fs, sizeof(struct nvs_ate));

	gc_addr = (fs->ate_wra & ADDR_SECT_MASK) + fs->sector_size - ate_size;
	_nvs_sector_advance(fs, &gc_addr);
	sec_addr = gc_addr;

	while (1) {
		/* if the sector is empty don't do gc */
		rc = _nvs_flash_cmp_const(fs, gc_addr, 0xff, ate_size);
		if (!rc) {
			break;
		}
		/* if sector end is reached stop gc */
		rc = _nvs_flash_cmp_const(fs, gc_addr, 0x00, ate_size);
		if (!rc) {
			break;
		}
		rc = _nvs_flash_ate_rd(fs, gc_addr, &gc_ate);
		if (rc) {
			return rc;
		}
		wlk_addr = fs->ate_wra;
		while (1) {
			wlk_prev_addr = wlk_addr;
			rc = _nvs_prev_ate(fs, &wlk_addr, &wlk_ate);
			if (rc) {
				return rc;
			}
			/* if ate with same id is reached and it has valid crc8
			 * we might need to copy.
			 */
			if ((wlk_ate.id == gc_ate.id) &&
			    (!_nvs_ate_crc8_check(&wlk_ate))) {
				break;
			}
		}
		/* if walk has reached the same address as gc_addr copy is
		 * needed unless it is a deleted item.
		 */
		if ((wlk_prev_addr == gc_addr) && gc_ate.len) {
			/* copy needed */
			SYS_LOG_DBG("Moving %d, len %d", gc_ate.id, gc_ate.len);

			data_addr = (gc_addr & ADDR_SECT_MASK);
			data_addr += gc_ate.offset;

			gc_ate.offset = (u16_t)(fs->data_wra & ADDR_OFFS_MASK);
			_nvs_ate_crc8_update(&gc_ate);

			rc = _nvs_flash_block_move(fs, data_addr, gc_ate.len);
			if (rc) {
				return rc;
			}

			rc = _nvs_flash_ate_wrt(fs, &gc_ate);
			if (rc) {
				return rc;
			}
		}
		gc_addr -= ate_size;
	}
	rc = _nvs_flash_erase_sector(fs, sec_addr);
	if (rc) {
		return rc;
	}
	return 0;
}

static int _nvs_update_free_space(struct nvs_fs *fs)
{

	int rc;
	struct nvs_ate step_ate, wlk_ate;
	u32_t step_addr, wlk_addr;
	size_t ate_size;

	ate_size = _nvs_al_size(fs, sizeof(struct nvs_ate));

	fs->free_space = 0;
	for (u16_t i = 1; i < fs->sector_count; i++) {
		fs->free_space += (fs->sector_size - ate_size);
	}

	step_addr = fs->ate_wra;

	while (1) {
		rc = _nvs_prev_ate(fs, &step_addr, &step_ate);
		if (rc) {
			return rc;
		}

		wlk_addr = fs->ate_wra;

		while (1) {
			rc = _nvs_prev_ate(fs, &wlk_addr, &wlk_ate);
			if (rc) {
				return rc;
			}
			if ((wlk_ate.id == step_ate.id) ||
			    (wlk_addr == fs->ate_wra)) {
				break;
			}

		}

		if ((wlk_addr == step_addr) && step_ate.len &&
		    (!_nvs_ate_crc8_check(&step_ate))) {
			/* count needed */
			fs->free_space -= _nvs_al_size(fs, step_ate.len);
			fs->free_space -= ate_size;
		}

		if (step_addr == fs->ate_wra) {
			break;
		}

	}
	return 0;
}

int nvs_clear(struct nvs_fs *fs)
{
	int rc;
	off_t addr;

	for (u16_t i = 0; i < fs->sector_count; i++) {
		addr = i << ADDR_SECT_SHIFT;
		rc = _nvs_flash_erase_sector(fs, addr);
		if (rc) {
			return rc;
		}
	}
	return 0;
}

int nvs_reinit(struct nvs_fs *fs)
{
	int rc;
	size_t ate_size, empty_len;
	struct nvs_ate ate;
	u32_t addr;


	k_mutex_lock(&fs->nvs_lock, K_FOREVER);

	ate_size = _nvs_al_size(fs, sizeof(struct nvs_ate));

	/* step through the sectors to find the last sector */
	for (u16_t i = 0; i < fs->sector_count; i++) {
		addr = (i << ADDR_SECT_SHIFT) + fs->sector_size - ate_size;
		fs->ate_wra = addr;
		rc = _nvs_prev_ate(fs, &addr, &ate);
		if (rc) {
			goto end;
		}
		rc = _nvs_flash_cmp_const(fs, addr - ate_size, 0x00, ate_size);
		if (!rc) {
			/* we are just before (or after) sector end */
			continue;
		}
		rc = _nvs_flash_cmp_const(fs, addr - ate_size, 0xff, ate_size);
		if (!rc) {
			break;
		}
	}

	fs->ate_wra = addr;
	fs->data_wra = addr & ADDR_SECT_MASK;

	/* Normally addr points to a location that does not contain all ff, so
	 * ate_size needs to be substracted to get ate_wra. However there is an
	 * exception, when addr is the very end of a sector; then we keep
	 * fs->ate_wra.
	 */
	rc = _nvs_flash_cmp_const(fs, addr, 0xff, ate_size);
	if (rc < 0) {
		goto end;
	}
	if (rc) {
		/* fs->ate_wra not at sector end */
		fs->ate_wra -= ate_size;
		rc = _nvs_flash_ate_rd(fs, addr, &ate);
		if (rc) {
			goto end;
		}
		if (!_nvs_ate_crc8_check(&ate)) {
		/* crc8 is ok, complete write of ate was performed */
			fs->data_wra += ate.offset;
			fs->data_wra += _nvs_al_size(fs, ate.len);
		}
	}

	/* possible data write after last ate write, update data_wra */
	while (1) {
		empty_len = fs->ate_wra - fs->data_wra;
		if (!empty_len) {
			break;
		}
		rc = _nvs_flash_cmp_const(fs, fs->data_wra, 0xff, empty_len);
		if (rc < 0) {
			goto end;
		}
		if (!rc) {
			break;
		}
		fs->data_wra += fs->write_block_size;
	}

	/* if the sector after the write sector is not empty gc was interrupted
	 * we need to restart gc, first erase the sector before restarting gc
	 * otherwise the data may not fit into the sector.
	 */
	addr = fs->ate_wra & ADDR_SECT_MASK;
	_nvs_sector_advance(fs, &addr);
	rc = _nvs_flash_cmp_const(fs, addr, 0xff, fs->sector_size);
	if (rc < 0) {
		goto end;
	}
	if (rc) {
		/* the sector after fs->ate_wrt is not empty */
		rc = _nvs_flash_erase_sector(fs, fs->ate_wra);
		if (rc) {
			goto end;
		}
		fs->ate_wra &= ADDR_SECT_MASK;
		fs->ate_wra += (fs->sector_size - ate_size);
		fs->data_wra = (fs->ate_wra & ADDR_SECT_MASK);
		rc = _nvs_gc(fs);
		if (rc) {
			goto end;
		}
	}

	rc = _nvs_update_free_space(fs);

end:
	k_mutex_unlock(&fs->nvs_lock);
	return rc;
}

int nvs_init(struct nvs_fs *fs, const char *dev_name)
{

	int rc;

	k_mutex_init(&fs->nvs_lock);

	fs->flash_device = device_get_binding(dev_name);
	if (!fs->flash_device) {
		SYS_LOG_ERR("No valid flash device found");
		return -ENXIO;
	}

	fs->write_block_size = flash_get_write_block_size(fs->flash_device);

	/* check the number of sectors, it should be at least 2 */
	if (fs->sector_count < 2) {
		SYS_LOG_ERR("Configuration error - sector count");
		return -EINVAL;
	}

	fs->locked = false;
	rc = nvs_reinit(fs);
	if (rc) {
		return rc;
	}

	SYS_LOG_INF("%d Sectors of %d bytes", fs->sector_count, fs->sector_size);
	SYS_LOG_INF("alloc wra: %d, %"PRIx32"",
		    (fs->ate_wra >> ADDR_SECT_SHIFT),
		    (fs->ate_wra & ADDR_OFFS_MASK));
	SYS_LOG_INF("data wra: %d, %"PRIx32"",
		    (fs->data_wra >> ADDR_SECT_SHIFT),
		    (fs->data_wra & ADDR_OFFS_MASK));
	SYS_LOG_INF("Free space: %d", fs->free_space);

	return 0;
}

ssize_t nvs_write(struct nvs_fs *fs, u16_t id, const void *data, size_t len)
{
	int rc, gc_count;
	size_t ate_size, data_size;
	struct nvs_ate wlk_ate;
	u32_t wlk_addr, rd_addr, freed_space;
	u16_t sector_freespace;

	ate_size = _nvs_al_size(fs, sizeof(struct nvs_ate));
	data_size = _nvs_al_size(fs, len);


	if ((len > (fs->sector_size - 2*ate_size)) ||
	    ((len > 0) && (data == NULL))) {
		return -EINVAL;
	}

	if (fs->locked) {
		return -EROFS;
	}

	/* find latest entry with same id */
	wlk_addr = fs->ate_wra;
	rd_addr = wlk_addr;
	freed_space = 0;

	while (1) {
		rd_addr = wlk_addr;
		rc = _nvs_prev_ate(fs, &wlk_addr, &wlk_ate);
		if (rc) {
			return rc;
		}
		if ((wlk_ate.id == id) && (!_nvs_ate_crc8_check(&wlk_ate))) {
			break;
		}
		if (wlk_addr == fs->ate_wra) {
			break;
		}
	}

	if (wlk_addr != fs->ate_wra) {
		/* previous entry found */
		rd_addr &= ADDR_SECT_MASK;
		rd_addr += wlk_ate.offset;

		if (len == 0) {
			/* do not try to compare with empty data */
			if (wlk_ate.len == 0) {
				return 0;
			}
		} else {
			/* compare the data and if equal return 0 */
			rc = _nvs_flash_block_cmp(fs, rd_addr, data, len);
			if (rc <= 0) {
				return rc;
			}
		}
		/* data different, calculate freed space */
		freed_space = _nvs_al_size(fs, wlk_ate.len);
		freed_space += ate_size;
	}

	k_mutex_lock(&fs->nvs_lock, K_FOREVER);

	fs->free_space += freed_space;
	if (fs->free_space < (data_size + ate_size)) {
		rc = -ENOSPC;
		goto end;
	}

	gc_count = 0;
	while (1) {
		if (gc_count == fs->sector_count) {
			rc = -EROFS;
			goto end;
		}
		sector_freespace = fs->ate_wra - fs->data_wra;
		if (sector_freespace >= data_size + ate_size) {
			rc = _nvs_flash_wrt_entry(fs, id, data, len);
			if (rc) {
				goto end;
			}
			break;
		}
		rc = _nvs_sector_close(fs);
		if (rc) {
			goto end;
		}
		rc = _nvs_gc(fs);
		if (rc) {
			goto end;
		}
		gc_count++;
	}
	rc = len;
end:
	k_mutex_unlock(&fs->nvs_lock);
	if (rc < 0) {
		fs->free_space -= freed_space;
	}
	if (rc == -EROFS) {
		fs->locked = true;
	}
	return rc;
}

int nvs_delete(struct nvs_fs *fs, u16_t id)
{
	return nvs_write(fs, id, NULL, 0);
}

ssize_t nvs_read_hist(struct nvs_fs *fs, u16_t id, void *data, size_t len,
		  u16_t cnt)
{
	int rc;
	u32_t wlk_addr, rd_addr;
	u16_t cnt_his;
	struct nvs_ate wlk_ate;
	size_t ate_size;

	ate_size = _nvs_al_size(fs, sizeof(struct nvs_ate));

	if (len > (fs->sector_size - 2*ate_size)) {
		return -EINVAL;
	}

	cnt_his = 0;

	wlk_addr = fs->ate_wra;
	rd_addr = wlk_addr;

	while (cnt_his <= cnt) {
		rd_addr = wlk_addr;
		rc = _nvs_prev_ate(fs, &wlk_addr, &wlk_ate);
		if (rc) {
			goto err;
		}
		if ((wlk_ate.id == id) &&  (!_nvs_ate_crc8_check(&wlk_ate))) {
			cnt_his++;
		}
		if (wlk_addr == fs->ate_wra) {
			break;
		}
	}

	if (((wlk_addr == fs->ate_wra) && (wlk_ate.id != id)) ||
	    (wlk_ate.len == 0) || (cnt_his < cnt)) {
		return -ENOENT;
	}

	rd_addr &= ADDR_SECT_MASK;
	rd_addr += wlk_ate.offset;
	rc = _nvs_flash_rd(fs, rd_addr, data, min(len, wlk_ate.len));
	if (rc) {
		goto err;
	}

	return wlk_ate.len;

err:
	return rc;
}

ssize_t nvs_read(struct nvs_fs *fs, u16_t id, void *data, size_t len)
{
	int rc;

	rc = nvs_read_hist(fs, id, data, len, 0);
	return rc;
}
