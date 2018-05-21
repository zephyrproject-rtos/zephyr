/*  NVS: non volatile storage in flash
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <flash.h>
#include <crc16.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <nvs/nvs.h>
#include "nvs_priv.h"


static inline u16_t _nvs_len_in_flash(struct nvs_fs *fs, u16_t len)
{
	if (fs->write_block_size <= 1) {
		return len;
	}
	return (len + (fs->write_block_size - 1)) & ~(fs->write_block_size - 1);
}

u16_t _nvs_entry_len_in_flash(struct nvs_fs *fs, u16_t len)
{
	return _nvs_len_in_flash(fs, len) +
	       _nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr)) +
	       _nvs_len_in_flash(fs, sizeof(struct _nvs_data_slt));
}


off_t _nvs_head_addr_in_flash(struct nvs_fs *fs, const struct nvs_entry *entry)
{
	return entry->data_addr -
	       _nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr));
}

off_t _nvs_slt_addr_in_flash(struct nvs_fs *fs, const struct nvs_entry *entry)
{
	return entry->data_addr + _nvs_len_in_flash(fs, entry->len);
}

int _nvs_bd_check(struct nvs_fs *fs, off_t offset, size_t len)
{
	if ((offset > fs->sector_size * fs->sector_count) ||
	    (offset + len > fs->sector_size * fs->sector_count)) {
		/* operation outside nvs area */
		return -EINVAL;
	}
	if ((offset & ~(fs->sector_size - 1)) !=
	    ((offset + len - 1) & ~(fs->sector_size - 1))) {
		/* operation over sector boundary */
		return -EINVAL;
	}
	return 0;
}

void _nvs_addr_advance(struct nvs_fs *fs, off_t *addr, u16_t step)
{
	*addr += step;
	if (*addr >= (fs->sector_count * fs->sector_size)) {
		*addr -= (fs->sector_count * fs->sector_size);
	}
}

/* Getting the sector header for address given in offset */
int _nvs_sector_hdr_get(struct nvs_fs *fs, off_t offset,
			struct _nvs_sector_hdr *sector_hdr)
{
	return flash_read(fs->flash_device,
			  fs->offset + (offset & ~(fs->sector_size - 1)),
			  sector_hdr,
			  sizeof(*sector_hdr));
}

/* Initializing sector by writing magic and status to sector header,
 * set fs->write_address just after header offset should be pointing
 * to the sector
 */
int _nvs_sector_init(struct nvs_fs *fs, off_t offset)
{
	struct _nvs_sector_hdr sector_hdr;
	u16_t sector_hdr_len;
	int rc;

	/* just to be sure, align offset to sector_size */
	offset &= ~(fs->sector_size - 1);
	fs->sector_id += 1;
	rc = _nvs_sector_hdr_get(fs, offset, &sector_hdr);
	if (rc) {
		SYS_LOG_DBG("Failed reading sector hdr");
		return rc;
	}
	if (sector_hdr.fd_magic != 0xFFFFFFFF) {
		SYS_LOG_DBG("Initializing non empty sector");
		return -EIO;
	}
	sector_hdr.fd_magic = fs->magic;
	sector_hdr.fd_id = fs->sector_id;
	sector_hdr_len = _nvs_len_in_flash(fs, sizeof(struct _nvs_sector_hdr));
	rc = nvs_flash_write(fs, offset, &sector_hdr, sizeof(sector_hdr));
	if (rc) {
		return rc;
	}
	fs->write_location = offset + sector_hdr_len;
	return 0;
}

/* check if a sector is used by looking at all elements of the sector.
 * return 0 if sector is empty, 1 if it is not empty
 */
int _nvs_sector_is_used(struct nvs_fs *fs, off_t offset)
{
	int rc;
	off_t addr;
	u8_t buf;

	rc = 0;
	offset &= ~(fs->sector_size - 1);
	for (addr = 0; addr < fs->sector_size; addr += sizeof(buf)) {
		rc = flash_read(fs->flash_device,
				fs->offset + offset + addr,
				&buf, sizeof(buf));
		if (rc) {
			return rc;
		}
		if (buf != 0xFF) {
			return 1;
		}
	}
	return 0;
}

