/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zffs/area.h"
#include <string.h>

struct zffs_disk_area {
	char fs_name[16];
	zffs_disk(u32_t, length);
	zffs_disk(u16_t, erase_seq);
	u8_t ver;
	u8_t id;
	u8_t gc_seq;
	zffs_disk(u16_t, crc);
};

#define area_offset(__zffs, __area) ((__area) - ((__zffs)->base_area))
#define area_foreach(__zffs, __area)			     \
	for (struct zffs_area *__area = (__zffs)->base_area; \
	     area_offset(__zffs, __area) < (__zffs)->area_num; __area++)
#define area_size(__area) ((__area)->length - sizeof(struct zffs_disk_area))
#define area_flash_addr(__area, __addr)	\
	((__area)->offset + sizeof(struct zffs_disk_area) + (__addr))
#define area_list_foreach(__area_list, __area)				\
	for (struct zffs_area *__area = *(__area_list); __area != NULL;	\
	     __area = *(++(__area_list)))
#define area_type(__area) ((__area)->type)
#define area_id(__area) ((__area)->id)
#define area_list_next(__area_list, __area) \
	((__area_list)[area_id(__area) - area_id((__area_list)[0]) + 1])

inline static int area_read(const struct flash_area *fa, off_t off, void *dst,
			    size_t len)
{
#if ZFFS_CONFIG_AREA_ALIGNED_SIZE == 1
	return flash_area_read(fa, off, dst, len);
#else
	u32_t buf;
	int rc;
	int byte_off, bytes;

	byte_off = off & (ZFFS_CONFIG_AREA_ALIGNED_SIZE - 1);

	if (byte_off) {
		rc = flash_area_read(fa,
				     off & ~(ZFFS_CONFIG_AREA_ALIGNED_SIZE - 1),
				     &buf, ZFFS_CONFIG_AREA_ALIGNED_SIZE);
		if (rc) {
			return rc;
		}

		bytes = min(ZFFS_CONFIG_AREA_ALIGNED_SIZE - byte_off, len);

		memcpy(dst, ((u8_t *)&buf) + byte_off, bytes);
		off += bytes;
		dst = (u8_t *)dst + bytes;
		len -= bytes;
	}

	bytes = len & ~(ZFFS_CONFIG_AREA_ALIGNED_SIZE - 1);

	if (bytes) {
		rc = flash_area_read(fa, off, dst, bytes);
		if (rc) {
			return rc;
		}

		off += bytes;
		dst = (u8_t *)dst + bytes;
		len -= bytes;
	}

	if (len) {

		rc = flash_area_read(fa, off, &buf,
				     ZFFS_CONFIG_AREA_ALIGNED_SIZE);
		if (rc) {
			return rc;
		}

		memcpy(dst, &buf, len);
	}

	return 0;
#endif
}

inline static int area_write(const struct flash_area *fa, off_t off,
			     const void *src, size_t len)
{
#if ZFFS_CONFIG_AREA_ALIGNED_SIZE == 1
	return flash_area_write(fa, off, src, len);
#else
	u32_t buf;
	int rc;
	int byte_off, bytes;

	byte_off = off & (ZFFS_CONFIG_AREA_ALIGNED_SIZE - 1);

	if (byte_off) {
		bytes = min(ZFFS_CONFIG_AREA_ALIGNED_SIZE - byte_off, len);
		buf = 0xffffffff;
		memcpy(((u8_t *)&buf) + byte_off, src, bytes);

		rc = flash_area_write(fa,
				      off & ~(ZFFS_CONFIG_AREA_ALIGNED_SIZE - 1),
				      &buf, ZFFS_CONFIG_AREA_ALIGNED_SIZE);
		if (rc) {
			return rc;
		}

		off += bytes;
		src = (const u8_t *)src + bytes;
		len -= bytes;
	}

	bytes = len & ~(ZFFS_CONFIG_AREA_ALIGNED_SIZE - 1);

	if (bytes) {
		rc = flash_area_write(fa, off, src, bytes);
		if (rc) {
			return rc;
		}

		off += bytes;
		src = (u8_t *)src + bytes;
		len -= bytes;
	}

	if (len) {
		buf = 0xffffffff;
		memcpy(&buf, src, len);
		rc = flash_area_write(fa, off, &buf,
				      ZFFS_CONFIG_AREA_ALIGNED_SIZE);
		if (rc) {
			return rc;
		}
	}

	return 0;
#endif
}

