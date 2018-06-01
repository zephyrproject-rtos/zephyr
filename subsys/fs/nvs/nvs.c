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
#include "nvs_priv.h"

/* basic calculation routines */

static inline size_t get_wbs_aligned_len(struct nvs_fs *fs, size_t len)
{
	if (fs->write_block_size <= 1) {
		return len;
	}
	return (len + (fs->write_block_size - 1)) & ~(fs->write_block_size - 1);
}

size_t get_wbs_aligned_entry_len(struct nvs_fs *fs, size_t len)
{
	if (len < 0x80) {
		return get_wbs_aligned_len(fs, len + sizeof(u16_t)
					   + 2*sizeof(u8_t));
	}
	return get_wbs_aligned_len(fs, len + sizeof(u16_t)
				   + 2*sizeof(u16_t));
}

void _nvs_addr_advance(struct nvs_fs *fs, off_t *addr, size_t step)
{
	off_t fs_size = fs->sector_count * fs->sector_size;

	*addr += step;
	if (*addr >= fs_size) {
		*addr -= fs_size;
	}
}
void _nvs_addr_decrease(struct nvs_fs *fs, off_t *addr, size_t step)
{
	*addr -= step;
	if (*addr < 0) {
		*addr += (fs->sector_count * fs->sector_size);
	}
}

/* length is encoded in:
 *  1 byte if len < 128 byte: 0 b6 .. b0
 *  2 byte if len >= 128 byte 1 b14 .. b8 | b7 .. b0
 * _nvs_decode_length returns the number of byte used and updates the length
 * _nvs_decode_length is used to determine if the flash is unwritten, therefor
 * _nvs_decode_length returns 0xffff if the data in buf[0] and buf[1] is 0xff.
 */
size_t _nvs_decode_length(u8_t *buf, size_t *len)
{
	if (buf[0] & 0x80) {
		if (buf[0] == 0xff && buf[1] == 0xff) {
			return 0xffff;
		}
		*len = (buf[1] & 0xff) | ((buf[0]&0x7f) << 8);
		return sizeof(u16_t);
	}
	*len = buf[0];
	return sizeof(u8_t);
}
/* length is encoded in:
 *  1 byte if len < 128 byte
 *  2 byte if len >= 128 byte
 * _nvs_encode_length returns the number of byte used and fills the buffer
 * _nvs_encode_length returns 0xffff if the len is larger then NVS_MAX_LEN
 */
size_t _nvs_encode_length(u8_t *buf, size_t len)
{
	if (len < 0x80) {
		buf[0] = len;
		return sizeof(u8_t);
	}
	if (len > NVS_MAX_LEN) {
		return 0xffff;
	}
	buf[1] = len & 0xff;
	buf[0] = ((len >> 8) & 0x7f) | 0x80;
	return sizeof(u16_t);
}

/* end basic calculation routines */

/* basic flash routines */

/* basic flash write */
int _nvs_flash_write(struct nvs_fs *fs, off_t offset, const void *data,
		     size_t len)
{
	int rc;

	rc = flash_write_protection_set(fs->flash_device, 0);
	if (rc) {
		/* flash protection set error */
		return rc;
	}
	rc = flash_write(fs->flash_device, fs->offset + offset, data, len);
	if (rc) {
		return rc;
	}

	(void) flash_write_protection_set(fs->flash_device, 1);
	return 0;
}

/* basic flash read */
int _nvs_flash_read(struct nvs_fs *fs, off_t offset, void *data,
		     size_t len)
{
	int rc;

	rc = flash_read(fs->flash_device, fs->offset + offset, data, len);
	if (rc) {
		return rc;
	}
	return 0;
}

/* basic flash erase */
int _nvs_flash_erase(struct nvs_fs *fs, off_t offset, size_t len)
{
	int rc;

	rc = flash_write_protection_set(fs->flash_device, 0);
	if (rc) {
		/* flash protection set error */
		return rc;
	}
	SYS_LOG_DBG("Erasing flash at %"PRIx32", len %d", offset, len);
	rc = flash_erase(fs->flash_device, fs->offset + offset, len);
	if (rc) {
		/* flash erase error */
		return rc;
	}
	(void) flash_write_protection_set(fs->flash_device, 1);
	return 0;
}

