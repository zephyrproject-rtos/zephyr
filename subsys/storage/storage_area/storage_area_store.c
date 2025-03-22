/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/storage/storage_area/storage_area_store.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(storage_area_store, CONFIG_STORAGE_AREA_LOG_LEVEL);

/* record magic: chosen to be different from the erase-value (0x00 or 0xFF) */
#define SAS_MAGIC      0xF0
#define SAS_FILLVAL    0xFF

/* header size: record magic (1 BYTE) + wrapcnt (1 BYTE) + size (2 BYTE) */
#define SAS_HDRSIZE    4
#define SAS_CRCINIT    0
#define SAS_CRCSIZE    sizeof(uint32_t)
#define SAS_MINBUFSIZE 32

#define SAS_MIN(a, b)             (a < b ? a : b)
#define SAS_MAX(a, b)             (a < b ? b : a)
#define SAS_ALIGNUP(num, align)   (((num) + ((align) - 1)) & ~((align) - 1))
#define SAS_ALIGNDOWN(num, align) ((num) & ~((align) - 1))

static void sector_advance(const struct storage_area_store *store,
			   size_t *sector, size_t cnt)
{
	while (cnt > 0U) {
		(*sector)++;
		cnt--;
		if ((*sector) == store->sector_cnt) {
			*sector = 0U;
		}
	}
}

static ALWAYS_INLINE int
store_init_semaphore(const struct storage_area_store *store)
{
#ifdef CONFIG_STORAGE_AREA_STORE_SEMAPHORE
	return k_sem_init(&store->data->semaphore, 1, 1);
#else
	ARG_UNUSED(store);
	return 0;
#endif /* CONFIG_STORAGE_AREA_STORE_SEMAPHORE */
}

static ALWAYS_INLINE int
store_take_semaphore(const struct storage_area_store *store)
{
#ifdef CONFIG_STORAGE_AREA_STORE_SEMAPHORE
	return k_sem_take(&store->data->semaphore, K_FOREVER);
#else
	ARG_UNUSED(store);
	return 0;
#endif /* CONFIG_STORAGE_AREA_STORE_SEMAPHORE */
}

static ALWAYS_INLINE void
store_give_semaphore(const struct storage_area_store *store)
{
#ifdef CONFIG_STORAGE_AREA_STORE_SEMAPHORE
	(void)k_sem_give(&store->data->semaphore);
#endif /* CONFIG_STORAGE_AREA_STORE_SEMAPHORE */
}

static bool store_record_valid(const struct storage_area_record *record)
{
	const struct storage_area *area = record->store->area;
	const size_t crc_skip = record->store->crc_skip;
	const size_t recpos = record->sector * record->store->sector_size +
			      record->loc + SAS_HDRSIZE + crc_skip;
	uint32_t crc = SAS_CRCINIT;
	uint8_t buf[SAS_MAX(SAS_MINBUFSIZE, area->write_size)];
	struct storage_area_iovec rd = {
		.data = &buf,
	};
	size_t rdlen = record->size - crc_skip;
	sa_off_t rdoff = recpos;

	while (rdlen != 0U) {
		rd.len = SAS_MIN(sizeof(buf), rdlen);
		if (storage_area_readv(area, rdoff, &rd, 1U) != 0) {
			goto end;
		}

		crc = crc32_ieee_update(crc, buf, rd.len);
		rdlen -= rd.len;
		rdoff += rd.len;
	}

	rd.len = SAS_CRCSIZE;
	if (storage_area_readv(area, rdoff, &rd, 1U) != 0) {
		goto end;
	}

	if (crc != sys_get_le32(buf)) {
		goto end;
	}

	return true;
end:
	LOG_DBG("record at [%d-%d] has bad crc", record->sector, record->loc);
	return false;
}