/* erase the flash starting at offset, erase length = len */
int _nvs_flash_erase(struct nvs_fs *fs, off_t offset, size_t len)
{
	int rc;

	rc = flash_write_protection_set(fs->flash_device, 0);
	if (rc) {
		/* flash protection set error */
		return rc;
	}

	rc = flash_erase(fs->flash_device, fs->offset + offset, len);
	if (rc) {
		/* flash write error */
		return rc;
	}

	SYS_LOG_DBG("Erasing flash at %"PRIx32", len %x", offset, len);
	(void) flash_write_protection_set(fs->flash_device, 1);
	return 0;
}

void _nvs_entry_sector_advance(struct nvs_fs *fs)
{
	fs->entry_sector++;
	if (fs->entry_sector == fs->sector_count) {
		fs->entry_sector = 0;
	}
}

/* garbage collection: addr is set to the start of the sector to be gc'ed,
 * the entry sector has been updated to point to the sector just after the
 * sector being gc'ed
 */
int _nvs_gc(struct nvs_fs *fs, off_t addr)
{
	int rc, len, bytes_to_copy;
	off_t rd_addr;
	struct nvs_entry walker, walker_last, last_entry, search;
	struct _nvs_data_hdr head;
	u16_t sec_hdr_len, hdr_len, walker_len, walker_last_len;
	u8_t buf[NVS_MOVE_BLOCK_SIZE];

	walker.data_addr = addr;
	sec_hdr_len = _nvs_len_in_flash(fs, sizeof(struct _nvs_sector_hdr));
	hdr_len = _nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr));
	_nvs_addr_advance(fs, &walker.data_addr, sec_hdr_len + hdr_len);
	while (1) {
		rd_addr = _nvs_head_addr_in_flash(fs, &walker);
		rc = nvs_flash_read(fs, rd_addr, &head, sizeof(head));
		if (rc) {
			return rc;
		}
		if ((head.id == NVS_ID_SECTOR_END) ||
		    (head.id == NVS_ID_EMPTY)) {
			return 0;
		}
		walker.len = head.len;
		walker.id = head.id;
		search.id = walker.id;
		if (nvs_get_first_entry(fs, &search)) {
			/* entry is not found, copy needed - but find the last
			 * entry first
			 */
			last_entry.len = 0;
			last_entry.data_addr = 0;
			walker_last = walker;
			while (walker_last.id != NVS_ID_SECTOR_END) {
				rd_addr = _nvs_head_addr_in_flash(
						fs, &walker_last);
				rc = nvs_flash_read(
					fs, rd_addr, &head, sizeof(head));
				if (rc) {
					return rc;
				}
				walker_last.len = head.len;
				walker_last.id = head.id;
				if (walker_last.id == walker.id) {
					last_entry = walker_last;
				}
				walker_last_len = _nvs_entry_len_in_flash(
							fs, walker_last.len);
				_nvs_addr_advance(
					fs, &walker_last.data_addr,
					walker_last_len);
			}
			if (last_entry.len == 0) {
				SYS_LOG_DBG("Skipped move of removed entry id"
					"%x", search.id);
			} else {
				rd_addr = _nvs_head_addr_in_flash(
						fs, &last_entry);
				len = _nvs_entry_len_in_flash(
					fs, last_entry.len);
				while (len > 0) {
					bytes_to_copy =
						min(NVS_MOVE_BLOCK_SIZE, len);
					rc = nvs_flash_read(fs, rd_addr,
						&buf, bytes_to_copy);
					if (rc) {
						return rc;
					}
					rc = nvs_flash_write(fs,
						fs->write_location,
						&buf, bytes_to_copy);
					if (rc) {
						return rc;
					}
					len -= bytes_to_copy;
					rd_addr += bytes_to_copy;
					fs->write_location += bytes_to_copy;
				}
				SYS_LOG_DBG("Entry with id %x moved to new "
					    "flash sector", search.id);
			}
		}
		walker_len = _nvs_entry_len_in_flash(fs, walker.len);
		_nvs_addr_advance(fs, &walker.data_addr, walker_len);
	}
}