static struct zffs_area *area_search(struct zffs_data *zffs, u8_t id)
{
	area_foreach(zffs, area){
		if (area->full_id == id) {
			return area;
		}
	}
	return NULL;
}

static void area_swap(struct zffs_area **area_a, struct zffs_area **area_b)
{
	struct zffs_area *tmp = *area_a;

	*area_a = *area_b;
	*area_b = tmp;
}

static struct zffs_area *area_new(struct zffs_data *zffs, u8_t id,
				  u8_t gc_seq)
{
	k_mutex_lock(&zffs->lock, K_FOREVER);

	struct zffs_area **area_a = zffs->area;
	struct zffs_area **area_b = area_a + 1;
	struct zffs_area *area = NULL;

	while (*area_b) {
		if ((*area_a)->erase_seq > (*area_b)->erase_seq) {
			area_a = area_b;
		}
		area_b++;
	}
	area_swap(area_a, area_b - 1);
	area = *(area_b - 1);
	*(area_b - 1) = NULL;

	struct zffs_disk_area disk_area;

	if (area) {
		if (area_read(zffs->flash, area->offset, &disk_area,
			      offsetof(struct zffs_disk_area, id))) {
			area = NULL;
			goto done;
		}

		disk_area.id = id & ~ZFFS_AREA_ID_TYPE_GC_MAKS;
		area->gc_seq = gc_seq;
		disk_area.gc_seq = area->gc_seq;

		sys_put_le16(crc16_ccitt(0, (const void *)&disk_area,
					 offsetof(struct zffs_disk_area, crc)),
			     disk_area.crc);

		if (area_write(
			    zffs->flash,
			    area->offset + offsetof(struct zffs_disk_area, id),
			    &disk_area.id,
			    sizeof(struct zffs_disk_area) -
			    offsetof(struct zffs_disk_area, id))) {
			area = NULL;
			goto done;
		}
		area->full_id = id;
	}

done:
	k_mutex_unlock(&zffs->lock);
	return area;
}

u32_t zffs_area_list_size(struct zffs_area **area_list)
{
	u32_t size = 0;

	area_list_foreach(area_list, area) {
		size += area_size(area);
	}
	return size;
}

static void area_list_append(struct zffs_area **area_list,
			     struct zffs_area *area)
{
	area_list_foreach(area_list, a){
		if (area == a) {
			return;
		}
	}

	*area_list = area;
	*(area_list + 1) = NULL;
}

static int area_random_read(struct zffs_data *zffs,
			    struct zffs_area **area_list, u32_t addr,
			    void *data, size_t len,
			    struct zffs_area **last_area, u32_t *offset)
{
	int lock = k_mutex_lock(&zffs->lock, K_NO_WAIT);
	int rc = -EIO;

	area_list_foreach(area_list, area) {
		if (addr < area_size(area)) {
			u32_t read_bytes;

			read_bytes = min(area_size(area) - addr, len);
			rc = area_read(zffs->flash,
				       area_flash_addr(area, addr), data,
				       read_bytes);
			if (rc) {
				goto done;
			}

			data = (u8_t *)data + read_bytes;
			addr += read_bytes;
			len -= read_bytes;

			if (len == 0) {
				if (last_area) {
					*last_area = area;
				}

				if (offset) {
					*offset = addr;
				}

				rc = 0;
				goto done;
			}
		}

		addr -= area_size(area);
	}

done:
	if (!lock) {
		k_mutex_unlock(&zffs->lock);
	}
	return rc;
}

static int zffs_area_erase(struct zffs_data *zffs, struct zffs_area *area)
{
	struct zffs_disk_area disk_area = { 0 };
	int rc;

	rc = flash_area_erase(zffs->flash, area->offset, area->length);
	if (rc) {
		return rc;
	}

	area->erase_seq += 1;

	memcpy(disk_area.fs_name, ZFFS_NAME,
	       min(strlen(ZFFS_NAME), sizeof(disk_area.fs_name)));
	sys_put_le32(area->length, disk_area.length);
	sys_put_le16(area->erase_seq, disk_area.erase_seq);
	disk_area.ver = ZFFS_VER;

	rc = area_write(zffs->flash, area->offset, &disk_area,
			offsetof(struct zffs_disk_area, id));
	if (rc) {
		return rc;
	}

	area->full_id = ZFFS_AREA_ID_NONE;

	return 0;
}