static int store_record_next_in_sector(struct storage_area_record *record,
				       bool wrapcheck)
{
	const struct storage_area_store *store = record->store;
	const struct storage_area_store_data *data = store->data;
	const struct storage_area *area = store->area;
	const size_t secpos = record->sector * store->sector_size;
	bool check_crc = false;
	bool found = false;
	int rc = 0;

	if ((record->loc == 0U) && (store->sector_cookie != NULL) &&
	    (store->sector_cookie_size != 0U)) {
		const size_t cksz = store->sector_cookie_size;

		record->loc = SAS_ALIGNUP(cksz, area->write_size);
	}

	while (!found) {
		uint8_t header[SAS_HDRSIZE];
		struct storage_area_iovec rd = {
			.data = &header,
			.len = sizeof(header),
		};
		size_t rdpos = record->loc;
		sa_off_t rdoff;

		if (record->size != 0U) {
			rdpos += (SAS_HDRSIZE + record->size + SAS_CRCSIZE);
			rdpos = SAS_ALIGNUP(rdpos, area->write_size);
		}

		if (((data->sector == record->sector) && (data->loc <= rdpos)) ||
		    (rdpos == store->sector_size)) {
			record->loc = rdpos;
			record->size = 0U;
			break;
		}

		rdoff = secpos + rdpos;
		rc = storage_area_readv(area, rdoff, &rd, 1U);
		if (rc != 0) {
			break;
		}

		if (header[0] == SAS_FILLVAL) {
			record->loc = rdpos;
			record->size = 0U;
			break;
		}

		size_t rsize = (size_t)sys_get_le16(&header[2]);
		size_t avail =
			store->sector_size - rdpos - SAS_CRCSIZE - SAS_HDRSIZE;
		bool size_ok = ((rsize > 0U) && (rsize < avail));

		if (record->sector > data->sector) {
			header[1]++;
		}

		if (!wrapcheck) {
			header[1] = data->wrapcnt;
		}

		if ((header[0] == SAS_MAGIC) && (header[1] == data->wrapcnt) &&
		    (size_ok)) {
			found = true;
		}

		if ((found) && (check_crc)) {
			const struct storage_area_record trecord = {
				.store = record->store,
				.sector = record->sector,
				.loc = rdpos,
				.size = rsize,
			};

			found = store_record_valid(&trecord);
		}

		if (!found) {
			check_crc = true;
			record->size = 0U;
			record->loc += area->write_size;
			continue;
		}

		record->loc = rdpos;
		record->size = rsize;
	}

	if (rc != 0) {
		LOG_DBG("Bad read occurred");
	}

	if (!found) {
		rc = -ENOENT;
	}

	return rc;
}

static int store_add_cookie(const struct storage_area_store *store)
{
	if ((store->data->loc != 0) || (store->sector_cookie == NULL) ||
	    (store->sector_cookie_size == 0U)) {
		return 0;
	}

	const sa_off_t wroff = store->data->sector * store->sector_size;
	const size_t cksize = store->sector_cookie_size;
	const size_t align = store->area->write_size;
	const size_t flen = SAS_ALIGNUP(cksize, align) - cksize;
	uint8_t fill[align];
	struct storage_area_iovec wr[2] = {
		{
			.data = store->sector_cookie,
			.len = cksize,
		},
		{
			.data = fill,
			.len = flen,
		},
	};
	int rc;

	memset(fill, SAS_FILLVAL, sizeof(fill));
	rc = storage_area_writev(store->area, wroff, wr, 2U);
	if (rc != 0) {
		goto end;
	}

	store->data->loc = cksize + flen;
end:
	if (rc != 0) {
		LOG_DBG("add cookie failed for sector %x", store->data->sector);
	}

	return rc;
}

static int store_get_sector_cookie(const struct storage_area_store *store,
				   uint16_t sector, void *cookie, size_t cksz)
{
	const sa_off_t rdoff = sector * store->sector_size;
	struct storage_area_iovec rd = {
		.data = cookie,
		.len = SAS_MIN(cksz, store->sector_cookie_size),
	};

	return storage_area_readv(store->area, rdoff, &rd, 1U);
}