/* end of basic flash routines */

/* advanced flash routines */

/* _nvs_flash_block_cmp compares the data in flash at offset to data
 * in blocks of size fs->write_block_size
 * returns 0 if equal, 1 if not equal, errcode if error
 */
int _nvs_flash_block_cmp(struct nvs_fs *fs, off_t offset, const void *data,
		   size_t len)
{
	int rc;
	size_t bytes_to_cmp, block_size;
	u8_t buf[fs->write_block_size];

	block_size = sizeof(buf);

	while (len) {
		bytes_to_cmp = min(block_size, len);
		rc = _nvs_flash_read(fs, offset, buf, bytes_to_cmp);
		if (rc) {
			return rc;
		}
		rc = memcmp(data, buf, bytes_to_cmp);
		if (rc) {
			return 1;
		}
		len -= bytes_to_cmp;
		offset += bytes_to_cmp;
		data += bytes_to_cmp;
	}
	return 0;
}

/* _nvs_flash_cmp_const compares the data in flash at offset to a constant
 * value. returns 0 if all data in flash is equal to value, 1 if not equal,
 * errcode if error
 */
int _nvs_flash_cmp_const(struct nvs_fs *fs, off_t offset, u8_t value,
			       size_t len)
{
	int rc;
	size_t bytes_to_cmp, block_size;
	u8_t buf[fs->write_block_size];
	u8_t cmp[fs->write_block_size];

	block_size = sizeof(buf);
	memset(cmp, value, block_size);
	while (len) {
		bytes_to_cmp = min(block_size, len);
		rc = _nvs_flash_read(fs, offset, buf, bytes_to_cmp);
		if (rc) {
			return rc;
		}
		rc = memcmp(cmp, buf, bytes_to_cmp);
		if (rc) {
			return 1;
		}
		len -= bytes_to_cmp;
		offset += bytes_to_cmp;
	}
	return 0;
}

/* flash block move: move a block at offset to the current write location
 * and updates the write location, len should be multiple of
 * fs->write_block_size.
 */
int _nvs_flash_block_move(struct nvs_fs *fs, off_t offset, size_t len)
{
	int rc;
	size_t bytes_to_copy, block_size;
	u8_t buf[fs->write_block_size];
	off_t write_location;

	write_location = fs->write_location;

	block_size = sizeof(buf);

	while (len) {
		bytes_to_copy =	min(block_size, len);
		rc = _nvs_flash_read(fs, offset, buf, bytes_to_copy);

		if (rc) {
			return rc;
		}
		rc = _nvs_flash_write(fs, write_location, buf,
				      bytes_to_copy);
		if (rc) {
			return rc;
		}
		len -= bytes_to_copy;
		offset += bytes_to_copy;
		write_location += bytes_to_copy;
	}
	/* update fs->write_location when all writes are successful */
	_nvs_addr_advance(fs, &fs->write_location, len);

	return 0;
}

