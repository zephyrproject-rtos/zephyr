/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Test for the storage_area API */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sa_api_test);

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
#define AREA_WRITE_SIZE		512

STORAGE_AREA_FLASH_RW_DEFINE(test, FLASH_AREA_DEVICE, FLASH_AREA_OFFSET,
	FLASH_AREA_XIP, AREA_WRITE_SIZE, AREA_ERASE_SIZE, AREA_SIZE,
	STORAGE_AREA_PROP_LOVRWRITE);
#endif /* CONFIG_STORAGE_AREA_FLASH */

#ifdef CONFIG_STORAGE_AREA_EEPROM
#include <zephyr/storage/storage_area/storage_area_eeprom.h>
#define EEPROM_NODE		DT_ALIAS(eeprom_0)
#define EEPROM_AREA_DEVICE	DEVICE_DT_GET(EEPROM_NODE)
#define AREA_SIZE		DT_PROP(EEPROM_NODE, size)
#define AREA_ERASE_SIZE		1024
#define AREA_WRITE_SIZE		4

STORAGE_AREA_EEPROM_RW_DEFINE(test, EEPROM_AREA_DEVICE, 0U, AREA_WRITE_SIZE,
	AREA_ERASE_SIZE, AREA_SIZE, 0);
#endif /* CONFIG_STORAGE_AREA_EEPROM */

#ifdef CONFIG_STORAGE_AREA_RAM
#include <zephyr/storage/storage_area/storage_area_ram.h>
#define RAM_NODE	DT_NODELABEL(storage_sram)
#define AREA_SIZE	DT_REG_SIZE(RAM_NODE)
#define AREA_ERASE_SIZE	4096
#define AREA_WRITE_SIZE	4

STORAGE_AREA_RAM_RW_DEFINE(test, DT_REG_ADDR(RAM_NODE), AREA_WRITE_SIZE,
	AREA_ERASE_SIZE, AREA_SIZE, 0);
#endif /* CONFIG_STORAGE_AREA_RAM */

#ifdef CONFIG_STORAGE_AREA_DISK
#include <zephyr/storage/storage_area/storage_area_disk.h>
#define DISK_NODE	DT_NODELABEL(ramdisk0)
#define DISK_NAME	DT_PROP(DISK_NODE, disk_name)
#define DISK_SSIZE	DT_PROP(DISK_NODE, sector_size)
#define DISK_SCNT	DT_PROP(DISK_NODE, sector_count)
#define AREA_SIZE	DISK_SCNT * DISK_SSIZE / 2
#define AREA_ERASE_SIZE	DISK_SSIZE
#define AREA_WRITE_SIZE	DISK_SSIZE

STORAGE_AREA_DISK_RW_DEFINE(test, DISK_NAME, DISK_SCNT / 2, DISK_SSIZE,
	AREA_WRITE_SIZE, AREA_ERASE_SIZE, AREA_SIZE, 0);
#endif /* CONFIG_STORAGE_AREA_DISK */

static void *storage_area_api_setup(void)
{
	return NULL;
}

static void storage_area_api_before(void *fixture)
{
	ARG_UNUSED(fixture);

	const struct storage_area *sa = GET_STORAGE_AREA(test);
	int rc = storage_area_erase(sa, 0, 1);

	zassert_ok(rc, "erase returned [%d]", rc);
}

ZTEST_USER(storage_area_api, test_read_write_simple)
{
	const struct storage_area *sa = GET_STORAGE_AREA(test);
	uint8_t wr[STORAGE_AREA_WRITESIZE(sa)];
	uint8_t rd[STORAGE_AREA_WRITESIZE(sa)];
	struct storage_area_iovec rdvec = {
		.data = &rd,
		.len = sizeof(rd),
	};
	struct storage_area_iovec wrvec = {
		.data = &wr,
		.len = sizeof(wr),
	};
	int rc;


	memset(wr, 'T', sizeof(wr));
	memset(rd, 0, sizeof(rd));

	rc = storage_area_writev(sa, 0U, &wrvec, 1U);
	zassert_ok(rc, "prog returned [%d]", rc);

	rc = storage_area_readv(sa, 0U, &rdvec, 1U);
	zassert_ok(rc, "read returned [%d]", rc);

	zassert_mem_equal(rd, wr, sizeof(wr), 0, "data mismatch");
}

ZTEST_USER(storage_area_api, test_read_write_direct)
{
	const struct storage_area *sa = GET_STORAGE_AREA(test);
	uint8_t wr[STORAGE_AREA_WRITESIZE(sa)];
	uint8_t rd[STORAGE_AREA_WRITESIZE(sa)];
	int rc;

	memset(wr, 'T', sizeof(wr));
	memset(rd, 0, sizeof(rd));

	rc = storage_area_write(sa, 0U, wr, sizeof(wr));
	zassert_ok(rc, "prog returned [%d]", rc);

	rc = storage_area_read(sa, 0U, rd, sizeof(rd));
	zassert_ok(rc, "read returned [%d]", rc);

	zassert_mem_equal(rd, wr, sizeof(wr), 0, "data mismatch");
}

ZTEST_USER(storage_area_api, test_read_write_blocks)
{
	const struct storage_area *sa = GET_STORAGE_AREA(test);
	uint8_t magic = 0xA0;
	uint8_t wr[STORAGE_AREA_WRITESIZE(sa)];
	uint8_t rd[STORAGE_AREA_WRITESIZE(sa)];
	uint8_t fill[STORAGE_AREA_WRITESIZE(sa) - 1];
	struct storage_area_iovec rdvec[] = {
		{
			.data = (void *)&magic,
			.len = sizeof(magic),
		},
		{
			.data = (void *)&rd,
			.len = sizeof(rd),
		},
	};
	struct storage_area_iovec wrvec[] = {
		{
			.data = (void *)&magic,
			.len = sizeof(magic),
		},
		{
			.data = (void *)&wr,
			.len = sizeof(wr),
		},
		{
			.data = (void *)&fill,
			.len = sizeof(fill),
		},
	};
	int rc;

	memset(fill, 0xff, sizeof(fill));
	memset(wr, 'T', sizeof(wr));
	memset(rd, 0, sizeof(rd));

	rc = storage_area_writev(sa, 0U, wrvec, ARRAY_SIZE(wrvec));
	zassert_ok(rc, "prog returned [%d]", rc);

	rc = storage_area_readv(sa, 0U, rdvec, ARRAY_SIZE(rdvec));
	zassert_ok(rc, "read returned [%d]", rc);

	zassert_equal(magic, 0xA0, "magic has changed");
	zassert_mem_equal(rd, wr, sizeof(wr), 0, "data mismatch");
}

ZTEST_USER(storage_area_api, test_ioctl)
{
	const struct storage_area *sa = GET_STORAGE_AREA(test);
	uintptr_t xip;

	int rc = storage_area_ioctl(sa, STORAGE_AREA_IOCTL_XIPADDRESS, &xip);

	if ((IS_ENABLED(CONFIG_STORAGE_AREA_DISK)) ||
	    (IS_ENABLED(CONFIG_STORAGE_AREA_EEPROM))) {
		zassert_equal(rc, -ENOTSUP, "xip returned invalid address");
	} else {
		zassert_ok(rc, "xip returned no address");
	}
}

ZTEST_SUITE(storage_area_api, NULL, storage_area_api_setup,
	    storage_area_api_before, NULL, NULL);
