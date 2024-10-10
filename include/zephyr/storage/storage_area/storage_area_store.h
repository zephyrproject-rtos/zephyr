/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for storing data on a storage area
 *
 * The storage area store enables storage of data record on top of a
 * storage area. The record format is:
 *	magic | data size | data | crc32
 * with:
 *	magic (2 byte): second byte is a wrapcnt variable that is increased
 *                      each time storage area is wrapped around,
 *      data size (2 byte): little endian uint16_t,
 *	crc32 (4 byte): little endian uint32_t, calculated over data
 *
 * The storage area is divided into constant sized sectors that are either a
 * whole divider or a multiple of the storage area erase blocks. Sectors are
 * smaller then 64kB. When the underlying storage area has erase-blocks that
 * are larger then 64kB the erase-blocks are split up in multiple sectors.
 * Records are written consecutively to a sector. Each record is aligned to
 * the write size of the storage area. Records can be written to a sector
 * until space is exhausted (write return `-ENOSPC`).
 *
 * To create space for new record the storage area store can be "advanced" or
 * "compacted". The "advance" routine will simply take into use a next sector.
 * The "compact" routine will move certain records to the front of the storage
 * area store. The "compact" routine will use a callback routine "move" to
 * determine if a record should be kept.
 *
 * The first byte of the data can be updated (if the storage area allows it).
 * Updating the first byte will make the data invalid (wrong crc), however it
 * can be used to mark a record as invalid (without checking the crc). So if
 * you would like to use this feature treat the first byte as a "valid" byte.
 */