/* store an entry in flash at location offset */
int _nvs_flash_wrt_entry(struct nvs_fs *fs, u16_t id, const void *data,
			 size_t len)
{
	int rc;
	off_t offset;
	size_t padding, flash_len, length_cnt, byte_cnt, blen, flen;
	u8_t lenstr[2], ilenstr[2];
	u8_t buf[fs->write_block_size];

	offset = fs->write_location;

	/* create the length string to be written at start and end */
	length_cnt = _nvs_encode_length(lenstr, len);

	if (length_cnt > 2) {
		return -EINVAL;
	}

	if (length_cnt == 1) {
		ilenstr[0] = lenstr[0];
	} else {
		ilenstr[0] = lenstr[1];
		ilenstr[1] = lenstr[0];
	}

	/* calculate the entry length in flash */
	flash_len = get_wbs_aligned_entry_len(fs, len);

	/* calculate the padding length */
	padding = flash_len - len - sizeof(u16_t) - 2*length_cnt;

	memcpy(buf, lenstr, length_cnt);
	byte_cnt = length_cnt;

	blen = (len + byte_cnt) &  ~(fs->write_block_size - 1);
	if (blen) {
		/* there is at least one write_block_size buffer to be
		 * written to flash, fill it with start of data
		 */
		flen = fs->write_block_size - byte_cnt;
		memcpy(buf + byte_cnt, data, flen);
		rc = _nvs_flash_write(fs, offset, buf,
					  fs->write_block_size);
		if (rc) {
			/* flash write or verify error */
			goto err;
		}
		byte_cnt = 0;
		offset += fs->write_block_size;
		blen -= fs->write_block_size;
		len -= flen;
		data += flen;
	}
	if (blen) {
		/* there is more data to be written, do it in one write */
		rc = _nvs_flash_write(fs, offset, data, blen);
		if (rc) {
			/* flash write or verify error */
			goto err;
		}
		offset += blen;
		len -= blen;
		data += blen;
	}
	blen = (len + padding) &  ~(fs->write_block_size - 1);
	if (len) {
		/* there still is some data left that needs to be written */
		memcpy(buf + byte_cnt, data, len);
		byte_cnt += len;
		data += len;
		len -= len;
	}
	if (blen) {
		/* the padding is split up over two write blocks */
		flen = fs->write_block_size - byte_cnt;
		memset(buf + byte_cnt, 0xff, flen);
		rc = _nvs_flash_write(fs, offset, buf,
					  fs->write_block_size);
		if (rc) {
			/* flash write or verify error */
			goto err;
		}
		byte_cnt = 0;
		offset += fs->write_block_size;
		padding -= flen;
	}
	if (padding) {
		/* there is still some padding to be added */
		memset(buf + byte_cnt, 0xff, padding);
		byte_cnt += padding;
	}

	/* add id at the end */
	memcpy(buf + byte_cnt, (u8_t *)&id, sizeof(u16_t));
	byte_cnt += sizeof(u16_t);

	/* add ilenstr at the end */
	memcpy(buf+byte_cnt, ilenstr, length_cnt);
	byte_cnt += length_cnt;

	rc = _nvs_flash_write(fs, offset, buf, fs->write_block_size);
	if (rc) {
		/* flash write or verify error */
		goto err;
	}

	_nvs_addr_advance(fs, &fs->write_location, flash_len);

	return 0;

err:
	/* lock the filesystem */
	fs->locked = true;
	SYS_LOG_ERR("File system is locked");
	return rc;
}
/* read the length from flash at location offset (at start of item),
 * returns 0xffff in len if the read length fields are ff and ff
 */
int _nvs_flash_rd_len_start(struct nvs_fs *fs, off_t offset, size_t *len)
{
	int rc;
	u8_t lenstr[sizeof(u16_t)];
	size_t len_size;
	off_t addr;

	addr = offset;
	rc = _nvs_flash_read(fs, addr, lenstr, sizeof(u16_t));
	if (rc) {
		return rc;
	}
	len_size = _nvs_decode_length(lenstr, len);
	if (len_size == 0xffff) {
		*len = 0xffff;
	}
	return 0;
}

/* read the length and from flash just before location offset (at end of
 * item), returns 0xffff in len and id if the read flash fields are ff.
 */
int _nvs_flash_rd_len_end(struct nvs_fs *fs, off_t offset, size_t *len)
{
	int rc;
	u8_t lenstr[sizeof(u16_t)], ilenstr[sizeof(u16_t)];
	size_t len_size;
	off_t addr;

	addr = offset;
	_nvs_addr_decrease(fs, &addr, sizeof(u16_t));
	rc = _nvs_flash_read(fs, addr, ilenstr, sizeof(u16_t));
	if (rc) {
		return rc;
	}
	lenstr[0] = ilenstr[1];
	lenstr[1] = ilenstr[0];
	len_size = _nvs_decode_length(lenstr, len);
	if (len_size == 0xffff) {
		*len = 0xffff;
	}
	return 0;
}

/* read the length and id from flash just before location offset (at end of
 * item), returns 0xffff in len and id if the read flash fields are ff.
 */
int _nvs_flash_rd_len_id_end(struct nvs_fs *fs, off_t offset, u16_t *id,
			     size_t *len)
{
	int rc;
	off_t addr;

	addr = offset;
	rc = _nvs_flash_rd_len_end(fs, addr, len);
	if (rc) {
		return rc;
	}
	if (*len == 0xffff) {
		*id = NVS_ID_EMPTY;
		return 0;
	}
	if (*len < 128) {
		_nvs_addr_decrease(fs, &addr, sizeof(u8_t) + sizeof(u16_t));
	} else {
		_nvs_addr_decrease(fs, &addr, sizeof(u16_t) + sizeof(u16_t));
	}
	rc = _nvs_flash_read(fs, addr, id, sizeof(u16_t));
	return rc;
}