void nvs_set_start_entry(struct nvs_fs *fs, struct nvs_entry *entry)
{
	u16_t sector_hdr_len, data_hdr_len;

	entry->data_addr = fs->entry_sector * fs->sector_size;
	sector_hdr_len = _nvs_len_in_flash(fs, sizeof(struct _nvs_sector_hdr));
	data_hdr_len = _nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr));
	_nvs_addr_advance(fs, &entry->data_addr, sector_hdr_len + data_hdr_len);
}

int nvs_get_first_entry(struct nvs_fs *fs, struct nvs_entry *entry)
{
	int rc;
	struct _nvs_data_hdr head;
	off_t hdr_addr;
	u16_t adv_len;

	nvs_set_start_entry(fs, entry);
	while (1) {
		hdr_addr = _nvs_head_addr_in_flash(fs, entry);
		rc = nvs_flash_read(fs, hdr_addr, &head, sizeof(head));
		if (rc) {
			return rc;
		}
		if (head.id == NVS_ID_EMPTY) {
			return -ENOENT;
		}
		if (head.id == entry->id) {
			entry->len = head.len;
			return 0;
		}
		adv_len = _nvs_entry_len_in_flash(fs, head.len);
		_nvs_addr_advance(fs, &entry->data_addr, adv_len);
	}
}


int nvs_get_last_entry(struct nvs_fs *fs, struct nvs_entry *entry)
{
	int rc;
	struct nvs_entry latest;
	struct _nvs_data_hdr head;
	off_t hdr_addr;
	u16_t adv_len;

	rc = nvs_get_first_entry(fs, entry);
	if (rc) {
		return rc;
	}
	latest.id = entry->id;
	latest.data_addr = entry->data_addr;
	latest.len = entry->len;
	while (1) {
		hdr_addr = _nvs_head_addr_in_flash(fs, entry);
		rc = nvs_flash_read(fs, hdr_addr, &head, sizeof(head));
		if (rc) {
			return rc;
		}
		if (head.id == NVS_ID_EMPTY) {
			entry->id = latest.id;
			entry->data_addr = latest.data_addr;
			entry->len = latest.len;
			return 0;
		}
		if (head.id == latest.id) {
			latest.len = head.len;
			latest.data_addr = entry->data_addr;
		}
		adv_len = _nvs_entry_len_in_flash(fs, head.len);
		_nvs_addr_advance(fs, &entry->data_addr, adv_len);
	}
}

/* walking over entries, stops on empty or entry with same entry id */
int nvs_walk_entry(struct nvs_fs *fs, struct nvs_entry *entry)
{
	int rc;
	struct _nvs_data_hdr head;
	off_t hdr_addr;
	u16_t adv_len;


	if (entry->id != NVS_ID_EMPTY) {
		adv_len = _nvs_entry_len_in_flash(fs, entry->len);
		_nvs_addr_advance(fs, &entry->data_addr, adv_len);
	}
	while (1) {
		hdr_addr = _nvs_head_addr_in_flash(fs, entry);
		rc = nvs_flash_read(fs, hdr_addr, &head, sizeof(head));
		if (rc) {
			return rc;
		}
		if (head.id == entry->id) {
			entry->len = head.len;
			return 0;
		}
		if (head.id == NVS_ID_EMPTY) {
			return -ENOENT;
		}
		adv_len = _nvs_entry_len_in_flash(fs, head.len);
		_nvs_addr_advance(fs, &entry->data_addr, adv_len);
	}
}