static int store_fill_sector(const struct storage_area_store *store)
{
	struct storage_area_store_data *data = store->data;
	const struct storage_area *area = store->area;
	const size_t secpos = data->sector * store->sector_size;
	uint8_t buf[SAS_MAX(SAS_MINBUFSIZE, area->write_size)];
	struct storage_area_iovec wr = {
		.data = buf,
	};
	int rc = 0;

	memset(buf, SAS_FILLVAL, sizeof(buf));
	while (data->loc < store->sector_size) {
		sa_off_t wroff = secpos + data->loc;

		wr.len = SAS_MIN(sizeof(buf), store->sector_size - data->loc);
		rc = storage_area_writev(area, wroff, &wr, 1U);
		if (rc != 0) {
			break;
		}

		data->loc += wr.len;
	}

	if (rc != 0) {
		LOG_DBG("failed to fill sector %d", data->sector);
	}

	return rc;
}

static int store_erase_block(const struct storage_area_store *store)
{
	const struct storage_area *area = store->area;
	const size_t erase_size = area->erase_size;
	struct storage_area_store_data *data = store->data;
	int rc = 0;

	if ((data->sector * store->sector_size) % erase_size != 0U) {
		goto end;
	}

	size_t sblock = (data->sector * store->sector_size) / erase_size;
	size_t bcnt = MAX(1U, store->sector_size / erase_size);

	rc = storage_area_erase(area, sblock, bcnt);

	if (rc != 0) {
		LOG_DBG("erase failed at block %d", sblock);
	}
end:
	return rc;
}

static int store_move_record(struct storage_area_record *record,
			     const struct storage_area_store_compact_cb *cb)
{
	if ((cb == NULL) || (cb->move == NULL) || (!cb->move(record))) {
		return 0;
	}

	if (!store_record_valid(record)) {
		LOG_DBG("invalid record, skipping move");
		return 0;
	}

	const struct storage_area_store *store = record->store;
	struct storage_area_store_data *data = store->data;
	const struct storage_area *area = store->area;
	const size_t sector_size = store->sector_size;
	const size_t align = area->write_size;
	const struct storage_area_record dest = {
		.store = record->store,
		.sector = data->sector,
		.loc = data->loc,
		.size = record->size};
	const size_t rdpos = record->sector * sector_size + record->loc;
	const size_t wrpos = data->sector * sector_size + data->loc;
	const size_t alsize =
		SAS_ALIGNUP(SAS_HDRSIZE + record->size + SAS_CRCSIZE, align);
	uint8_t buf[SAS_MAX(SAS_MINBUFSIZE, align)];
	struct storage_area_iovec rdwr = {
		.data = buf,
	};
	size_t start = 0U;
	int rc = 0;

	if ((store->sector_size - alsize) < data->loc) {
		rc = -ENOSPC;
		goto end;
	}

	while (start < alsize) {
		sa_off_t rdwroff = rdpos + start;

		rdwr.len = SAS_MIN(sizeof(buf), alsize - start);
		rc = storage_area_readv(area, rdwroff, &rdwr, 1U);
		if (rc != 0) {
			goto end;
		}

		if (start == 0U) {
			buf[1] = data->wrapcnt;
		}

		rdwroff = wrpos + start;
		rc = storage_area_writev(area, rdwroff, &rdwr, 1U);
		if (rc != 0) {
			goto end;
		}

		data->loc += rdwr.len;
		start += rdwr.len;
	}

	if (cb->move_cb != NULL) {
		cb->move_cb(record, &dest);
	}

end:
	if (rc != 0) {
		LOG_DBG("failed to move record at [%d-%d]", record->sector,
			record->loc);
	}

	return rc;
}

