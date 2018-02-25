/*  NVS: non volatile storage in flash
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <flash.h>
#include <crc16.h>
#include <nvs/nvs.h>

#define CONFIG_NVS_DEBUG 1
#if defined(CONFIG_NVS_DEBUG)
#define DBG_LAYOUT "[DBG_NVS] %s: " /* [DBG_NVS] function: */
#include <misc/printk.h>
#define DBG_NVS(fmt, ...) printk(DBG_LAYOUT fmt, __func__, ##__VA_ARGS__)
#endif
#if !defined(CONFIG_NVS_DEBUG)
#define DBG_NVS(...) {; }
#endif

#define NVS_ID_GT(a, b) (((a) > (b)) ? ((((a)-(b)) > 0x7FFF) ? (0):(1)) : \
	((((b)-(a)) > 0x7FFF) ? (1):(0)))

struct _nvs_sector_hdr {
	u32_t fd_magic;
	u16_t fd_id;
	u16_t _pad;
};

struct _nvs_data_hdr {
	u16_t id;
	u16_t len;
};

struct _nvs_data_slt {
	u16_t crc16;
	u16_t _pad;
};

static inline int _nvs_len_in_flash(struct nvs_fs *fs, u16_t len)
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
		/* operation outside fcb */
		return -1;
	}
	if ((offset & ~(fs->sector_size - 1)) !=
	((offset + len - 1) & ~(fs->sector_size - 1))) {
		/* operation over sector boundary */
		return -1;
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
		_nvs_len_in_flash(fs, sizeof(*sector_hdr)));
}

/* Initializing sector by writing magic and status to sector header,
 * set fs->write_address just after header offset should be pointing
 * to the sector
 */
int _nvs_sector_init(struct nvs_fs *fs, off_t offset)
{
	struct _nvs_sector_hdr sector_hdr;
	int rc;

	/* just to be sure, align offset to sector_size */
	offset &= ~(fs->sector_size - 1);
	fs->sector_id += 1;
	rc = _nvs_sector_hdr_get(fs, offset, &sector_hdr);
	if (rc) {
		return NVS_ERR_FLASH;
	}
	if (sector_hdr.fd_magic != 0xFFFFFFFF) {
		return NVS_ERR_NOSPACE;
	}
	sector_hdr.fd_magic = fs->magic;
	sector_hdr.fd_id = fs->sector_id;
	sector_hdr._pad = 0;
	rc = nvs_flash_write(fs, offset, &sector_hdr,
		_nvs_len_in_flash(fs, sizeof(struct _nvs_sector_hdr)));
	if (rc) {
		return NVS_ERR_FLASH;
	}
	fs->write_location = offset +
		_nvs_len_in_flash(fs, sizeof(struct _nvs_sector_hdr));
	return NVS_OK;
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
		rc = flash_read(fs->flash_device, fs->offset + addr,
			&buf, sizeof(buf));
		if (rc) {
			return NVS_ERR_FLASH;
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
		return NVS_ERR_FLASH;
	}

	rc = flash_erase(fs->flash_device, fs->offset + offset, len);
	if (rc) {
		/* flash write error */
		return NVS_ERR_FLASH;
	}

	DBG_NVS("Erasing flash at %x, len %x\n", offset, len);
	(void) flash_write_protection_set(fs->flash_device, 1);
	return NVS_OK;
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
	u8_t buf[NVS_MOVE_BLOCK_SIZE];

	walker.data_addr = addr;
	_nvs_addr_advance(fs, &walker.data_addr,
		_nvs_len_in_flash(fs, sizeof(struct _nvs_sector_hdr)) +
		_nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr)));
	while (1) {
		rc = nvs_flash_read(fs,
			_nvs_head_addr_in_flash(fs, &walker),
			&head,
			_nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr)));
		if (rc) {
			return NVS_ERR_FLASH;
		}
		if (head.id == NVS_ID_EMPTY) {
			return NVS_ERR_NOVAR;
		}
		if (head.id == NVS_ID_SECTOR_END) {
			return NVS_OK;
		}
		walker.len = head.len;
		walker.id = head.id;
		search.id = walker.id;
		if (nvs_get_first_entry(fs, &search) != NVS_OK) {
			/* entry is not found, copy needed - but find the last
			 * entry first
			 */
			walker_last = walker;
			while (head.id != NVS_ID_SECTOR_END) {
				rc = nvs_flash_read(fs,
					_nvs_head_addr_in_flash(fs,
						&walker_last),
					&head,
					_nvs_len_in_flash(fs,
						sizeof(struct _nvs_data_hdr)));
				if (rc) {
					return NVS_ERR_FLASH;
				}
				if (head.id == walker.id) {
					last_entry = walker_last;
				}
				walker_last.len = head.len;
				walker_last.id = head.id;
				_nvs_addr_advance(fs, &walker_last.data_addr,
					_nvs_entry_len_in_flash(fs,
						walker_last.len));
			}
			DBG_NVS("Entry with id %x moved to new flash sector\n",
				search.id);
			rd_addr = _nvs_head_addr_in_flash(fs, &last_entry);
			len = _nvs_entry_len_in_flash(fs, last_entry.len);
			while (len > 0) {
				bytes_to_copy = min(NVS_MOVE_BLOCK_SIZE, len);
				rc = nvs_flash_read(fs, rd_addr,
					&buf, bytes_to_copy);
				if (rc) {
					return NVS_ERR_FLASH;
				}
				rc = nvs_flash_write(fs, fs->write_location,
					&buf, bytes_to_copy);
				if (rc) {
					return NVS_ERR_FLASH;
				}
				len -= bytes_to_copy;
				rd_addr += bytes_to_copy;
				fs->write_location += bytes_to_copy;
			}
		}
		_nvs_addr_advance(fs, &walker.data_addr,
			_nvs_entry_len_in_flash(fs, walker.len));
	}
}