int nvs_init(struct nvs_fs *fs, const char *dev_name, u32_t magic)
{

	int i, rc, active_sector_cnt = 0;
	int entry_sector = -1;
	u16_t entry_sector_id, active_sector_id, max_item_len;
	struct _nvs_sector_hdr sector_hdr;
	struct nvs_entry entry;
	off_t addr;

	fs->magic = magic;
	fs->sector_id = 0;
	fs->free_space = fs->max_len;
	fs->flash_device = device_get_binding(dev_name);
	if (!fs->flash_device) {
		SYS_LOG_ERR("No valid flash device found");
		return -ENXIO;
	}
	fs->write_block_size = flash_get_write_block_size(fs->flash_device);

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
	 * sector_size - sector_hdr size - item_hdr_size - item_slt_size
	 */
	max_item_len = fs->sector_size
		       - _nvs_len_in_flash(fs, sizeof(struct _nvs_sector_hdr))
		       - _nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr))
		       - _nvs_len_in_flash(fs, sizeof(struct _nvs_data_slt));

	if (fs->max_len > max_item_len) {
		SYS_LOG_ERR("Configuration error - max item size");
		return -EINVAL;
	}
	for (i = 0; i < fs->sector_count; i++) {
		rc = _nvs_sector_hdr_get(fs, i * fs->sector_size, &sector_hdr);
		if (rc) {
			return rc;
		}
		if (sector_hdr.fd_magic != fs->magic) {
			continue;
		}
		active_sector_cnt++;
		if (entry_sector < 0) {
			entry_sector = i;
			entry_sector_id = sector_hdr.fd_id;
			active_sector_id = sector_hdr.fd_id;
			continue;
		}
		if (NVS_ID_GT(sector_hdr.fd_id, active_sector_id)) {
			active_sector_id = sector_hdr.fd_id;
		}
		if (NVS_ID_GT(entry_sector_id, sector_hdr.fd_id)) {
			entry_sector = i;
			entry_sector_id = sector_hdr.fd_id;
		}
	}
	if (entry_sector < 0) { /* No valid sectors found */
		SYS_LOG_DBG("No valid sectors found, initializing sectors");
		for (addr = 0; addr < (fs->sector_count * fs->sector_size);
			addr += fs->sector_size) {
			/* first check if used, only erase if it is */
			if (_nvs_sector_is_used(fs, addr)) {
				rc = _nvs_flash_erase(fs, addr,
						      fs->sector_size);
				if (rc) {
					return rc;
				}
			}
		}
		rc = _nvs_sector_init(fs, 0);
		if (rc) {
			return rc;
		}
		entry_sector = 0;
		active_sector_id = fs->sector_id;
	}
	fs->entry_sector = entry_sector;
	fs->sector_id = active_sector_id;

	/* Find the first empty entry */
	nvs_set_start_entry(fs, &entry);
	entry.id = NVS_ID_EMPTY;
	rc = nvs_walk_entry(fs, &entry);
	if (rc) {
		return rc;
	}
	fs->write_location = _nvs_head_addr_in_flash(fs, &entry);
	if (active_sector_cnt == fs->sector_count) {
		/* one sector should always be empty, unless power was cut
		 * during garbage collection start gc again on the last
		 * sector.
		 */
		SYS_LOG_DBG("Restarting garbage collection");
		addr = fs->entry_sector * fs->sector_size;
		_nvs_entry_sector_advance(fs);
		rc = _nvs_gc(fs, addr);
		if (rc) {
			return rc;
		}
		rc = _nvs_flash_erase(fs, addr, fs->sector_size);
		if (rc) {
			return rc;
		}
	}

	SYS_LOG_INF("maximum storage length %d bytes", fs->max_len);
	SYS_LOG_INF("write-align: %d, write-addr: %"PRIx32"",
		fs->write_block_size, fs->write_location);
	SYS_LOG_INF("entry sector: %d, entry sector ID: %d",
		fs->entry_sector, fs->sector_id);

	k_mutex_init(&fs->nvs_lock);

	return 0;
}