/* store advance for circular buffer without persistence (no records copy) */
static int store_advance_simple(const struct storage_area_store *store,
				const struct storage_area_store_compact_cb *cb)
{
	ARG_UNUSED(cb);

	const struct storage_area *area = store->area;
	struct storage_area_store_data *data = store->data;
	int rc = 0;

	if (STORAGE_AREA_FOVRWRITE(area)) {
		rc = store_fill_sector(store);
		if (rc != 0) {
			goto end;
		}
	}

	sector_advance(store, &data->sector, 1U);
	if (data->sector == 0U) {
		data->wrapcnt++;
	}

	data->loc = 0U;

	if ((!STORAGE_AREA_FOVRWRITE(area)) &&
	    (!STORAGE_AREA_AUTOERASE(area))) {
		rc = store_erase_block(store);
		if (rc != 0) {
			goto end;
		}
	}

	rc = store_add_cookie(store);
end:
	return rc;
}

/* store advance for circular buffer with persistence (with record copy) */
static int store_advance(const struct storage_area_store *store,
			 const struct storage_area_store_compact_cb *cb)
{
	const struct storage_area_store_data *data = store->data;
	int rc = store_advance_simple(store, NULL);

	if ((cb == NULL) || (cb->move == NULL) || (rc != 0)) {
		goto end;
	}

	const size_t erase_size = store->area->erase_size;
	const size_t sector_size = store->sector_size;

	if ((data->sector * sector_size) % erase_size != 0U) {
		goto end;
	}

	size_t scnt = MAX(1U, erase_size / sector_size);
	struct storage_area_record walk = {
		.store = (struct storage_area_store *)store,
		.sector = data->sector,
	};

	sector_advance(store, &walk.sector, store->spare_sectors);
	while (scnt > 0U) {
		walk.loc = 0U;
		walk.size = 0U;
		while (store_record_next_in_sector(&walk, true) == 0) {
			while (true) {
				rc = store_move_record(&walk, cb);
				if ((rc == 0) || (rc != -ENOSPC)) {
					break;
				}

				rc = store_advance_simple(store, NULL);
				if (rc != 0) {
					break;
				}
			}

			if (rc != 0) {
				break;
			}
		}

		sector_advance(store, &walk.sector, 1U);
		scnt--;
	}

end:
	return rc;
}

static void sector_reverse(const struct storage_area_store *store,
			   size_t *sector, size_t cnt)
{
	while (cnt > 0U) {
		if (*sector == 0U) {
			*sector = store->sector_cnt;
		}

		(*sector)--;
		cnt--;
	}
}

static void store_reverse(const struct storage_area_store *store)
{
	sector_reverse(store, &store->data->sector, 1U);
	store->data->loc = store->sector_size;
	if (store->data->sector == (store->sector_cnt - 1)) {
		store->data->wrapcnt--;
	}
}