void nvs_set_start_entry(struct nvs_fs *fs, struct nvs_entry *entry)
{
	entry->data_addr = fs->entry_sector * fs->sector_size;
	_nvs_addr_advance(fs, &entry->data_addr,
		_nvs_len_in_flash(fs, sizeof(struct _nvs_sector_hdr)) +
		_nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr)));
}

int nvs_get_first_entry(struct nvs_fs *fs, struct nvs_entry *entry)
{
	int rc;
	struct _nvs_data_hdr head;

	nvs_set_start_entry(fs, entry);
	while (1) {
		rc = nvs_flash_read(fs,
			_nvs_head_addr_in_flash(fs, entry),
			&head,
			_nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr)));
		if (rc) {
			return NVS_ERR_FLASH;
		}
		if (head.id == NVS_ID_EMPTY) {
			return NVS_ERR_NOVAR;
		}
		if (head.id == entry->id) {
			entry->len = head.len;
			return NVS_OK;
		}
		_nvs_addr_advance(fs,
			&entry->data_addr,
			_nvs_entry_len_in_flash(fs, head.len));
	}
}


int nvs_get_last_entry(struct nvs_fs *fs, struct nvs_entry *entry)
{
	int rc;
	struct nvs_entry latest;
	struct _nvs_data_hdr head;

	rc = nvs_get_first_entry(fs, entry);
	if (rc) {
		if (rc == NVS_ERR_FLASH) {
			return NVS_ERR_FLASH;
		}
		return NVS_ERR_NOVAR;
	}
	latest.id = entry->id;
	latest.data_addr = entry->data_addr;
	latest.len = entry->len;
	while (1) {
		rc = nvs_flash_read(fs,
			_nvs_head_addr_in_flash(fs, entry),
			&head,
			_nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr)));
		if (rc) {
			return NVS_ERR_FLASH;
		}
		if (head.id == NVS_ID_EMPTY) {
			entry->id = latest.id;
			entry->data_addr = latest.data_addr;
			entry->len = latest.len;
			return NVS_OK;
		}
		if (head.id == latest.id) {
			latest.len = head.len;
			latest.data_addr = entry->data_addr;
		}
		_nvs_addr_advance(fs, &entry->data_addr,
			_nvs_entry_len_in_flash(fs, head.len));
	}
}

