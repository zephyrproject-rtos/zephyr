/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Test for the storage_area_store API */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/storage_area/storage_area_store.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sas_test);

#ifdef CONFIG_STORAGE_AREA_FLASH
#include <zephyr/storage/storage_area/storage_area_flash.h>
#define FLASH_AREA_NODE		DT_NODELABEL(storage_partition)
#define FLASH_AREA_OFFSET	DT_REG_ADDR(FLASH_AREA_NODE)
#define FLASH_AREA_DEVICE							\
	DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(FLASH_AREA_NODE))
#define FLASH_AREA_XIP		FLASH_AREA_OFFSET +				\
	DT_REG_ADDR(DT_MTD_FROM_FIXED_PARTITION(FLASH_AREA_NODE))
#define AREA_SIZE		DT_REG_SIZE(FLASH_AREA_NODE)
#define AREA_ERASE_SIZE		4096
#define AREA_WRITE_SIZE		8

const static struct storage_area_flash area = flash_storage_area(
	FLASH_AREA_DEVICE, FLASH_AREA_OFFSET, FLASH_AREA_XIP, AREA_WRITE_SIZE,
	AREA_ERASE_SIZE, AREA_SIZE, SA_PROP_LOVRWRITE);
#endif /* CONFIG_STORAGE_AREA_FLASH */

#ifdef CONFIG_STORAGE_AREA_EEPROM
#include <zephyr/storage/storage_area/storage_area_eeprom.h>
#define EEPROM_NODE		DT_ALIAS(eeprom_0)
#define EEPROM_AREA_DEVICE	DEVICE_DT_GET(EEPROM_NODE)
#define AREA_SIZE		DT_PROP(EEPROM_NODE, size)
#define AREA_ERASE_SIZE		4096
#define AREA_WRITE_SIZE		4

const static struct storage_area_eeprom area = eeprom_storage_area(
	EEPROM_AREA_DEVICE, 0U, AREA_WRITE_SIZE, AREA_ERASE_SIZE, AREA_SIZE, 0);
#endif /* CONFIG_STORAGE_AREA_EEPROM */

#ifdef CONFIG_STORAGE_AREA_RAM
#include <zephyr/storage/storage_area/storage_area_ram.h>
#define RAM_NODE		DT_NODELABEL(storage_sram)
#define AREA_SIZE		DT_REG_SIZE(RAM_NODE)
#define AREA_ERASE_SIZE		4096
#define AREA_WRITE_SIZE		4
const static struct storage_area_ram area = ram_storage_area(
	DT_REG_ADDR(RAM_NODE), AREA_WRITE_SIZE, AREA_ERASE_SIZE, AREA_SIZE, 0);
#endif /* CONFIG_STORAGE_AREA_RAM */

#ifdef CONFIG_STORAGE_AREA_DISK
#include <zephyr/storage/storage_area/storage_area_disk.h>
#define DISK_NODE	DT_NODELABEL(ramdisk0)
#define DISK_NAME	DT_PROP(DISK_NODE, disk_name)
#define DISK_SSIZE	DT_PROP(DISK_NODE, sector_size)
#define DISK_SCNT	DT_PROP(DISK_NODE, sector_count)
#define AREA_SIZE	DISK_SCNT * DISK_SSIZE / 2
#define AREA_ERASE_SIZE	4096
#define AREA_WRITE_SIZE	DISK_SSIZE
const static struct storage_area_disk area = disk_storage_area(
	DISK_NAME, DISK_SCNT / 2, DISK_SSIZE, AREA_WRITE_SIZE, AREA_ERASE_SIZE,
	AREA_SIZE, 0);
#endif /* CONFIG_STORAGE_AREA_DISK */

static const char cookie[] = "!NVS";

bool move(const struct storage_area_record *record)
{
	uint8_t nsz;

	if (storage_area_record_read(record, 0U, &nsz, sizeof(nsz)) != 0) {
		return false;
	}

	if (record->size == (nsz + sizeof(nsz))) {
		return false;
	}

	char name[nsz];

	if (storage_area_record_read(record, sizeof(nsz), &name, nsz) != 0) {
		return false;
	}

	struct storage_area_record walk = {
		.store = record->store,
		.sector = record->sector,
		.loc = record->loc,
		.size = record->size,
	};

	while (true) {
		int rc;

		rc = storage_area_record_next(record->store, &walk);
		if (rc != 0) {
			break;
		}

		rc = storage_area_record_read(&walk, 0U, &nsz, sizeof(nsz));
		if (rc != 0) {
			break;
		}

		if (nsz != sizeof(name)) {
			continue;
		}

		char wname[nsz];

		rc = storage_area_record_read(&walk, sizeof(nsz), wname, nsz);
		if (rc != 0) {
			break;
		}

		if (memcmp(name, wname, nsz) != 0) {
			continue;
		}

		break;
	}

	if (walk.size == 0U) {
		return true;
	}

	return false;
}

void move_cb(const struct storage_area_record *src,
	     const struct storage_area_record *dst)
{
	LOG_INF("Moved %d-%d to %d-%d", src->sector, src->loc, dst->sector,
		dst->loc);
}

#define SECTOR_SIZE 4096
create_storage_area_store(test, &area.area, (void *)cookie, sizeof(cookie),
			  SECTOR_SIZE, AREA_SIZE / SECTOR_SIZE,
			  AREA_ERASE_SIZE / SECTOR_SIZE, 0U, move, move_cb,
			  NULL);

static void *storage_area_store_api_setup(void)
{
	return NULL;
}