static int store_recover(const struct storage_area_store *store,
			 const struct storage_area_store_compact_cb *cb)
{
	struct storage_area_store_data *data = store->data;

	if ((cb == NULL) || (cb->move == NULL)) {
		return 0;
	}

	const size_t erase_size = store->area->erase_size;
	const size_t sec_size = store->sector_size;
	const size_t dsector = data->sector;
	const size_t dloc = data->loc;
	const size_t dwrapcnt = data->wrapcnt;
	int rc = 0;

	for (size_t loop = 0; loop < 2; loop++) {
		size_t rscnt = 0U;

		while (((data->sector * sec_size) % erase_size) != 0U) {
			store_reverse(store);
			rscnt++;
		}

		store_reverse(store);
		rscnt++;
		if (loop != 0U) {
			/* some data was not moved, restart the move */
			rc = store_advance(store, cb);
			break;
		}

		struct storage_area_record walk = {
			.store = (struct storage_area_store *)store,
		};
		size_t mrcnt = 0U; /* cnt records that should be moved */
		size_t vrcnt = 0U; /* cnt records that are moved and valid */

		walk.sector = data->sector;
		sector_advance(store, &walk.sector, store->spare_sectors + 1U);
		for (size_t cnt = 0U; cnt < erase_size / sec_size; cnt++) {
			walk.loc = 0U;
			walk.size = 0U;
			while (store_record_next_in_sector(&walk, true) == 0) {
				if ((cb->move(&walk)) &&
				    (store_record_valid(&walk))) {
					mrcnt++;
				}
			}

			sector_advance(store, &walk.sector, 1U);
		}

		data->sector = dsector;
		data->loc = dloc;
		data->wrapcnt = dwrapcnt;

		if (mrcnt == 0U) {
			break;
		}

		walk.sector = data->sector;
		while (((walk.sector * sec_size) % erase_size) != 0U) {
			sector_reverse(store, &walk.sector, 1U);
		}

		for (size_t cnt = 0U; cnt < rscnt; cnt++) {
			walk.loc = 0U;
			walk.size = 0U;
			while (store_record_next_in_sector(&walk, true) == 0) {
				if (store_record_valid(&walk)) {
					vrcnt++;
				}
			}

			sector_advance(store, &walk.sector, 1U);
		}

		if (vrcnt >= mrcnt) {
			/* all records were moved */
			break;
		}
	}

	return rc;
}

static size_t store_iovec_size(const struct storage_area_iovec *iovec,
			       size_t iovcnt)
{
	size_t rv = 0U;

	for (size_t i = 0U; i < iovcnt; i++) {
		rv += iovec[i].len;
	}

	return rv;
}

static int store_writev(const struct storage_area_store *store,
			const struct storage_area_iovec *iovec, size_t iovcnt)
{
	struct storage_area_store_data *data = store->data;

	if (data->advance == NULL) {
		return -ENOTSUP;
	}

	const size_t len =
		SAS_HDRSIZE + store_iovec_size(iovec, iovcnt) + SAS_CRCSIZE;

	if ((store->sector_size - len) < data->loc) {
		return -ENOSPC;
	}

	const struct storage_area *area = store->area;
	const size_t secpos = data->sector * store->sector_size;
	const uint8_t erasevalue = STORAGE_AREA_ERASEVALUE(area);
	struct storage_area_iovec wr[iovcnt + 2];
	uint8_t header[SAS_HDRSIZE];
	uint8_t cbuf[SAS_CRCSIZE + SAS_ALIGNUP(len, area->write_size) - len];
	uint32_t crc = SAS_CRCINIT;
	size_t crc_skip = store->crc_skip;
	int rc;

	header[0] = SAS_MAGIC;
	header[1] = data->wrapcnt;
	sys_put_le16((uint16_t)store_iovec_size(iovec, iovcnt), &header[2]);
	wr[0].data = header;
	wr[0].len = sizeof(header);
	wr[iovcnt + 1].data = cbuf;
	wr[iovcnt + 1].len = sizeof(cbuf);

	for (size_t i = 1; i < (iovcnt + 1); i++) {
		wr[i].data = iovec[i - 1].data;
		wr[i].len = iovec[i - 1].len;

		if (crc_skip >= wr[i].len) {
			crc_skip -= wr[i].len;
			continue;
		}

		uint8_t *crc_data = (uint8_t *)wr[i].data;

		crc_data += crc_skip;
		crc = crc32_ieee_update(crc, crc_data, wr[i].len - crc_skip);
		crc_skip = 0U;
	}

	sys_put_le32(crc, cbuf);
	memset(&cbuf[SAS_CRCSIZE], erasevalue, sizeof(cbuf) - SAS_CRCSIZE);
	while (true) {
		sa_off_t wroff = secpos + data->loc;

		rc = storage_area_writev(area, wroff, wr, iovcnt + 2);
		if (rc == 0) {
			data->loc += SAS_ALIGNUP(len, area->write_size);
			break;
		}

		data->loc += area->write_size;
		if ((store->sector_size - len) < data->loc) {
			rc = -ENOSPC;
			break;
		}
	}

	return rc;
}

