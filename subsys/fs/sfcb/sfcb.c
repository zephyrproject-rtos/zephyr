/*  Simple Flash Circular Buffer for storage */

/*
 * Copyright (c) 2017 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <device.h>
#include <flash.h>
#include <crc16.h>
#include <sfcb/sfcb.h>

#define CONFIG_SFCB_DEBUG 1
#if defined(CONFIG_SFCB_DEBUG)
#define DBG_LAYOUT "[DBG_SFCB] %s: " /* [DBG_SFCB] function: */
#include <misc/printk.h>
#define DBG_SFCB(fmt,...) printk(DBG_LAYOUT fmt,__func__, ##__VA_ARGS__)
#endif
#if !defined(CONFIG_SFCB_DEBUG)
#define DBG_SFCB(...) { ; }
#endif

#define SFCB_ID_GT(a, b) (((a)>(b))?((((a)-(b))>0x7FFF)?(0):(1)):((((b)-(a))>0x7FFF)?(1):(0)))

struct _fcb_sector_hdr {
  u32_t fd_magic;
  u16_t fd_id;
  u16_t _pad;
};

struct _fcb_data_hdr {
  u16_t id;
  u16_t len;
};

struct _fcb_data_slt {
  u16_t crc16;
  u16_t _pad;
};

static inline int _sfcb_len_in_flash(struct sfcb_fs *fs, u16_t len)
{
    if (fs->write_block_size <= 1) {
        return len;
    }
    return (len + (fs->write_block_size - 1)) & ~(fs->write_block_size - 1);
}

u16_t _sfcb_entry_len_in_flash(struct sfcb_fs *fs, u16_t len) {
  return _sfcb_len_in_flash(fs, len) + _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_hdr))
                                    + _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_slt));
}


off_t _sfcb_head_addr_in_flash(struct sfcb_fs *fs, const struct sfcb_entry *entry) {
  return entry->data_addr - _sfcb_len_in_flash(fs, sizeof(struct _fcb_data_hdr));
}

off_t _sfcb_slt_addr_in_flash(struct sfcb_fs *fs, const struct sfcb_entry *entry) {
  return entry->data_addr + _sfcb_len_in_flash(fs, entry->len);
}

int _sfcb_bd_check(struct sfcb_fs *fs, off_t offset, size_t len) {
  if ((offset & ~(fs->sector_size-1)) != ((offset + len - 1) & ~(fs->sector_size-1))) {
    /* operation over sector boundary */
    return -1;
  }
  return 0;
}

void _sfcb_addr_advance(struct sfcb_fs *fs, off_t *addr, u16_t step) {
  *addr += step;
  if (*addr >= fs->fap.fa_size) {
    *addr -= fs->fap.fa_size;
  }
}

/* Getting the sector header for address given in offset */
int _sfcb_sector_hdr_get(struct sfcb_fs *fs, off_t offset, struct _fcb_sector_hdr *sector_hdr) {
  return flash_area_read(&fs->fap, (offset & ~(fs->sector_size-1)) , sector_hdr, _sfcb_len_in_flash(fs, sizeof( *sector_hdr)));
}

/* Initializing sector by writing magic and status to sector header, set fs->write_address just after header
   offset should be pointing to the sector */
int _sfcb_sector_init(struct sfcb_fs *fs, off_t offset) {
  struct _fcb_sector_hdr sector_hdr;
  int rc;

  /* just to be sure, align offset to sector_size */
  offset &= ~(fs->sector_size - 1);
  fs->sector_id += 1;
  rc=_sfcb_sector_hdr_get(fs, offset, &sector_hdr);
  if (rc) {
    return SFCB_ERR_FLASH;
  }
  if (sector_hdr.fd_magic != 0xFFFFFFFF) {
    return SFCB_ERR_NOSPACE;
  }
  sector_hdr.fd_magic = fs->magic;
  sector_hdr.fd_id = fs->sector_id;
  sector_hdr._pad = 0;
  rc=sfcb_fs_flash_write(fs, offset, &sector_hdr, _sfcb_len_in_flash(fs,sizeof(struct _fcb_sector_hdr)));
  if (rc) {
    return SFCB_ERR_FLASH;
  }
  fs->write_location = offset + _sfcb_len_in_flash(fs,sizeof(struct _fcb_sector_hdr));
  return SFCB_OK;
}