/* walking over entries, stops on empty or entry with same entry id */
int nvs_walk_entry(struct nvs_fs *fs, struct nvs_entry *entry)
{
	int rc;
	struct _nvs_data_hdr head;

	if (entry->id != NVS_ID_EMPTY) {
		_nvs_addr_advance(fs, &entry->data_addr,
			_nvs_entry_len_in_flash(fs, entry->len));
	}
	while (1) {
		rc = nvs_flash_read(fs,
			_nvs_head_addr_in_flash(fs, entry),
			&head,
			_nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr)));
		if (rc) {
			return NVS_ERR_FLASH;
		}
		if (head.id == entry->id) {
			entry->len = head.len;
			return NVS_OK;
		}
		if (head.id == NVS_ID_EMPTY) {
			return NVS_ERR_NOVAR;
		}
		_nvs_addr_advance(fs, &entry->data_addr,
			_nvs_entry_len_in_flash(fs, head.len));
	}
}

int nvs_init(struct nvs_fs *fs, const char *dev_name, u32_t magic)
{

	int i, rc, active_sector_cnt = 0;
	int entry_sector = -1;
	u16_t entry_sector_id, active_sector_id;
	struct _nvs_sector_hdr sector_hdr;
	struct nvs_entry entry;
	off_t addr;

	fs->magic = magic;
	fs->sector_id = 0;
	fs->max_len = fs->sector_size >> 2;
	fs->flash_device = device_get_binding(dev_name);
	if (!fs->flash_device) {
		DBG_NVS("No valid flash device found\n");
		return NVS_ERR_FLASH;
	}
	fs->write_block_size = flash_get_write_block_size(fs->flash_device);

	/* check the sector size, should be power of 2 */
	if (!((fs->sector_size != 0) &&
		!(fs->sector_size & (fs->sector_size - 1)))) {
		return NVS_ERR_CFG;
	}
	/* check the number of sectors, it should be at least 2 */
	if (fs->sector_count < 2) {
		return NVS_ERR_CFG;
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
		DBG_NVS("No valid sectors found, initializing sectors\n");
		for (addr = 0; addr < (fs->sector_count * fs->sector_size);
			addr += fs->sector_size) {
			/* first check if used, only erase if it is */
			if (_nvs_sector_is_used(fs, addr)) {
				rc = _nvs_flash_erase(fs,
					addr,
					fs->sector_size);
				if (rc) {
					return NVS_ERR_FLASH;
				}
			}
		}
		if (_nvs_sector_init(fs, 0)) {
			return NVS_ERR_FLASH;
		}
		entry_sector = 0;
		active_sector_id = fs->sector_id;
	}
	fs->entry_sector = entry_sector;
	fs->sector_id = active_sector_id;

	/* Find the first empty entry */
	nvs_set_start_entry(fs, &entry);
	entry.id = NVS_ID_EMPTY;
	if (nvs_walk_entry(fs, &entry)) {
		return NVS_ERR_FLASH;
	}
	fs->write_location = _nvs_head_addr_in_flash(fs, &entry);
	if (active_sector_cnt == fs->sector_count) {
		/* one sector should always be empty, unless power was cut
		 * during garbage collection start gc again on the last
		 * sector.
		 */
		DBG_NVS("Restarting garbage collection\n");
		addr = fs->entry_sector * fs->sector_size;
		_nvs_entry_sector_advance(fs);
		rc = _nvs_gc(fs, addr);
		if (rc) {
			return NVS_ERR_FLASH;
		}
		if (_nvs_flash_erase(fs, addr, fs->sector_size)) {
			return NVS_ERR_FLASH;
		}
	}

	DBG_NVS("maximum storage length %d bytes\n", fs->max_len);
	DBG_NVS("write-align: %d, write-addr: %x\n", fs->write_block_size,
		fs->write_location);
	DBG_NVS("entry sector: %d, entry sector ID: %d\n", fs->entry_sector,
		fs->sector_id);

	k_mutex_init(&fs->fcb_lock);

	return NVS_OK;
}