int nvs_append(struct nvs_fs *fs, struct nvs_entry *entry)
{
	int rc;
	u16_t required_len, extended_len, hdr_len, slt_len;
	struct _nvs_data_hdr data_hdr;

	rc = 0;

	k_mutex_lock(&fs->nvs_lock, K_FOREVER);
	required_len = _nvs_entry_len_in_flash(fs, entry->len);

	/* the space available should be big enough to fit the data + header
	 * and slot of next data
	 */
	hdr_len = _nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr));
	slt_len = _nvs_len_in_flash(fs, sizeof(struct _nvs_data_slt));
	extended_len = required_len + hdr_len + slt_len;

	if ((fs->sector_size - (fs->write_location & (fs->sector_size - 1))) <
		extended_len) {
		rc = NVS_STATUS_NOSPACE;
		goto err;
	}
	data_hdr.id = entry->id;
	data_hdr.len = _nvs_len_in_flash(fs, entry->len);

	rc = nvs_flash_write(fs, fs->write_location, &data_hdr, sizeof(data_hdr));
	if (rc) {
		goto err;
	}

	entry->data_addr = fs->write_location + hdr_len;
	fs->write_location += required_len;
	rc = 0;

err:
	k_mutex_unlock(&fs->nvs_lock);
	return rc;
}

/* close the append by applying a seal to the data that includes a crc */
int nvs_append_close(struct nvs_fs *fs, const struct nvs_entry *entry)
{
	int rc;
	struct _nvs_data_slt data_slt;
	off_t addr;

	k_mutex_lock(&fs->nvs_lock, K_FOREVER);
	rc = nvs_compute_crc(fs, entry, &data_slt.crc16);
	if (rc) {
		goto err;
	}
	addr = _nvs_slt_addr_in_flash(fs, entry);
	rc = nvs_flash_write(fs, addr, &data_slt, sizeof(data_slt));

err:
	k_mutex_unlock(&fs->nvs_lock);
	return rc;
}

/* check the crc of an entry */
int nvs_check_crc(struct nvs_fs *fs, struct nvs_entry *entry)
{
	int rc;
	struct _nvs_data_slt data_slt;
	off_t addr;
	u16_t crc16;

	rc = nvs_compute_crc(fs, entry, &crc16);
	if (rc) {
		return -EIO;
	}
	addr = _nvs_slt_addr_in_flash(fs, entry);
	rc = nvs_flash_read(fs, addr, &data_slt, sizeof(data_slt));
	if (rc || (crc16 != data_slt.crc16)) {
		return -EIO;
	}
	return 0;
}



/* rotate the nvs, frees the next sector (based on fs->write_location) */
int nvs_rotate(struct nvs_fs *fs)
{
	int rc;
	off_t addr;
	struct _nvs_data_hdr head;
	u16_t hdr_len, slt_len, sec_hdr_len;

	rc = 0;

	k_mutex_lock(&fs->nvs_lock, K_FOREVER);

	/* fill previous sector with data to jump to the next sector */
	head.id = NVS_ID_SECTOR_END;
	hdr_len = _nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr));
	slt_len = _nvs_len_in_flash(fs, sizeof(struct _nvs_data_slt));
	sec_hdr_len = _nvs_len_in_flash(fs, sizeof(struct _nvs_sector_hdr));
	head.len = fs->sector_size
		   - (fs->write_location & (fs->sector_size - 1))
		   - slt_len - hdr_len + sec_hdr_len;
	rc = nvs_flash_write(fs, fs->write_location, &head, sizeof(head));
	if (rc) {
		goto out;
	}

	/* advance to next sector for writing */
	addr = (fs->write_location & ~(fs->sector_size - 1));
	_nvs_addr_advance(fs, &addr, fs->sector_size);

	/* initialize the new sector */
	rc = _nvs_sector_init(fs, addr);
	if (rc) {
		goto out;
	}

	/* data copy */
	addr = (fs->write_location & ~(fs->sector_size - 1));
	_nvs_addr_advance(fs, &addr, fs->sector_size);
	/* Do we need to advance the entry_sector, if so we need to copy */
	if ((addr & ~(fs->sector_size - 1)) ==
		fs->entry_sector * fs->sector_size) {
		SYS_LOG_DBG("Starting data copy...");
		_nvs_entry_sector_advance(fs);
		rc = _nvs_gc(fs, addr);
		if (rc) {
			SYS_LOG_DBG("Quit data copy - gc error");
			goto out;
		}
		rc = _nvs_flash_erase(fs, addr, fs->sector_size);
		if (rc) {
			SYS_LOG_DBG("Quit data copy - flash erase error");
			goto out;
		}
		SYS_LOG_DBG("Done data copy - no error");
	}
	rc = 0;

