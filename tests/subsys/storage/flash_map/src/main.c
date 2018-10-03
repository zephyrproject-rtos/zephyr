/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <flash.h>
#include <flash_map.h>



#define FLASH_AREA_BOOTLOADER                    0
#define FLASH_AREA_IMAGE_0                       1
#define FLASH_AREA_IMAGE_1                       2
#define FLASH_AREA_IMAGE_SCRATCH                 3

extern int flash_map_entries;
struct flash_sector fs_sectors[256];

/**
 * @brief Test flash_area_to_sectors()
 */
void test_flash_area_to_sectors(void)
{
	const struct flash_area *fa;
	u32_t sec_cnt;
	int i;
	int rc;
	off_t off;
	u8_t wd[256];
	u8_t rd[256];
	struct device *flash_dev;

	rc = flash_area_open(FLASH_AREA_IMAGE_1, &fa);
	zassert_true(rc == 0, "flash_area_open() fail");

	/* First erase the area so it's ready for use. */
	flash_dev = device_get_binding(FLASH_DEV_NAME);

	rc = flash_write_protection_set(flash_dev, false);
	zassert_false(rc, "failed to disable flash write protection");

	rc = flash_erase(flash_dev, fa->fa_off, fa->fa_size);
	zassert_true(rc == 0, "flash area erase fail");

	rc = flash_write_protection_set(flash_dev, true);
	zassert_false(rc, "failed to enable flash write protection");

	(void)memset(wd, 0xa5, sizeof(wd));

	sec_cnt = ARRAY_SIZE(fs_sectors);
	rc = flash_area_get_sectors(FLASH_AREA_IMAGE_1, &sec_cnt, fs_sectors);
	zassert_true(rc == 0, "flash_area_get_sectors failed");

	/* write stuff to beginning of every sector */
	off = 0;
	for (i = 0; i < sec_cnt; i++) {
		rc = flash_area_write(fa, off, wd, sizeof(wd));
		zassert_true(rc == 0, "flash_area_write() fail");

		/* read it back via hal_flash_Read() */
		rc = flash_read(flash_dev, fa->fa_off + off, rd, sizeof(rd));
		zassert_true(rc == 0, "hal_flash_read() fail");

		rc = memcmp(wd, rd, sizeof(wd));
		zassert_true(rc == 0, "read data != write data");

		(void) flash_write_protection_set(flash_dev, false);
		/* write stuff to end of area */
		rc = flash_write(flash_dev, fa->fa_off + off +
					    fs_sectors[i].fs_size - sizeof(wd),
				 wd, sizeof(wd));
		zassert_true(rc == 0, "hal_flash_write() fail");

		/* and read it back */
		(void)memset(rd, 0, sizeof(rd));
		rc = flash_area_read(fa, off + fs_sectors[i].fs_size -
					 sizeof(rd),
				     rd, sizeof(rd));
		zassert_true(rc == 0, "hal_flash_read() fail");

		rc = memcmp(wd, rd, sizeof(rd));
		zassert_true(rc == 0, "read data != write data");

		off += fs_sectors[i].fs_size;
	}

	/* erase it */
	rc = flash_area_erase(fa, 0, fa->fa_size);
	zassert_true(rc == 0, "read data != write data");

	/* should read back ff all throughout*/
	(void)memset(wd, 0xff, sizeof(wd));
	for (off = 0; off < fa->fa_size; off += sizeof(rd)) {
		rc = flash_area_read(fa, off, rd, sizeof(rd));
		zassert_true(rc == 0, "hal_flash_read() fail");

		rc = memcmp(wd, rd, sizeof(rd));
		zassert_true(rc == 0, "area not erased");
	}

}

void test_main(void)
{
	ztest_test_suite(test_flash_map,
			 ztest_unit_test(test_flash_area_to_sectors));
	ztest_run_test_suite(test_flash_map);
}