static int area_compar(struct zffs_area **area_a, struct zffs_area **area_b)
{
	return (*area_a)->id - (*area_b)->id;
}

static void area_sort(struct zffs_area **area_a)
{
	struct zffs_area **area_b;

	while (*area_a) {
		area_b = area_a + 1;
		while (*area_b) {
			if (area_compar(area_a, area_b) > 0) {
				area_swap(area_a, area_b);
			}
			area_b++;
		}
		area_a++;
	}
}

int zffs_area_load(struct zffs_data *zffs, u32_t offset, u32_t *length)
{
	struct zffs_disk_area disk_area;
	struct zffs_area *area;
	int rc;

	rc = area_read(zffs->flash, offset, &disk_area,
		       sizeof(disk_area));
	if (rc) {
		return rc;
	}

	if (strncmp(ZFFS_NAME, disk_area.fs_name, sizeof(disk_area.fs_name))) {
		return -EFAULT;
	}

	if (disk_area.id != ZFFS_AREA_ID_NONE &&
	    crc16_ccitt(0, (const void *)&disk_area, sizeof(disk_area))) {
		return -EFAULT;
	}

	if (disk_area.ver > ZFFS_VER) {
		return -ENOTSUP;
	}

	zffs->base_area[zffs->area_num].offset = offset;
	zffs->base_area[zffs->area_num].length =
		sys_get_le32(disk_area.length);
	zffs->base_area[zffs->area_num].erase_seq =
		sys_get_le16(disk_area.erase_seq);
	zffs->base_area[zffs->area_num].gc_seq = disk_area.gc_seq;
	zffs->base_area[zffs->area_num].full_id = disk_area.id;

	if (disk_area.id != ZFFS_AREA_ID_NONE) {
		area = area_search(zffs, disk_area.id);
		if (area) {
			if (area->gc_seq > disk_area.gc_seq) {
				area->is_gc = 1;
				area_list_append(zffs->swap_area, area);
			} else {
				zffs->base_area[zffs->area_num].is_gc = 1;
				area_list_append(
					zffs->swap_area,
					&zffs->base_area[zffs->area_num]);
			}
		}
	} else {
		area_list_append(zffs->area,
				 &zffs->base_area[zffs->area_num]);
	}

	if (length) {
		*length = zffs->base_area[zffs->area_num].length;
	}

	zffs->area_num++;

	return 0;
}

int zffs_area_init(struct zffs_data *zffs, u32_t offset, u32_t length)
{
	int rc;

	zffs->base_area[zffs->area_num].offset = offset;
	zffs->base_area[zffs->area_num].length = length;
	zffs->base_area[zffs->area_num].erase_seq = 0;

	rc = zffs_area_erase(zffs, &zffs->base_area[zffs->area_num]);

	if (rc) {
		return rc;
	}

	area_list_append(zffs->area, &zffs->base_area[zffs->area_num]);

	zffs->area_num++;

	return 0;
}

int zffs_area_list_init(struct zffs_data *zffs,
			struct zffs_area **area_list, u8_t type)
{
	*area_list = NULL;
	area_foreach(zffs, area){
		if (area_type(area) == type) {
			area_list_append(area_list, area);
		}
	}

	if (*area_list == NULL) {
		struct zffs_area *area =
			area_new(zffs, type << ZFFS_AREA_ID_TYPE_BIT, 0);

		if (area == NULL) {
			return -ENOSPC;
		}

		area_list_append(area_list, area);

		return 0;
	}

	area_sort(area_list);

	return 0;
}

void zffs_area_addr_to_pointer(struct zffs_data *zffs, u32_t addr,
			       struct zffs_area_pointer *pointer)
{
	int lock = k_mutex_lock(&zffs->lock, K_NO_WAIT);

	struct zffs_area **area_list = *pointer->area_index;
	u32_t offset = 0;

	pointer->area = NULL;

	area_list_foreach(area_list, area){
		pointer->area = area;

		if (addr < area_size(area) + offset) {
			break;
		}

		offset += area_size(area);
	}

	pointer->offset = addr - offset;

	if (!lock) {
		k_mutex_unlock(&zffs->lock);
	}
}

u32_t zffs_area_pointer_to_addr(struct zffs_data *zffs,
				const struct zffs_area_pointer *pointer)
{
	int lock = k_mutex_lock(&zffs->lock, K_NO_WAIT);
	u32_t addr = pointer->offset;
	struct zffs_area **area_list = *pointer->area_index;

	area_list_foreach(area_list, area){
		if (area == pointer->area) {
			break;
		}

		addr += area_size(area);
	}

	if (!lock) {
		k_mutex_unlock(&zffs->lock);
	}

	return addr;
}