out:
	k_mutex_unlock(&fs->nvs_lock);
	return rc;
}

int nvs_compute_crc(struct nvs_fs *fs, const struct nvs_entry *entry,
		    u16_t *crc16)
{
	int rc;

	*crc16 = 0xFFFF;
	for (int i = 0; i < entry->len; i += fs->write_block_size) {
		u8_t buf[fs->write_block_size];

		rc = nvs_flash_read(fs, entry->data_addr + i, buf, sizeof(buf));
		if (rc) {
			return -EIO;
		}
		*crc16 = crc16_ccitt(*crc16, buf, sizeof(buf));
	}
	return 0;
}

int nvs_clear(struct nvs_fs *fs)
{
	int rc;
	off_t addr;

	k_mutex_lock(&fs->nvs_lock, K_FOREVER);

	for (addr = 0; addr < fs->sector_count * fs->sector_size;
		addr += fs->sector_size) {
		rc = _nvs_flash_erase(fs, addr, fs->sector_size);
		if (rc) {
			goto out;
		}
	}
	rc = 0;

out:
	k_mutex_unlock(&fs->nvs_lock);
	return rc;
}

int nvs_flash_read(struct nvs_fs *fs, off_t offset, void *data, size_t len)
{
	int rc;

	rc = _nvs_bd_check(fs, offset, len);
	if (rc) {
		return rc;
	}
	rc = flash_read(fs->flash_device, fs->offset + offset, data, len);
	if (rc) {
		/* flash read error */
		return rc;
	}
	return 0;
}

int nvs_flash_write(struct nvs_fs *fs, off_t offset, const void *data,
	size_t len)
{
	int rc;

	rc = _nvs_bd_check(fs, offset, len);
	if (rc) {
		return rc;
	}
	rc = flash_write_protection_set(fs->flash_device, 0);
	if (rc) {
		/* flash protection set error */
		return rc;
	}
	/* write and entire number of blocks */
	if (len >= fs->write_block_size) {
		size_t blen = len & ~(fs->write_block_size - 1);
		rc = flash_write(fs->flash_device, fs->offset + offset, data, blen);
		if (rc) {
			/* flash write error */
			return rc;
		}
		len -= blen;
		offset += len;
		data += len;
	}
	/* write the remaining data padding up to write_blocks_zize with 0xff */
	if (len) {
		u8_t buf[fs->write_block_size];
		memcpy(buf, data, len);
		memset(buf + len, 0xff, fs->write_block_size - len);
		rc = flash_write(fs->flash_device, fs->offset + offset, buf, fs->write_block_size);
		if (rc) {
			/* flash write error */
			return rc;
		}
	}
	(void) flash_write_protection_set(fs->flash_device, 1);
	/* don't mind about this error */
	return 0;
}