static bool store_valid(const struct storage_area_store *store)
{
	if ((store == NULL) || (store->data == NULL) || (store->area == NULL)) {
		LOG_DBG("Store definition is invalid");
		return false;
	}

	return true;
}

static bool store_ready(const struct storage_area_store *store)
{
	if ((!store_valid(store)) || (!store->data->ready)) {
		return false;
	}

	return true;
}

static bool store_config_valid(const struct storage_area_store *store)
{
	const struct storage_area *area = store->area;
	const size_t sa_size = area->erase_size * area->erase_blocks;
	const size_t st_size = store->sector_size * store->sector_cnt;

	if ((store->sector_size & (area->write_size - 1)) != 0U) {
		LOG_DBG("Sector size not a multiple of write block size");
		return false;
	}

	if (((area->erase_size & (store->sector_size - 1)) != 0U) &&
	    ((store->sector_size & (area->erase_size - 1)) != 0U)) {
		LOG_DBG("Sector incorrectly sized");
		return false;
	}

	if (sa_size < st_size) {
		LOG_DBG("Store does not fit area");
		return false;
	}

	return true;
}

static int store_init(const struct storage_area_store *store)
{
	const struct storage_area *area = store->area;
	struct storage_area_store_data *data = store->data;
	struct storage_area_record record = {
		.store = (struct storage_area_store *)store,
		.size = 0U,
	};
	uint8_t rd_wrapcnt;
	struct storage_area_iovec rd = {
		.data = &rd_wrapcnt,
		.len = 1U,
	};

	data->sector = store->sector_cnt;
	data->loc = store->sector_size;

	for (size_t i = 0U; i < store->sector_cnt; i++) {
		record.sector = i;
		record.loc = 0U;

		if (store_record_next_in_sector(&record, false) != 0) {
			continue;
		}

		const sa_off_t rdoff = i * store->sector_size + record.loc + 1U;

		if (storage_area_readv(area, rdoff, &rd, 1U) != 0) {
			continue;
		}

		if (data->sector > i) {
			data->wrapcnt = rd_wrapcnt;
		}

		if (rd_wrapcnt != data->wrapcnt) {
			break;
		}

		data->sector = i;
	}

	if (data->sector == store->sector_cnt) {
		data->sector--;
		goto end;
	}

	size_t loc = 0U;

	record.sector = data->sector;
	record.loc = 0U;
	record.size = 0U;
	while (store_record_next_in_sector(&record, true) == 0) {
		loc = record.loc +
		      SAS_ALIGNUP(SAS_HDRSIZE + record.size + SAS_CRCSIZE,
				  area->write_size);
	}

	data->loc = loc;
	data->ready = true;
end:
	return 0;
}

int storage_area_store_mount_ro(const struct storage_area_store *store)
{
	if ((!store_valid(store)) || (!store_config_valid(store))) {
		return -EINVAL;
	}

	if (store->data->ready) {
		return -EALREADY;
	}

	int rc = store_init_semaphore(store);

	if (rc != 0) {
		return rc;
	}

	(void)store_take_semaphore(store);
	rc = store_init(store);
	if (rc != 0) {
		goto end;
	}

	store->data->advance = NULL;
	store->data->ready = true;
end:
	store_give_semaphore(store);
	return rc;
}

int storage_area_store_mount_cb(const struct storage_area_store *store)
{
	if ((!store_valid(store)) || (!store_config_valid(store))) {
		return -EINVAL;
	}

	if (store->data->ready) {
		return -EALREADY;
	}

	int rc = store_init_semaphore(store);

	if (rc != 0) {
		return rc;
	}

	(void)store_take_semaphore(store);
	rc = store_init(store);
	if (rc != 0) {
		goto end;
	}

	if (!store->data->ready) {
		rc = store_advance_simple(store, NULL);
	}

	if (rc != 0) {
		goto end;
	}

	store->data->advance = store_advance_simple;
	store->data->ready = true;
end:
	store_give_semaphore(store);
	return rc;
}