#ifndef ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_STORE_H_
#define ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_STORE_H_

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/storage_area/storage_area.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Storage_area_store interface
 * @defgroup storage_area_store_interface Storage_area_store interface
 * @ingroup storage_apis
 * @{
 */

struct storage_area_record;

struct storage_area_store_compact_cb {
	/* move function is used to evaluate if record should
	 * be copied to maintain persistence.
	 */
	bool (*move)(const struct storage_area_record *record);
	/* move_cb is called after moving a record and can e.g.
	 * be used to update a hash table of records.
	 */
	void (*move_cb)(const struct storage_area_record *orig,
			const struct storage_area_record *dest);
};

struct storage_area_store_data {
#ifdef CONFIG_STORAGE_AREA_STORE_SEMAPHORE
	struct k_sem semaphore;
#endif
	struct storage_area_store_compact_cb cb;
	bool ready;
	size_t sector; /* current sector */
	size_t loc;    /* current location */
	uint8_t wrapcnt;
};

struct storage_area_store {
	const struct storage_area *area;
	void *sector_cookie;
	size_t sector_cookie_size;
	size_t sector_size;
	size_t sector_cnt;
	size_t spare_sectors;
	/* wrap_cb is called when the storage area wraps around,
	 * and can e.g. be  used to update the sector_cookie
	 */
	void (*wrap_cb)(const struct storage_area_store *store);
	struct storage_area_store_data *data;
};

struct storage_area_record {
	struct storage_area_store *store;
	size_t sector;
	size_t loc;
	size_t size;
};

/**
 * @brief	Mount storage area store.
 *
 * @param store	storage area store.
 * @param cb	routines used during compacting
 *
 * @retval	0 on success else negative errno code.
 */
int storage_area_store_mount(const struct storage_area_store *store,
			     const struct storage_area_store_compact_cb *cb);

/**
 * @brief	Unmount storage area store.
 *
 * @param store	storage area store.
 *
 * @retval	0 on success else negative errno code.
 */
int storage_area_store_unmount(const struct storage_area_store *store);

/**
 * @brief	Wipe storage area store (storage area needs to be unmounted).
 *
 * @param store	storage area store.
 *
 * @retval	0 on success else negative errno code.
 */
int storage_area_store_wipe(const struct storage_area_store *store);

/**
 * @brief	Write data to storage area store.
 *
 * @param store	storage area store.
 * @param ch	data chunk (see storage_area_chunk).
 * @param cnt	chunk count.
 *
 * @retval	0 on success else negative errno code.
 */
int storage_area_store_write(const struct storage_area_store *store,
			     const struct storage_area_chunk *ch, size_t cnt);

/**
 * @brief	Direct write data to storage area store.
 *
 * @param store	storage area store.
 * @param data	data.
 * @param len	write size.
 *
 * @retval	0 on success else negative errno code.
 */
int storage_area_store_dwrite(const struct storage_area_store *store,
			      const void *data, size_t len);

/**
 * @brief	Advance storage area store.
 *		Take a new sector into use. This might erase old data.
 *		REMARK: This can be a slow operation.
 *
 * @param store	storage area store.
 *
 * @retval	0 on success else negative errno code.
 */
int storage_area_store_advance(const struct storage_area_store *store);

/**
 * @brief	Compact storage area store.
 *		Reduce the used storage area space by removing obsolete
 *              record and moving records that need keeping.
 *		REMARK: This can be a slow operation.
 *
 * @param store	storage area store.
 *
 * @retval	0 on success else negative errno code.
 */
int storage_area_store_compact(const struct storage_area_store *store);

/**
 * @brief	 Retrieve the next record of the store. To get the first
 *		 record set the record.store to NULL.
 *
 * @param store	 storage area store.
 * @param record returned storage area record.
 *
 * @retval	 0 on success else negative errno code.
 */
int storage_area_record_next(const struct storage_area_store *store,
			     struct storage_area_record *record);

/**
 * @brief	 Validate a record (crc checks out)
 *
 * @param record storage area record.
 *
 * @retval	 true if record is valid else false.
 */
bool storage_area_record_valid(const struct storage_area_record *record);

/**
 * @brief	 Read data from a record
 *
 * @param record storage area record.
 * @param start  offset in the record (data).
 * @param ch	 data chunk.
 * @param cnt	 chunk count.
 *
 * @retval	 0 on success else negative errno code.
 */
int storage_area_record_read(const struct storage_area_record *record,
			     size_t start, const struct storage_area_chunk *ch,
			     size_t cnt);

/**
 * @brief	 Direct data read from a record
 *
 * @param record storage area record.
 * @param start  offset in the record (data).
 * @param data   data.
 * @param len	 read size.
 *
 * @retval	 0 on success else negative errno code.
 */
int storage_area_record_dread(const struct storage_area_record *record,
			      size_t start, void *data, size_t len);

/**
 * @brief	 Update the first byte of the record data. This is only possible
 *		 if the storage area supports multiple writes. The allowed
 *		 update data may be limited (e.g. only toggle bits from 1 to 0).
 *		 This can be used to invalidate records.
 *
 * @param record storage area record.
 * @param update new byte data to write.
 * @retval	 0 on success else negative errno code.
 */
int storage_area_record_fbupdate(const struct storage_area_record *record,
				 uint8_t update);

/**
 * @brief	 Get the cookie of a sector
 *
 * @param store  storage area store.
 * @param sector the sector number.
 * @param cookie destination.
 * @param cksz   cookie size (size of the destination buffer).
 *
 * @retval	 0 on success else negative errno code.
 */
int storage_area_store_get_sector_cookie(const struct storage_area_store *store,
					 size_t sector, void *cookie,
					 size_t cksz);

#define storage_area_store(_area, _cookie, _cookie_size, _sector_size,          \
			   _sector_cnt, _spare_sectors, _wrap_cb, _data)        \
	{                                                                       \
		.area = _area, .sector_cookie = _cookie,                        \
		.sector_cookie_size = _cookie_size,                             \
		.sector_size = _sector_size, .sector_cnt = _sector_cnt,         \
		.spare_sectors = _spare_sectors, .wrap_cb = _wrap_cb,           \
		.data = _data,                                                  \
	}

#define create_storage_area_store(_name, _area, _cookie, _cookie_size,          \
				  _sector_size, _sector_cnt, _spare_sectors,    \
				  _move, _move_cb, _wrap_cb)                    \
	static struct storage_area_store_data                                   \
		_storage_area_store_##_name##_data = {                          \
			.cb.move = _move,                                       \
			.cb.move_cb = _move_cb,                                 \
			.ready = false,                                         \
	};                                                                      \
	static const struct storage_area_store _storage_area_store_##_name =    \
		storage_area_store(_area, _cookie, _cookie_size, _sector_size,  \
				   _sector_cnt, _spare_sectors, _wrap_cb,       \
				   &(_storage_area_store_##_name##_data));

#define get_storage_area_store(_name)                                           \
	(struct storage_area_store *)&(_storage_area_store_##_name)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_STORE_H_ */