int nvs_append(struct nvs_fs *fs, struct nvs_entry *entry)
{
	int rc;
	u16_t required_len, extended_len;
	struct _nvs_data_hdr data_hdr;

	rc = 0;

	k_mutex_lock(&fs->fcb_lock, K_FOREVER);
	required_len = _nvs_entry_len_in_flash(fs, entry->len);

	/* the space available should be big enough to fit the data + header
	 * and slot of next data
	 */
	extended_len = required_len +
		_nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr)) +
		_nvs_len_in_flash(fs, sizeof(struct _nvs_data_slt));
	if ((fs->sector_size - (fs->write_location & (fs->sector_size - 1))) <
		extended_len) {
		rc = NVS_ERR_NOSPACE;
		goto err;
	}
	data_hdr.id = entry->id;
	data_hdr.len = _nvs_len_in_flash(fs, entry->len);

	rc = nvs_flash_write(fs, fs->write_location, &data_hdr,
		_nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr)));
	if (rc) {
		goto err;
	}

	entry->data_addr = fs->write_location +
		_nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr));
	fs->write_location += required_len;
	rc = NVS_OK;

err:
	k_mutex_unlock(&fs->fcb_lock);
	return rc;
}

/* close the append by applying a seal to the data that includes a crc */
int nvs_append_close(struct nvs_fs *fs, const struct nvs_entry *entry)
{
	int rc;
	struct _nvs_data_slt data_slt;

	k_mutex_lock(&fs->fcb_lock, K_FOREVER);
	/* crc16_ccitt is calculated on flash data, set correct offset */
	data_slt.crc16 = crc16_ccitt(0xFFFF,
		(const u8_t *)(entry->data_addr + fs->offset), entry->len);
	data_slt._pad = 0xFFFF;
	rc = nvs_flash_write(fs, _nvs_slt_addr_in_flash(fs, entry), &data_slt,
		_nvs_len_in_flash(fs, sizeof(struct _nvs_data_slt)));
	k_mutex_unlock(&fs->fcb_lock);
	return rc;
}

/* check the crc of an entry */
int nvs_check_crc(struct nvs_fs *fs, struct nvs_entry *entry)
{
	int rc;
	struct _nvs_data_slt data_slt;
	u16_t crc16;

	/* crc16_ccitt is calculated on flash data, set correct offset */
	crc16 = crc16_ccitt(0xFFFF,
			(const u8_t *)(entry->data_addr + fs->offset),
			entry->len);
	rc = nvs_flash_read(fs,
		_nvs_slt_addr_in_flash(fs, entry),
		&data_slt,
		_nvs_len_in_flash(fs, sizeof(struct _nvs_data_slt)));
	if (rc || (crc16 != data_slt.crc16)) {
		return NVS_ERR_CRC;
	}
	return NVS_OK;
}



/* rotate the fcb, frees the next sector (based on fs->write_location) */
int nvs_rotate(struct nvs_fs *fs)
{
	int rc;
	off_t addr;
	struct _nvs_data_hdr head;

	rc = 0;

	k_mutex_lock(&fs->fcb_lock, K_FOREVER);

	/* fill previous sector with data to jump to the next sector */
	head.id = NVS_ID_SECTOR_END;
	head.len = fs->sector_size -
		(fs->write_location & (fs->sector_size - 1)) -
		_nvs_len_in_flash(fs, sizeof(struct _nvs_data_slt)) -
		_nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr)) +
		_nvs_len_in_flash(fs, sizeof(struct _nvs_sector_hdr));
	rc = nvs_flash_write(fs,
		fs->write_location,
		&head,
		_nvs_len_in_flash(fs, sizeof(struct _nvs_data_hdr)));
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
		DBG_NVS("Starting data copy...\n");
		_nvs_entry_sector_advance(fs);
		rc = _nvs_gc(fs, addr);
		if (rc) {
			DBG_NVS("Quit data copy - gc error\n");
			goto out;
		}
		if (_nvs_flash_erase(fs, addr, fs->sector_size)) {
			rc = NVS_ERR_FLASH;
			DBG_NVS("Quit data copy - flash erase error\n");
			goto out;
		}
		DBG_NVS("Done data copy - no error\n");
	}

	rc = NVS_OK;