int storage_area_store_mount(const struct storage_area_store *store,
			     const struct storage_area_store_compact_cb *cb)
{
	if ((!store_valid(store)) || (!store_config_valid(store))) {
		return -EINVAL;
	}

	const size_t spsize = store->spare_sectors * store->sector_size;

	if ((cb != NULL) && (cb->move != NULL) &&
	    (spsize < store->area->erase_size)) {
		LOG_DBG("Not enough spare sectors");
		return -EINVAL;
	}

	if (store->data->ready) {
		return -EALREADY;
	}

	int rc = store_init_semaphore(store);

	if (rc != 0) {
		return rc;
	}

	(void)store_take_semaphore(store);
	rc = store_init(store);
	if (rc != 0) {
		goto end;
	}

	if (!store->data->ready) {
		rc = store_advance_simple(store, NULL);
	} else {
		rc = store_recover(store, cb);
	}

	if (rc != 0) {
		goto end;
	}

	store->data->advance = store_advance;
	store->data->ready = true;
end:
	store_give_semaphore(store);
	return rc;
}

int storage_area_store_unmount(const struct storage_area_store *store)
{
	if (!store_valid(store)) {
		return -EINVAL;
	}

	if (store->data->ready) {
		store->data->advance = NULL;
		store->data->ready = false;
	}

	return 0;
}

int storage_area_store_advance(const struct storage_area_store *store)
{
	if (!store_ready(store)) {
		return -EINVAL;
	}

	if (store->data->advance == NULL) {
		return -ENOTSUP;
	}

	int rc;

	(void)store_take_semaphore(store);
	rc = store->data->advance(store, NULL);
	store_give_semaphore(store);
	return rc;
}

int storage_area_store_compact(const struct storage_area_store *store,
			       const struct storage_area_store_compact_cb *cb)
{
	if (!store_ready(store)) {
		return -EINVAL;
	}

	if (store->data->advance == NULL) {
		return -ENOTSUP;
	}

	int rc;

	(void)store_take_semaphore(store);
	rc = store->data->advance(store, cb);
	store_give_semaphore(store);
	return rc;
}

bool storage_area_record_valid(const struct storage_area_record *record)
{
	if (!store_ready(record->store)) {
		return -EINVAL;
	}

	return store_record_valid(record);
}

int storage_area_store_writev(const struct storage_area_store *store,
			      const struct storage_area_iovec *iovec,
			      size_t iovcnt)
{
	if (!store_ready(store)) {
		return -EINVAL;
	}

	int rc;

	(void)store_take_semaphore(store);
	rc = store_writev(store, iovec, iovcnt);
	store_give_semaphore(store);
	return rc;
}

int storage_area_store_write(const struct storage_area_store *store,
			     const void *data, size_t len)
{
	const struct storage_area_iovec iovec = {
		.data = (void *)data,
		.len = len,
	};

	return storage_area_store_writev(store, &iovec, 1U);
}

int storage_area_record_next(const struct storage_area_store *store,
			     struct storage_area_record *record)
{
	if (!store_valid(store)) {
		return -EINVAL;
	}

	if (record->store == NULL) {
		record->loc = 0U;
		record->size = 0U;
		record->sector = store->data->sector;
		sector_advance(store, &record->sector,
			       store->spare_sectors + 1U);
	}

	record->store = (struct storage_area_store *)store;

	int rc = 0;

	while (true) {
		rc = store_record_next_in_sector(record, true);
		if (rc != -ENOENT) {
			break;
		}

		if (record->sector == store->data->sector) {
			break;
		}

		sector_advance(store, &record->sector, 1U);
		record->loc = 0U;
		record->size = 0U;
	}