/* read the data from flash, the offset is the start of the entry
 * first read the length from flash as partial reading of the data can be
 * requested
 */
ssize_t _nvs_flash_read_data(struct nvs_fs *fs, off_t offset, void *data,
			     size_t len)
{
	int rc;
	u8_t lenstr[sizeof(u16_t)];
	size_t len_size;
	size_t len_flash;
	off_t addr;

	addr = offset;
	rc = _nvs_flash_read(fs, addr, lenstr, sizeof(u16_t));
	if (rc) {
		return rc;
	}
	len_size = _nvs_decode_length(lenstr, &len_flash);

	if (len_size > 2) {
		return -EINVAL;
	}
	_nvs_addr_advance(fs, &addr, len_size);
	rc = _nvs_flash_read(fs, addr, data, len);
	return rc;
}

/* end of advanced flash routines */

/* end of flash routines */

/* jump to previous item from offset updates offset, id, len. Does not update
 * when offset when empty ID is found.
 * return 0 when OK, errcode if error.
 */
int _nvs_entry_decrease(struct nvs_fs *fs, off_t *offset, u16_t *id,
			size_t *len)
{
	int rc;
	size_t flash_size;

	rc = _nvs_flash_rd_len_id_end(fs, *offset, id, len);
	if (rc) {
		return rc;
	}
	if (*id == NVS_ID_EMPTY) {
		return 0;
	}
	/* set the address to the start of the data */
	flash_size = get_wbs_aligned_entry_len(fs, *len);
	_nvs_addr_decrease(fs, offset, flash_size);
	return 0;
}

/* erase a sector by first checking it is used and then erasing if required
 * return 0 if OK, errorcode on error.
 */
int _nvs_sector_erase(struct nvs_fs *fs, off_t offset)
{
	int rc;

	offset &= ~(fs->sector_size - 1);
	rc = _nvs_flash_cmp_const(fs, offset, 0xff, fs->sector_size);
	if (rc < 0) {
		return rc;
	}
	if (rc) {
		rc = _nvs_flash_erase(fs, offset, fs->sector_size);
		if (rc) {
			return rc;
		}
	}
	return 0;
}

/* _nvs_sector_check does a check of the sector in flash.*end is pointing to
 * the start of the sector. _nvs_sector_check updates *end with the next write
 * location, calculates the range that is in use .
 * return 0 if OK, errorcode on error.
 */
int _nvs_sector_check(struct nvs_fs *fs, off_t *end, size_t *range)
{
	int rc;
	size_t len1, len2, entry_len, buf_size;
	off_t addr, start;

	start = *end;
	*range = 0;

	/* walk trough from begin to end update *end and *range */
	addr = start;
	while (1) {
		rc = _nvs_flash_rd_len_start(fs, addr, &len1);
		if (rc) {
			return rc;
		}
		if (len1 == 0xffff) {
			break;
		}
		entry_len = get_wbs_aligned_entry_len(fs, len1);
		/* avoid doing read over sector boundary */
		if (*range + entry_len > fs->sector_size) {
			break;
		}
		_nvs_addr_advance(fs, &addr, entry_len);
		rc = _nvs_flash_rd_len_end(fs, addr, &len2);
		if (len1 != len2) {
			break;
		}
		*end = addr;
		*range += entry_len;
		if (*range == fs->sector_size) {
			return 0;
		}
	}

	/* check from *end to sector end if everything is 0xff, decrement
	 * *range until it is set to the last location that is 0xff
	 */
	buf_size = max(fs->write_block_size, NVS_MIN_WRITE_BLOCK_SIZE);
	*range = fs->sector_size;
	addr = start + fs->sector_size;
	while (addr > *end) {
		addr -= buf_size;
		rc = _nvs_flash_cmp_const(fs, addr, 0xff, buf_size);
		if (rc < 0) {
			return rc;
		}
		if (rc) {
			break;
		}
		*range -= buf_size;
	}
	return 0;
}

/* garbage collection: the entry sector has been updated to point to the sector
 * just after the sector being gc'ed
 */