ssize_t nvs_write(struct nvs_fs *fs, u16_t id, const void *data, size_t len)
{
	int rc, entry_cmp;
	int rot_cnt = 0;
	u16_t free_up, entry_len_fl, hdr_len, slt_len;
	struct nvs_entry entry, stored_entry;
	off_t stored_entry_addr;


	if ((id == NVS_ID_EMPTY) ||
	    (id == NVS_ID_SECTOR_END) ||
	    (len > fs->max_len) ||
	    ((len > 0) && (data == NULL))) {
		return -EINVAL;
	}
	if ((len > 0) && (fs->free_space < fs->max_len)) {
		return -ENOSPC;
	}
	if ((!len) || IS_ENABLED(CONFIG_NVS_PROTECT_FLASH)) {
		/* find item to delete or check rewriting same data  */
		stored_entry.id = id;
		rc = nvs_get_last_entry(fs, &stored_entry);
		if (!rc) {
			entry_len_fl = _nvs_len_in_flash(fs, len);
			if (stored_entry.len == entry_len_fl) {
				stored_entry_addr = stored_entry.data_addr
						    + fs->offset;
				entry_cmp = memcmp(data,
						   (void *) stored_entry_addr,
						   len);
				if (!entry_cmp) {
					return 0;
				}
			}
		} else {
			return rc;
		}
	}
	entry.id = id;
	entry.len = len;
	/* new data or data has changed */
	while (1) {
		rc = nvs_append(fs, &entry);
		if (rc) {
			if (rc == NVS_STATUS_NOSPACE) {
				if (fs->free_space == 0) {
					/* if we get here when there is no
					 * free_space available this means
					 * even deletes fails, we just
					 * give up and say the file system
					 * is full
					 */
					rc = -ENOSPC;
					goto err;
				}
				if (rot_cnt == fs->sector_count) {
					fs->free_space = 0;
					rc = -ENOSPC;
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
	rc = nvs_flash_write(fs, entry.data_addr, data, entry.len);
	if (rc) {
		goto err;
	}
	rc = nvs_append_close(fs, &entry);
	if (rc) {
		goto err;
	}
	if ((!entry.len) && (fs->free_space < fs->max_len)) {
		/* freeing up space by deleting */
		hdr_len = _nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr));
		slt_len = _nvs_len_in_flash(fs, sizeof(struct _nvs_data_slt));
		free_up = stored_entry.len + hdr_len + slt_len;
		fs->free_space = min(fs->max_len, fs->free_space + free_up);
	}
	rc = entry.len;
err:
	return rc;
}

int nvs_delete(struct nvs_fs *fs, u16_t id)
{
	return nvs_write(fs, id, NULL, 0);
}

ssize_t nvs_read(struct nvs_fs *fs, u16_t id, void *data, size_t len)
{
	int rc;
	struct nvs_entry entry;

	if ((id == NVS_ID_EMPTY) || (id == NVS_ID_SECTOR_END)) {
		return -EINVAL;
	}
	entry.id = id;
	/* Read last entry */
	rc = nvs_get_last_entry(fs, &entry);
	if (entry.len == 0) {
		return -ENOENT;
	}
	if (rc) {
		goto err;
	}
	rc = nvs_check_crc(fs, &entry);
	if (rc) {
		goto err;
	}
	rc = nvs_flash_read(fs, entry.data_addr, data,
			    min(entry.len, len));
	if (rc) {
		goto err;
	}
	rc = entry.len;

err:
	return rc;
}

ssize_t nvs_read_hist(struct nvs_fs *fs, u16_t id, void *data, size_t len,
		  u16_t cnt)
{
	int rc;
	struct nvs_entry entry;
	u16_t cnt_his;

	if ((id == NVS_ID_EMPTY) || (id == NVS_ID_SECTOR_END)) {
		return -EINVAL;
	}
	entry.id = id;
	/* Read history entry */
	/* First find out how many entries are in the history */
	rc = nvs_get_first_entry(fs, &entry);
	if (rc) {
		goto err;
	}
	cnt_his = 0;
	while (1) {
		rc = nvs_walk_entry(fs, &entry);
		if (rc) {
			break;
		}
		/* disregard items with zero length */
		if (entry.len > 0) {
			cnt_his++;
		}
	}
	if (cnt_his < cnt) {
		/* Trying to read history item that is not available */
		return -ENOENT;
	}
	/* Now get the correct item by decreasing cnt_his until cnt
	 * is reached
	 */
	rc = nvs_get_first_entry(fs, &entry);
	if (rc) {
		goto err;
	}
	while (1) {
		if (cnt_his == cnt) {
			break;
		}
		rc = nvs_walk_entry(fs, &entry);
		if (rc) {
			break;
		}
		/* disregard items with zero length */
		if (entry.len > 0) {
			cnt_his--;
		}
	}

	if (rc) {
		goto err;
	}
	rc = nvs_check_crc(fs, &entry);
	if (rc) {
		goto err;
	}
	rc = nvs_flash_read(fs, entry.data_addr, data,
			    min(entry.len, len));
	if (rc) {
		goto err;
	}
	rc = entry.len;

err:
	return rc;
}