int zffs_area_random_read(struct zffs_data *zffs,
			  struct zffs_area **area_list, u32_t addr,
			  void *data, size_t len)
{
	return area_random_read(zffs, area_list, addr, data, len, NULL, NULL);
}

int zffs_area_read(struct zffs_data *zffs,
		   struct zffs_area_pointer *pointer, void *data, size_t len)
{
	int rc;

	if (pointer->area == NULL) {
		return -EIO;
	}

	if (len + pointer->offset <= area_size(pointer->area)) {
		rc = area_read(zffs->flash,
			       area_flash_addr(pointer->area, pointer->offset),
			       data, len);
		if (rc) {
			return rc;
		}

		pointer->offset += len;
		return 0;
	} else {
		int lock = k_mutex_lock(&zffs->lock, K_NO_WAIT);
		struct zffs_area **area_list = *pointer->area_index;

		rc = -ESPIPE;

		while (*area_list) {
			if (*area_list == pointer->area) {
				rc = area_random_read(
					zffs, area_list, pointer->offset, data,
					len, &pointer->area, &pointer->offset);
				break;
			}
			area_list++;
		}

		if (!lock) {
			k_mutex_unlock(&zffs->lock);
		}

		return rc;
	}
}

int zffs_area_write(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer, const void *data,
		    size_t len)
{
	struct zffs_area **area_list = *pointer->area_index;
	struct zffs_area *area = pointer->area;
	u32_t offset = pointer->offset;
	u32_t write_bytes;

	if (area == NULL) {
		return -ENOSPC;
	}

	while (len) {
		while (offset >= area_size(area)) {
			u8_t id = area->full_id;

			area = area_list_next(area_list, area);

			if (area == NULL) {
				area = area_new(zffs, id + 1, 0);

				if (area == NULL) {
					return -ENOSPC;
				}

				area_list_append(area_list, area);
			}

			offset -= area_size(area);
		}

		write_bytes = min(area_size(area) - offset, len);

		if (area_write(zffs->flash,
			       area_flash_addr(area, offset), data,
			       write_bytes)) {
			return -EIO;
		}

		data = (const u8_t *)data + write_bytes;
		offset += write_bytes;
		len -= write_bytes;

		pointer->area = area;
		pointer->offset = offset;
	}

	return 0;
}

int zffs_area_crc(struct zffs_data *zffs, struct zffs_area_pointer *pointer,
		  size_t len, u16_t *crc)
{
	u8_t buf[ZFFS_CONFIG_AREA_BUF_SIZE];
	u32_t readbytes;
	int rc;

	while (len) {
		readbytes = min(len, sizeof(buf));
		rc = zffs_area_read(zffs, pointer, buf, readbytes);
		if (rc) {
			return rc;
		}

		*crc = crc16_ccitt(*crc, buf, readbytes);
		len -= readbytes;
	}

	return 0;
}

int zffs_area_is_not_empty(struct zffs_data *zffs,
			   struct zffs_area_pointer *pointer, size_t len)
{
	u8_t buf[ZFFS_CONFIG_AREA_BUF_SIZE];
	u32_t readbytes;
	int rc;

	while (len) {
		readbytes = min(len, sizeof(buf));
		rc = zffs_area_read(zffs, pointer, buf, readbytes);
		if (rc) {
			return rc;
		}

		for (int i = 0; i < readbytes; i++) {
			if (buf[i] != 0xff) {
				return -ENOTEMPTY;
			}
		}

		len -= readbytes;
	}

	return 0;
}

int zffs_area_copy_crc(struct zffs_data *zffs,
		       struct zffs_area_pointer *from,
		       struct zffs_area_pointer *to, size_t len, u16_t *crc)
{
	u8_t buf[ZFFS_CONFIG_AREA_BUF_SIZE];
	u32_t readbytes;
	int rc;

	while (len) {
		readbytes = min(len, sizeof(buf));
		rc = zffs_area_read(zffs, from, buf, readbytes);
		if (rc) {
			return rc;
		}

		rc = zffs_area_write(zffs, to, buf, readbytes);
		if (rc) {
			return rc;
		}

		*crc = crc16_ccitt(*crc, buf, readbytes);
		len -= readbytes;
	}

	return 0;
}