int _nvs_gc(struct nvs_fs *fs)
{
	int rc;
	off_t wlk_addr, wlk_last_addr, stop_addr;
	u16_t wlk_id, wlk_last_id;
	size_t wlk_len, wlk_last_len, jmp_len;


	wlk_addr = fs->entry_location;
	stop_addr = wlk_addr;
	_nvs_addr_decrease(fs, &stop_addr, fs->sector_size);

	/* Go backwards through the gc sector until we reach the start of
	 * the sector
	 */
	while (wlk_addr != stop_addr) {

		rc = _nvs_entry_decrease(fs, &wlk_addr, &wlk_id, &wlk_len);
		if (rc) {
			goto err;
		}
		if (wlk_id != NVS_ID_JUMP) {
			wlk_last_addr = fs->write_location;
			while (wlk_last_addr != wlk_addr) {
				rc = _nvs_entry_decrease(fs, &wlk_last_addr,
						&wlk_last_id, &wlk_last_len);
				if (wlk_last_id == wlk_id) {
					break;
				}
			}
			if ((wlk_last_addr == wlk_addr) && (wlk_len)) {
				SYS_LOG_DBG("Moving entry: id %d", wlk_id);
				jmp_len = get_wbs_aligned_entry_len(fs,
								    wlk_len);
				rc = _nvs_flash_block_move(fs, wlk_addr,
							   jmp_len);
				if (rc) {
					goto err;
				}
			}
		}
	}
	return 0;
err:
	/* lock the filesystem */
	fs->locked = true;
	SYS_LOG_ERR("File system is locked");
	return rc;

}

int nvs_clear(struct nvs_fs *fs)
{
	int rc, i;
	off_t addr;

	for (i = 0; i < fs->sector_count; i++) {
		addr = i * fs->sector_size;
		rc = _nvs_sector_erase(fs, addr);
		if (rc) {
			return rc;
		}
	}
	return 0;
}

int nvs_reinit(struct nvs_fs *fs)
{
	int rc, i = 0;
	bool end_found;
	size_t sector_range, len;
	off_t addr, stop_addr;
	u16_t id;
	u8_t zeros[fs->write_block_size];


	k_mutex_lock(&fs->nvs_lock, K_FOREVER);

	end_found = false;

	/* step through the sectors to find the last sector */
	while (i < fs->sector_count) {
		addr = i * fs->sector_size;
		rc = _nvs_sector_check(fs, &addr, &sector_range);
		if (rc) {
			goto end;
		}
		if ((sector_range) && (addr < ((i+1) * fs->sector_size))) {
			fs->write_location = addr;
			end_found = true;
			break;
		}
		i++;
	}

	/* step back from fs->write_location to find start */
	if (end_found) {
		stop_addr = fs->write_location & ~(fs->sector_size - 1);
		_nvs_addr_advance(fs, &stop_addr, fs->sector_size);
		while (addr != stop_addr) {
			rc = _nvs_entry_decrease(fs, &addr, &id, &len);
			if (rc) {
				goto end;
			}
			if (id == NVS_ID_EMPTY) {
				break;
			}
		}
		fs->entry_location = addr;
	}

	/* if there is no started sector or reninit of a locked fs is requested
	 * clear the fs and reinitialize it.
	 */
	if ((!end_found) || (fs->locked)) {
		/* only empty sectors or reinit of locked fs -> clear the fs */
		SYS_LOG_DBG("(Re)Initializing sectors");
		rc = nvs_clear(fs);
		if (rc) {
			goto end;
		}
		fs->write_location = 0;
		fs->entry_location = 0;
		fs->locked = false;

		rc = 0;
		goto end;
	}

	/* In case of a difference between the range and fs->write_location
	 * the errors are corrected by writing jumps of length zero at
	 * fs->write_location until the error is corrected. All data is
	 * set to zero.
	 */
	addr = (fs->write_location &  ~(fs->sector_size - 1));
	_nvs_addr_advance(fs, &addr, sector_range);
	if (addr != fs->write_location) {
		SYS_LOG_DBG("Correcting fs error");
		len = fs->write_block_size - sizeof(u16_t) - 2*sizeof(u8_t);
		memset(zeros, 0, sizeof(zeros));
		while (fs->write_location != addr) {
			_nvs_flash_write(fs, fs->write_location, zeros,
					 fs->write_block_size);
			_nvs_addr_advance(fs, &fs->write_location,
					  fs->write_block_size);
		}
	}

	/* one sector should always be empty, unless power was cut during
	 * then the next sector after fs->write_location will be
	 * fs->entry_location. Check if this is the case and restart garbage
	 * collection by clearing the fs->write_location sector and restart gc
	 * on the entry sector.
	 */
	addr = fs->write_location & ~(fs->sector_size - 1);
	_nvs_addr_advance(fs, &addr, fs->sector_size);
	if (addr == fs->entry_location) {
		SYS_LOG_DBG("Erasing last added sector and restarting gc");
		addr = fs->write_location;
		/* erase the last added sector */
		rc = _nvs_sector_erase(fs, addr);
		if (rc) {
			goto end;
		}
		_nvs_addr_advance(fs, &fs->entry_location, fs->sector_size);
		rc = _nvs_gc(fs);
		if (rc) {
			goto end;
		}
		_nvs_addr_advance(fs, &addr, fs->sector_size);
		rc = _nvs_sector_erase(fs, addr);
		goto end;
	}

	rc = 0;

end:
	k_mutex_unlock(&fs->nvs_lock);
	if (rc) {
		fs->locked = true;
		SYS_LOG_ERR("File system is locked");
	}
	return rc;
}