/* check if a sector is used by looking at all elements of the sector.
   return 0 if sector is empty, 1 if it is not empty */
int _sfcb_fs_sector_is_used(struct sfcb_fs *fs, off_t offset) {
  int rc;
  off_t addr;
  u8_t buf;
  rc = 0;
  offset &= ~(fs->sector_size-1);
  for (addr=0;addr<fs->sector_size;addr += sizeof(buf)) {
    rc = flash_area_read(&fs->fap, addr , &buf, sizeof(buf));
    if (rc) {
      return SFCB_ERR_FLASH;
    }
    if (buf!=0xFF) {
      return 1;
    }
  }
  return 0;
}

/* erase the flash starting at offset, erase length = len */
int _sfcb_fs_flash_erase(struct sfcb_fs *fs, off_t offset, size_t len) {
  int rc;

  rc=flash_area_erase(&fs->fap, offset , len);
  if (rc) {
     /* flash write error */
     return SFCB_ERR_FLASH;
  }

  DBG_SFCB("Erasing flash at %x, len %x\n", offset,len);
  return SFCB_OK;
}

void _sfcb_entry_sector_advance(struct sfcb_fs *fs) {
  fs->entry_sector++;
  if (fs->entry_sector == fs->sector_count) {
    fs->entry_sector = 0;
  }
}

/* garbage collection: addr is set to the start of the sector to be gc'ed,
   the entry sector has been updated to point to the sector just after the
   sector being gc'ed */
int _sfcb_gc(struct sfcb_fs *fs, off_t addr) {
  int len,bytes_to_copy;
  off_t rd_addr;
  struct sfcb_entry walker, search;
  struct _fcb_data_hdr head;
  u8_t buf[SFCB_MOVE_BLOCK_SIZE];

  walker.data_addr = addr;
  _sfcb_addr_advance(fs,&walker.data_addr, _sfcb_len_in_flash(fs,sizeof(struct _fcb_sector_hdr))
                                         + _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_hdr)));
  while (1) {
    if (sfcb_fs_flash_read(fs, _sfcb_head_addr_in_flash(fs, &walker), &head, _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_hdr)))) {
      return SFCB_ERR_FLASH;
    }
    if (head.id == SFCB_ID_EMPTY) {
      return SFCB_ERR_NOVAR;
    }
    if (head.id == SFCB_ID_SECTOR_END) {
      return SFCB_OK;
    }
    walker.len = head.len;
    walker.id = head.id;
    search.id = walker.id;
    if (sfcb_fs_get_first_entry(fs, &search) != SFCB_OK) {
      /* entry is not found, copy needed */
      DBG_SFCB("Copying entry with id %x to front of circular buffer\n", search.id);
      rd_addr = _sfcb_head_addr_in_flash(fs, &walker);
      len = _sfcb_entry_len_in_flash(fs, walker.len);
      while (len>0) {
        bytes_to_copy=min(SFCB_MOVE_BLOCK_SIZE,len);
        if (sfcb_fs_flash_read(fs, rd_addr, &buf, bytes_to_copy)) {
          return SFCB_ERR_FLASH;
        }
        if (sfcb_fs_flash_write(fs, fs->write_location, &buf, bytes_to_copy)) {
          return SFCB_ERR_FLASH;
        }
        len -= bytes_to_copy;
        rd_addr += bytes_to_copy;
        fs->write_location += bytes_to_copy;
      }
    }
    _sfcb_addr_advance(fs,&walker.data_addr, _sfcb_entry_len_in_flash(fs,walker.len));
  }
}

void sfcb_fs_set_start_entry(struct sfcb_fs *fs, struct sfcb_entry *entry) {
  entry->data_addr = fs->entry_sector * fs->sector_size;
  _sfcb_addr_advance(fs,&entry->data_addr, _sfcb_len_in_flash(fs,sizeof(struct _fcb_sector_hdr))
                                         + _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_hdr)));
}