static void storage_area_store_api_before(void *fixture)
{
	ARG_UNUSED(fixture);

	int rc = storage_area_erase(&area.area, 0, 1);

	zassert_equal(rc, 0, "erase returned [%d]", rc);
}

void storage_area_store_report_state(char *tag,
				     const struct storage_area_store *store)
{
	struct storage_area_store_data *data = store->data;

	LOG_INF("%s: sector: %d - loc:%d - wrapcnt:%d", tag, data->sector,
		data->loc, data->wrapcnt);
}

int write_data(const struct storage_area_store *store, char *name,
	       uint32_t value)
{
	uint8_t nsz = strlen(name);
	struct storage_area_iovec wr[] = {
		{
			.data = &nsz,
			.len = sizeof(nsz),
		},
		{
			.data = name,
			.len = nsz,
		},
		{
			.data = &value,
			.len = sizeof(uint32_t),
		}
	};

	return storage_area_store_writev(store, wr, ARRAY_SIZE(wr));
}

int read_data(const struct storage_area_store *store, char *name,
	      uint32_t *value)
{
	uint8_t nsz = strlen(name);
	struct storage_area_record walk, match;
	int rc = 0;

	walk.store = NULL;
	while (storage_area_record_next(store, &walk) == 0) {
		uint8_t nlen;

		rc = storage_area_record_read(&walk, 0U, &nlen, sizeof(nlen));
		if (rc != 0) {
			break;
		}

		if (nlen != nsz) {
			continue;
		}

		char rdname[nlen];

		rc = storage_area_record_read(&walk, 1U, rdname, nlen);
		if (rc != 0) {
			break;
		}

		if (memcmp(name, rdname, nlen) != 0) {
			continue;
		}

		memcpy(&match, &walk, sizeof(walk));
	}

	if ((rc != 0) || (match.store != walk.store) ||
	    ((match.size - nsz - 1U) != sizeof(uint32_t))) {
		rc = -ENOENT;
		goto end;
	}

	rc = storage_area_record_read(&match, 1U + nsz, value, sizeof(uint32_t));
end:
	return rc;
}

ZTEST_USER(storage_area_store_api, test_store)
{
	struct storage_area_store *store = get_storage_area_store(test);
	uint32_t wvalue1, wvalue2, rvalue;
	int rc;

	rc = storage_area_store_mount(store, NULL);
	zassert_equal(rc, 0, "mount returned [%d]", rc);
	storage_area_store_report_state("Mount", store);

	wvalue1 = 0U;
	rc = write_data(store, "data1", wvalue1);
	zassert_equal(rc, 0, "write returned [%d]", rc);
	storage_area_store_report_state("Write", store);

	rvalue = 0xFFFF;
	rc = read_data(store, "data1", &rvalue);
	zassert_equal(rc, 0, "read returned [%d]", rc);
	zassert_equal(rvalue, wvalue1, "bad data read");

	rc = storage_area_store_unmount(store);
	zassert_equal(rc, 0, "unmount returned [%d]", rc);
	storage_area_store_report_state("Unmount", store);

	rc = storage_area_store_mount(store, NULL);
	zassert_equal(rc, 0, "mount returned [%d]", rc);
	storage_area_store_report_state("Mount", store);

	rvalue = 0xFFFF;
	rc = read_data(store, "data1", &rvalue);
	zassert_equal(rc, 0, "read returned [%d]", rc);
	zassert_equal(rvalue, wvalue1, "bad data read");

	wvalue2 = 0x00C0FFEE;
	rc = write_data(store, "mydata/test", wvalue2);
	zassert_equal(rc, 0, "write returned [%d]", rc);
	storage_area_store_report_state("Write", store);

	rvalue = 0xFFFF;
	rc = read_data(store, "mydata/test", &rvalue);
	zassert_equal(rc, 0, "read returned [%d]", rc);
	zassert_equal(rvalue, wvalue2, "bad data read");

	wvalue2 = 0U;
	for (int i = 0; i < store->sector_cnt; i++) {
		while (write_data(store, "data2", wvalue2) == 0) {

		}
		storage_area_store_report_state("Write", store);
		rc = storage_area_store_compact(store);
		zassert_equal(rc, 0, "compact returned [%d]", rc);
		storage_area_store_report_state("Compact", store);
	}

	rvalue = 0xFFFF;
	rc = read_data(store, "data1", &rvalue);
	zassert_equal(rc, 0, "read returned [%d]", rc);
	zassert_equal(rvalue, wvalue1, "bad data read");

	rc = storage_area_store_unmount(store);
	zassert_equal(rc, 0, "unmount returned [%d]", rc);
	storage_area_store_report_state("Unmount", store);

	size_t wroff;
	uint8_t bad[STORAGE_AREA_WRITESIZE(store->area)];

	wroff = store->data->sector * store->sector_size + store->data->loc;
	wroff -= STORAGE_AREA_WRITESIZE(store->area);
	memset(bad, 0x0, sizeof(bad));
	rc = storage_area_write(store->area, wroff, bad, sizeof(bad));
	zassert_equal(rc, 0, "dprog failed [%d]", rc);

	rc = storage_area_store_mount(store, NULL);
	zassert_equal(rc, 0, "mount returned [%d]", rc);
	storage_area_store_report_state("Mount", store);

	rvalue = 0xFFFF;
	rc = read_data(store, "data1", &rvalue);
	zassert_equal(rc, 0, "read returned [%d]", rc);
	zassert_equal(rvalue, wvalue1, "bad data read");
}

ZTEST_SUITE(storage_area_store_api, NULL, storage_area_store_api_setup,
	    storage_area_store_api_before, NULL, NULL);