int nvs_init(struct nvs_fs *fs, const char *dev_name)
{

	int rc;
	u16_t max_item_len;

	k_mutex_init(&fs->nvs_lock);

	fs->flash_device = device_get_binding(dev_name);
	if (!fs->flash_device) {
		SYS_LOG_ERR("No valid flash device found");
		return -ENXIO;
	}

	fs->write_block_size = flash_get_write_block_size(fs->flash_device);
	if (fs->write_block_size < NVS_MIN_WRITE_BLOCK_SIZE) {
		fs->write_block_size = NVS_MIN_WRITE_BLOCK_SIZE;
	}

	/* check the sector size, should be power of 2 */
	if (!((fs->sector_size != 0) &&
	     !(fs->sector_size & (fs->sector_size - 1)))) {
		SYS_LOG_ERR("Configuration error - sector size");
		return -EINVAL;
	}
	/* check the number of sectors, it should be at least 2 */
	if (fs->sector_count < 2) {
		SYS_LOG_ERR("Configuration error - sector count");
		return -EINVAL;
	}
	/* check the maximum item size, it should be at least smaller than the
	 * sector_size - sector close (min length = 0)
	 */
	max_item_len = fs->sector_size - get_wbs_aligned_entry_len(fs, 0);

	if (fs->max_len > max_item_len) {
		SYS_LOG_ERR("Configuration error - max item size");
		return -EINVAL;
	}

	fs->entry_location = 0;
	fs->locked = false;
	rc = nvs_reinit(fs);
	if (rc) {
		return rc;
	}

	SYS_LOG_INF("maximum storage length %d byte", fs->max_len);
	SYS_LOG_INF("write-align: %d, write-addr: %"PRIx32"",
		fs->write_block_size, fs->write_location);

	return 0;
}

int nvs_append(struct nvs_fs *fs, u16_t id, size_t len)
{
	int rc;
	size_t ext_len, free_len;

	rc = 0;

	/* the free space should be big enough to always leave a sector
	 * end, which is of length 0.
	 */
	ext_len = get_wbs_aligned_entry_len(fs, len) +
		  get_wbs_aligned_entry_len(fs, 0);

	free_len = fs->sector_size -
		   (fs->write_location & (fs->sector_size - 1));

	if (free_len < ext_len) {
		rc = NVS_STATUS_NOSPACE;
		goto err;
	}
	return 0;

err:
	return rc;
}