int sfcb_fs_get_first_entry(struct sfcb_fs *fs, struct sfcb_entry *entry) {
  int rc;
  struct _fcb_data_hdr head;

  sfcb_fs_set_start_entry(fs, entry);
  while (1) {
    rc=sfcb_fs_flash_read(fs, _sfcb_head_addr_in_flash(fs, entry), &head, _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_hdr)));
    if (rc) {
      return SFCB_ERR_FLASH;
    }
    if (head.id == SFCB_ID_EMPTY) {
      return SFCB_ERR_NOVAR;
    }
    if (head.id == entry->id) {
      entry->len = head.len;
      return SFCB_OK;
    }
    _sfcb_addr_advance(fs,&entry->data_addr, _sfcb_entry_len_in_flash(fs,head.len));
  }
}


int sfcb_fs_get_last_entry(struct sfcb_fs *fs, struct sfcb_entry *entry) {
  int rc;
  struct sfcb_entry latest;
  struct _fcb_data_hdr head;

  rc=sfcb_fs_get_first_entry(fs, entry);
  if (rc) {
    if (rc == SFCB_ERR_FLASH) {
      return SFCB_ERR_FLASH;
    }
    return SFCB_ERR_NOVAR;
  }
  latest.id = entry->id;
  latest.data_addr = entry->data_addr;
  latest.len = entry->len;
  while (1) {
    if (sfcb_fs_flash_read(fs, _sfcb_head_addr_in_flash(fs, entry), &head, _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_hdr)))) {
      return SFCB_ERR_FLASH;
    }
    if  (head.id == SFCB_ID_EMPTY) {
      entry->id = latest.id;
      entry->data_addr = latest.data_addr;
      entry->len = latest.len;
      return SFCB_OK;
    }
    if (head.id == latest.id) {
      latest.len = entry->len;
      latest.data_addr = entry->data_addr;
    }
    _sfcb_addr_advance(fs,&entry->data_addr, _sfcb_entry_len_in_flash(fs,head.len));

  }
}

/* walking over entries, stops on empty or entry with same entry id */
int sfcb_fs_walk_entry(struct sfcb_fs *fs, struct sfcb_entry *entry) {
  struct _fcb_data_hdr head;

  while (1) {
    if (sfcb_fs_flash_read(fs, _sfcb_head_addr_in_flash(fs, entry), &head, _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_hdr)))) {
      return SFCB_ERR_FLASH;
    }
    if (head.id == entry->id) {
      return SFCB_OK;
    }
    if (head.id == SFCB_ID_EMPTY) {
      return SFCB_ERR_NOVAR;
    }
    entry->len = head.len;
    _sfcb_addr_advance(fs,&entry->data_addr, _sfcb_entry_len_in_flash(fs,head.len));
  }
}