out:
	k_mutex_unlock(&fs->fcb_lock);
	return rc;
}

int nvs_clear(struct nvs_fs *fs)
{
	int rc;
	off_t addr;

	k_mutex_lock(&fs->fcb_lock, K_FOREVER);

	for (addr = 0; addr < fs->sector_count * fs->sector_size;
		addr += fs->sector_size) {
		rc = _nvs_flash_erase(fs, addr, fs->sector_size);
		if (rc) {
			goto out;
		}
	}
	rc = NVS_OK;

out:
	k_mutex_unlock(&fs->fcb_lock);
	return rc;
}

int nvs_flash_read(struct nvs_fs *fs, off_t offset, void *data, size_t len)
{

	if (_nvs_bd_check(fs, offset, len)) {
		return NVS_ERR_ARGS;
	}

	if (flash_read(fs->flash_device, fs->offset + offset, data, len)) {
		/* flash read error */
		return NVS_ERR_FLASH;
	}
	return NVS_OK;
}

int nvs_flash_write(struct nvs_fs *fs, off_t offset, const void *data,
	size_t len)
{

	if (_nvs_bd_check(fs, offset, len)) {
		return NVS_ERR_ARGS;
	}

	if (flash_write_protection_set(fs->flash_device, 0)) {
		/* flash protection set error */
		return NVS_ERR_FLASH;
	}
	if (flash_write(fs->flash_device, fs->offset + offset, data, len)) {
		/* flash write error */
		return NVS_ERR_FLASH;
	}
	(void) flash_write_protection_set(fs->flash_device, 1);
	/* don't mind about this error */
	return NVS_OK;
}


int nvs_write(struct nvs_fs *fs, struct nvs_entry *entry, const void *data)
{
	int rc;

	if ((entry->id == NVS_ID_EMPTY) || (entry->id == NVS_ID_SECTOR_END)) {
		return NVS_ERR_ARGS;
	}
	if (entry->len > fs->max_len) {
		return NVS_ERR_LEN;
	}
	while (1) {
		rc = nvs_append(fs, entry);
		if (rc) {
			if (rc == NVS_ERR_NOSPACE) {
				if (nvs_rotate(fs)) {
					goto err;
				}
			} else {
				goto err;
			}
		} else {
			break;
		}
	}
	rc = nvs_flash_write(fs, entry->data_addr, data, entry->len);
	if (rc) {
		goto err;
	}
	rc = nvs_append_close(fs, entry);
	if (rc) {
		goto err;
	}
	return NVS_OK;
err:
	return rc;
}

int nvs_read(struct nvs_fs *fs, struct nvs_entry *entry, void *data)
{
	return nvs_read_hist(fs, entry, data, 2);
}

int nvs_read_hist(struct nvs_fs *fs, struct nvs_entry *entry, void *data,
		  u8_t mode)
{
	int rc;

	if ((entry->id == NVS_ID_EMPTY) || (entry->id == NVS_ID_SECTOR_END)) {
		return NVS_ERR_ARGS;
	}
	switch (mode) {
	case 0: /* Read first entry */
		rc = nvs_get_first_entry(fs, entry);
		break;
	case 1: /* Read next entry */
		rc = nvs_walk_entry(fs, entry);
		break;
	case 2: /* Read last entry */
		rc = nvs_get_last_entry(fs, entry);
		break;
	default:
		return NVS_ERR_ARGS;
	}
	if (rc) {
		goto err;
	}
	rc = nvs_check_crc(fs, entry);
	if (rc) {
		goto err;
	}
	rc = nvs_flash_read(fs, entry->data_addr, data, entry->len);
	if (rc) {
		goto err;
	}
	return NVS_OK;

err:
	return rc;
}