/* rotate the nvs, frees the next sector (based on fs->write_location) */
int nvs_rotate(struct nvs_fs *fs)
{
	int rc;
	off_t offset;
	size_t len;
	u8_t buf[fs->write_block_size];

	rc = 0;

	len = fs->sector_size - (fs->write_location & (fs->sector_size - 1));
	/* fill previous sector with data to jump to the next sector
	 */
	memset(buf, 0xff, fs->write_block_size);
	while (len) {
		_nvs_flash_wrt_entry(fs, NVS_ID_JUMP, buf, 0);
		len -= fs->write_block_size;
	}

	/* fs->write_location is allready pointing to new sector for writing */
	offset = (fs->write_location & ~(fs->sector_size - 1));

	/* data copy is required if the sector after the new sector is the
	 * entry sector.
	 */
	_nvs_addr_advance(fs, &offset, fs->sector_size);
	if (offset == fs->entry_location) {
		SYS_LOG_DBG("Garbage collection...");
		_nvs_addr_advance(fs, &fs->entry_location, fs->sector_size);
		rc = _nvs_gc(fs);
		if (rc) {
			goto err;
		}
		rc = _nvs_sector_erase(fs, offset);
		if (rc) {
			goto err;
		}
	}
	return 0;

err:
	/* lock the file system */
	fs->locked = true;
	SYS_LOG_ERR("File system is locked");
	return rc;
}

ssize_t nvs_write(struct nvs_fs *fs, u16_t id, const void *data, size_t len)
{
	int rc;
	int rot_cnt = 0;
#if IS_ENABLED(CONFIG_NVS_PROTECT_FLASH)
	u16_t wlk_id;
	size_t wlk_len;
	off_t wlk_addr, stop_addr;
#endif

	if ((id == NVS_ID_EMPTY) || (id == NVS_ID_JUMP) ||
	    (len > fs->max_len) || ((len > 0) && (data == NULL))) {
		return -EINVAL;
	}
	if (fs->locked) {
		return -EROFS;
	}

#if IS_ENABLED(CONFIG_NVS_PROTECT_FLASH)
	wlk_addr = fs->write_location;
	stop_addr = fs->entry_location;
	while (wlk_addr != stop_addr) {
		rc = _nvs_entry_decrease(fs, &wlk_addr, &wlk_id, &wlk_len);
		if (rc) {
			goto err;
		}
		if (wlk_id != NVS_ID_JUMP) {
			if (wlk_id == id) {
				if (wlk_len == len) {
					rc = _nvs_flash_block_cmp(fs, wlk_addr,
								  data, len);
					if (!rc) {
						return len;
					}
				}
				break;
			}
		}
	}
#endif
	k_mutex_lock(&fs->nvs_lock, K_FOREVER);
	while (1) {
		rc = nvs_append(fs, id, len);
		if (rc) {
			if (rc == NVS_STATUS_NOSPACE) {
				if (rot_cnt == fs->sector_count) {
					fs->locked = true;
					rc = -EROFS;
					goto err;
				}
				if (nvs_rotate(fs)) {
					goto err;
				}
				rot_cnt++;
			} else {
				rc = -ENOSPC;
				goto err;
			}
		} else {
			break;
		}
	}

	_nvs_flash_wrt_entry(fs, id, data, len);
	if (rc) {
		goto err;
	}

	return len;
err:
	k_mutex_unlock(&fs->nvs_lock);
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
	off_t wlk_addr, stop_addr;
	size_t wlk_len;
	u16_t wlk_id, cnt_his;

	if ((id == NVS_ID_EMPTY) || (id == NVS_ID_JUMP) ||
	    (len > fs->max_len)) {
		return -EINVAL;
	}
	cnt_his = 0;

	wlk_addr = fs->write_location;
	stop_addr = fs->entry_location;
	while ((wlk_addr != stop_addr) && (cnt_his <= cnt)) {
		rc = _nvs_entry_decrease(fs, &wlk_addr, &wlk_id, &wlk_len);
		if (rc) {
			goto err;
		}
		if (wlk_id != NVS_ID_JUMP) {
			if (wlk_id == id) {
				cnt_his++;
			}
		}
	}
	if (((wlk_addr == stop_addr) && (wlk_id != id))
	    || (wlk_len == 0) || (cnt_his < cnt)) {
		return -ENOENT;
	}
	rc = _nvs_flash_read_data(fs, wlk_addr, data, len);
	if (rc) {
		goto err;
	}
	return wlk_len;

err:
	return rc;
}

ssize_t nvs_read(struct nvs_fs *fs, u16_t id, void *data, size_t len)
{
	int rc;

	rc = nvs_read_hist(fs, id, data, len, 0);
	return rc;
}