int sfcb_fs_init(struct sfcb_fs *fs, u32_t magic) {

  int i,rc,active_sector_cnt=0;
  int entry_sector=-1;
  u16_t entry_sector_id,active_sector_id;
  struct _fcb_sector_hdr sector_hdr;
  struct sfcb_entry entry;
  off_t addr;

  fs->magic = magic;
  fs->sector_id = 0;
  fs->write_block_size = flash_area_align(&fs->fap);

  /* check the sector size, should be power of 2 */
  if (!((fs->sector_size != 0) && !(fs->sector_size & (fs->sector_size - 1)))) {
    return SFCB_ERR_CFG;
  }
  /* check the number of sectors */
  if (((fs->gc) && (fs->sector_count < 2)) || ((!fs->gc) && (fs->sector_count < 1)))  {
    return SFCB_ERR_CFG;
  }
  for (i=0; i<fs->sector_count;i++) {
    rc=_sfcb_sector_hdr_get(fs, i * fs->sector_size, &sector_hdr);
    if (rc) {
      return rc;
    }
    if (sector_hdr.fd_magic != fs->magic) {
      continue;
    }
    active_sector_cnt++;
    if (entry_sector<0) {
      entry_sector=i;
      entry_sector_id=sector_hdr.fd_id;
      active_sector_id=sector_hdr.fd_id;
      continue;
    }
    if SFCB_ID_GT(sector_hdr.fd_id, active_sector_id) {
      active_sector_id = sector_hdr.fd_id;
    }
    if SFCB_ID_GT(entry_sector_id, sector_hdr.fd_id) {
      entry_sector = i;
      entry_sector_id = sector_hdr.fd_id;
    }
  }
  if (entry_sector < 0) { /* No valid sectors found */
    DBG_SFCB("No valid sectors found, initializing sectors\n");
    for (addr=0;addr<fs->fap.fa_size;addr+=fs->sector_size) {
      /* first check if used, only erase if it is */
      if (_sfcb_fs_sector_is_used(fs, addr)) {
        if (_sfcb_fs_flash_erase(fs, addr, fs->sector_size)) {
          return SFCB_ERR_FLASH;
        }
      }
    }
    if (_sfcb_sector_init(fs, 0)) {
      return SFCB_ERR_FLASH;
    }
    entry_sector = 0;
    active_sector_id = fs->sector_id;
  }
  fs->entry_sector = entry_sector;
  fs->sector_id = active_sector_id;

  /* Find the first empty entry */
  sfcb_fs_set_start_entry(fs, &entry);
  entry.id = SFCB_ID_EMPTY;
  if (sfcb_fs_walk_entry(fs,&entry)) {
     return SFCB_ERR_FLASH;
  }

  fs->write_location = _sfcb_head_addr_in_flash(fs, &entry);

  if ((fs->gc)&&(active_sector_cnt==fs->sector_count)) {
    /* In gc mode one sector should always be empty, unless power was cut during garbage collection
       start gc again on the last sector */
    DBG_SFCB("Restarting garbage collection\n");
    addr = fs->entry_sector * fs->sector_size;
    _sfcb_entry_sector_advance(fs);
    rc = _sfcb_gc(fs, addr);
    if (rc) {
      return SFCB_ERR_FLASH;
    }
    if (_sfcb_fs_flash_erase(fs, addr, fs->sector_size)) {
      return SFCB_ERR_FLASH;
    }
  }

  DBG_SFCB("Finished init:\n...write-align: %d, entry sector: %d, entry sector ID: %d, write-addr: %x\n",fs->write_block_size, fs->entry_sector, fs->sector_id, fs->write_location);

  k_mutex_init(&fs->fcb_lock);

  return SFCB_OK;
}

int sfcb_fs_append(struct sfcb_fs *fs, struct sfcb_entry *entry) {
  int rc;
  u16_t required_len,extended_len;
  struct _fcb_data_hdr data_hdr;
  rc = 0;

  k_mutex_lock(&fs->fcb_lock, K_FOREVER);
  required_len = _sfcb_entry_len_in_flash(fs, entry->len);

  /* the space available should be big enough to fit the data + header and slot of next data */
  extended_len = required_len + _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_hdr))
                              + _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_slt));
  if ((fs->sector_size - (fs->write_location & (fs->sector_size-1))) < extended_len) {
    rc = SFCB_ERR_NOSPACE;
    goto err;
  }
  data_hdr.id=entry->id;
  data_hdr.len= _sfcb_len_in_flash(fs, entry->len);

  rc = sfcb_fs_flash_write(fs, fs->write_location, &data_hdr, _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_hdr)));
  if (rc) {
    goto err;
  }

  entry->data_addr = fs->write_location + _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_hdr));
  fs->write_location += required_len;
  rc = SFCB_OK;

  err:
    k_mutex_unlock(&fs->fcb_lock);
    return(rc);
}
/* close the append by applying a seal to the data that includes a crc */
int sfcb_fs_append_close(struct sfcb_fs *fs, const struct sfcb_entry *entry) {
  int rc;
  struct _fcb_data_slt data_slt;

  k_mutex_lock(&fs->fcb_lock, K_FOREVER);
  // crc16_ccitt is directly calculated on flash data, set the correct offset !!!
  data_slt.crc16 = crc16_ccitt((const u8_t *)(entry->data_addr + fs->fap.fa_off),entry->len);
  data_slt._pad = 0xFFFF;
  rc = sfcb_fs_flash_write(fs, _sfcb_slt_addr_in_flash(fs,entry), &data_slt, _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_slt)));
  k_mutex_unlock(&fs->fcb_lock);
  return(rc);
}

/* check the crc of an entry */
int sfcb_fs_check_crc(struct sfcb_fs *fs, struct sfcb_entry *entry) {
  int rc;
  struct _fcb_data_slt data_slt;
  u16_t crc16;
  // crc16_ccitt is directly calculated on flash data, set the correct offset !!!
  crc16 = crc16_ccitt((const u8_t *)(entry->data_addr + fs->fap.fa_off),entry->len);
  rc = sfcb_fs_flash_read(fs, _sfcb_slt_addr_in_flash(fs,entry), &data_slt, _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_slt)));
  if (rc || (crc16 != data_slt.crc16)) {
    return SFCB_ERR_CRC;
  }
  return SFCB_OK;
}