	return rc;
}

int storage_area_record_readv(const struct storage_area_record *record,
			      size_t start,
			      const struct storage_area_iovec *iovec,
			      size_t iovcnt)
{
	if ((record == NULL) || (record->store == NULL) ||
	    (!store_valid(record->store)) ||
	    (record->loc > record->store->sector_size) ||
	    (record->size > record->store->sector_size) ||
	    (record->size < (start + store_iovec_size(iovec, iovcnt)))) {
		return -EINVAL;
	}

	const struct storage_area_store *store = record->store;
	const struct storage_area *area = store->area;
	const size_t rdpos = record->sector * store->sector_size + record->loc +
			     start + SAS_HDRSIZE;

	return storage_area_readv(area, (sa_off_t)rdpos, iovec, iovcnt);
}

int storage_area_record_read(const struct storage_area_record *record,
			     size_t start, void *data, size_t len)
{
	const struct storage_area_iovec iovec = {
		.data = data,
		.len = len,
	};

	return storage_area_record_readv(record, start, &iovec, 1U);
}

int storage_area_record_update(const struct storage_area_record *record,
			       void *data, size_t len)
{
	const struct storage_area *area = record->store->area;

	if ((!STORAGE_AREA_FOVRWRITE(area)) && (!STORAGE_AREA_LOVRWRITE(area))) {
		return -ENOTSUP;
	}

	if ((!storage_area_record_valid(record)) ||
	    (record->store->crc_skip < len)) {
		return -EINVAL;
	}

	const size_t align = area->write_size;
	const size_t secpos = record->sector * record->store->sector_size;
	size_t rpos = record->loc + SAS_HDRSIZE;
	size_t apos = SAS_ALIGNDOWN(rpos, align);
	uint8_t *data8 = (uint8_t *)data;
	uint8_t buf[align];
	int rc = 0;

	while (len != 0U) {
		const size_t modlen = SAS_MIN(len, align - (rpos - apos));
		struct storage_area_iovec iovec = {
			.data = buf,
			.len = sizeof(buf),
		};
		const sa_off_t rdwroff = (sa_off_t)(secpos + apos);

		rc = storage_area_readv(area, rdwroff, &iovec, 1U);
		if (rc != 0) {
			break;
		}

		memcpy(buf + (rpos - apos), data8, modlen);
		rc = storage_area_writev(area, rdwroff, &iovec, 1U);
		if (rc != 0) {
			break;
		}

		len -= modlen;
		rpos += modlen;
		apos += align;
	}

	if (rc != 0) {
		LOG_DBG("failed to update record at [%d-%d]", record->sector,
			record->loc);
	}

	return rc;
}

int storage_area_store_get_sector_cookie(const struct storage_area_store *store,
					 size_t sector, void *cookie,
					 size_t cksz)
{
	if ((!store_valid(store)) || (store->sector_cookie == NULL) ||
	    (store->sector_cookie_size == 0)) {
		return -EINVAL;
	}

	return store_get_sector_cookie(store, sector, cookie, cksz);
}

int storage_area_store_wipe(const struct storage_area_store *store)
{
	if (!store_valid(store)) {
		return -EINVAL;
	}

	if (store->data->ready) {
		return -EINVAL;
	}

	const struct storage_area *area = store->area;
	uint8_t wbuf[area->write_size];
	const struct storage_area_iovec wr = {
		.data = wbuf,
		.len = sizeof(wbuf)
	};
	sa_off_t wroff = 0;
	int rc;

	rc = storage_area_erase(area, 0, area->erase_blocks);

	if (rc != 0) {
		goto end;
	}

	while (wroff < (area->erase_size * area->erase_blocks)) {
		rc = storage_area_writev(area, wroff, &wr, 1U);
		if (rc != 0) {
			break;
		}

		wroff += sizeof(wbuf);
	}
end:
	return rc;
}