/* rotate the fcb, frees the next sector (based on fs->write_location) */
int sfcb_fs_rotate(struct sfcb_fs *fs) {
  int rc;
  off_t addr;
  struct _fcb_data_hdr head;

  rc = 0;

  k_mutex_lock(&fs->fcb_lock, K_FOREVER);

  /* fill up previous sector with invalid data to jump into the next sector */
  head.id=SFCB_ID_SECTOR_END;
  head.len=fs->sector_size - (fs->write_location & (fs->sector_size -1))
                           - _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_slt))
                           - _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_hdr))
                           + _sfcb_len_in_flash(fs,sizeof(struct _fcb_sector_hdr));
  rc = sfcb_fs_flash_write(fs, fs->write_location, &head, _sfcb_len_in_flash(fs,sizeof(struct _fcb_data_hdr)));
  if (rc) {
    goto out;
  }

  /* advance to next sector for writing */
  addr = (fs->write_location & ~(fs->sector_size-1));
  _sfcb_addr_advance(fs,&addr,fs->sector_size);

  if (!fs->gc) {
    /* when gc is not enabled we need to erase the sector before writing,
       except when the entry sector has not been reached */
    if ((addr & ~(fs->sector_size-1)) == fs->entry_sector * fs->sector_size) {
      _sfcb_entry_sector_advance(fs);
      if (_sfcb_fs_flash_erase(fs, addr, fs->sector_size)) {
        rc = SFCB_ERR_FLASH;
        goto out;
      }
    }
  }

  /* initialize the new sector */
  rc = _sfcb_sector_init(fs, addr);
  if (rc) {
     goto out;
  }

  if (fs->gc) {
    addr = (fs->write_location & ~(fs->sector_size-1));
    _sfcb_addr_advance(fs,&addr, fs->sector_size);
    /* Do we need to advance the entry_sector, if so we need to copy */
    if ((addr & ~(fs->sector_size-1)) == fs->entry_sector * fs->sector_size) {
      DBG_SFCB("Starting garbage collection...\n");
      _sfcb_entry_sector_advance(fs);
      rc = _sfcb_gc(fs, addr);
      if (rc) {
        DBG_SFCB("Quiting garbage collection with gc error\n");
        goto out;
      }
      if (_sfcb_fs_flash_erase(fs, addr, fs->sector_size)) {
        rc = SFCB_ERR_FLASH;
        DBG_SFCB("Quiting garbage collection with flash erase error\n");
        goto out;
      }
      DBG_SFCB("Done garbage collection without error\n");
    }

  }
  rc = SFCB_OK;

  out:
    k_mutex_unlock(&fs->fcb_lock);
    return(rc);
}

int sfcb_fs_clear(struct sfcb_fs *fs) {
  int rc;
  off_t addr;

  k_mutex_lock(&fs->fcb_lock, K_FOREVER);
  for (addr=0;addr<fs->fap.fa_size;addr+=fs->sector_size) {
    rc=_sfcb_fs_flash_erase(fs, addr, fs->sector_size);
    if (rc) {
      goto out;
    }
  }
  rc = SFCB_OK;

  out:
    k_mutex_unlock(&fs->fcb_lock);
    return rc;
}

int sfcb_fs_flash_read(struct sfcb_fs *fs, off_t offset, void *data, size_t len) {

  if (_sfcb_bd_check(fs, offset, len)) {
    return SFCB_ERR_ARGS;
  }

  if (flash_area_read(&fs->fap, offset, data, len)) {
    /* flash read error */
    return SFCB_ERR_FLASH;
  }
  return SFCB_OK;
}

int sfcb_fs_flash_write(struct sfcb_fs *fs, off_t offset, const void *data, size_t len) {

  if (_sfcb_bd_check(fs, offset, len)) {
    return SFCB_ERR_ARGS;
  }

  if (flash_area_write(&fs->fap, offset, data, len)) {
    /* flash write error */
    return SFCB_ERR_FLASH;
  }
  return SFCB_OK;
}
