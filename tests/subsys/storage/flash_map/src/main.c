/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

extern int flash_map_entries;
struct flash_sector fs_sectors[256];

void test_flash_area_disabled_device(void)
{
	const struct flash_area *fa;
	int rc;

	/* Test that attempting to open a disabled flash area fails */
	rc = flash_area_open(FLASH_AREA_ID(disabled_a), &fa);
	zassert_equal(rc, -ENODEV, "Open did not fail");
	rc = flash_area_open(FLASH_AREA_ID(disabled_b), &fa);
	zassert_equal(rc, -ENODEV, "Open did not fail");
}

/**
 * @brief Test flash_area_get_sectors()
 */
void test_flash_area_get_sectors(void)
{
	const struct flash_area *fa;
	uint32_t sec_cnt;
	int i;
	int rc;
	off_t off;
	uint8_t wd[256];
	uint8_t rd[256];
	const struct device *flash_dev;
	const struct device *flash_dev_a = FLASH_AREA_DEVICE(image_1);

	rc = flash_area_open(FLASH_AREA_ID(image_1), &fa);
	zassert_true(rc == 0, "flash_area_open() fail");

	/* First erase the area so it's ready for use. */
	flash_dev = flash_area_get_device(fa);

	/* Device obtained by label should match the one from fa object */
	zassert_equal(flash_dev, flash_dev_a, "Device for image_1 do not match");

	rc = flash_erase(flash_dev, fa->fa_off, fa->fa_size);
	zassert_true(rc == 0, "flash area erase fail");

	(void)memset(wd, 0xa5, sizeof(wd));

	sec_cnt = ARRAY_SIZE(fs_sectors);
	rc = flash_area_get_sectors(FLASH_AREA_ID(image_1), &sec_cnt,
				    fs_sectors);
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

	flash_area_close(fa);
}

void test_flash_area_check_int_sha256(void)
{
	/* echo $'0123456789abcdef\nfedcba98765432' > tst.sha
	 * hexdump tst.sha
	 */
	uint8_t tst_vec[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
			      0x38, 0x39, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
			      0x0a, 0x66, 0x65, 0x64, 0x63, 0x62, 0x61, 0x39,
			      0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x0a };
	/* sha256sum tst.sha */
	uint8_t tst_sha[] = { 0x28, 0xf1, 0x6e, 0xea, 0xc3, 0xea, 0x89, 0x8d,
			      0x80, 0x9e, 0x98, 0xeb, 0x09, 0x49, 0x98, 0x08,
			      0x40, 0x69, 0x43, 0xa6, 0xef, 0xe1, 0xa3, 0xf9,
			      0x3d, 0xdf, 0x15, 0x9e, 0x06, 0xf8, 0xdd, 0xbd };

	const struct flash_area *fa;
	struct flash_area_check fac = { NULL, 0, -1, NULL, 0 };
	uint8_t buffer[16];
	int rc;

	rc = flash_area_open(FLASH_AREA_ID(image_1), &fa);
	zassert_true(rc == 0, "flash_area_open() fail, error %d\n", rc);
	rc = flash_area_erase(fa, 0, fa->fa_size);
	zassert_true(rc == 0, "Flash erase failure (%d), error %d\n", rc);
	rc = flash_area_write(fa, 0, tst_vec, sizeof(tst_vec));
	zassert_true(rc == 0, "Flash img write, error %d\n", rc);

	rc = flash_area_check_int_sha256(NULL, NULL);
	zassert_true(rc == -EINVAL, "Flash area check int 256 params 1, 2\n");
	rc = flash_area_check_int_sha256(NULL, &fac);
	zassert_true(rc == -EINVAL, "Flash area check int 256 params 2\n");
	rc = flash_area_check_int_sha256(fa, NULL);
	zassert_true(rc == -EINVAL, "Flash area check int 256 params 1\n");

	rc = flash_area_check_int_sha256(fa, &fac);
	zassert_true(rc == -EINVAL, "Flash area check int 256 fac match\n");
	fac.match = tst_sha;
	rc = flash_area_check_int_sha256(fa, &fac);
	zassert_true(rc == -EINVAL, "Flash area check int 256 fac clen\n");
	fac.clen = sizeof(tst_vec);
	rc = flash_area_check_int_sha256(fa, &fac);
	zassert_true(rc == -EINVAL, "Flash area check int 256 fac off\n");
	fac.off = 0;
	rc = flash_area_check_int_sha256(fa, &fac);
	zassert_true(rc == -EINVAL, "Flash area check int 256 fac rbuf\n");
	fac.rbuf = buffer;
	rc = flash_area_check_int_sha256(fa, &fac);
	zassert_true(rc == -EINVAL, "Flash area check int 256 fac rblen\n");
	fac.rblen = sizeof(buffer);

	rc = flash_area_check_int_sha256(fa, &fac);
	zassert_true(rc == 0, "Flash area check int 256 OK, error %d\n", rc);
	tst_sha[0] = 0x00;
	rc = flash_area_check_int_sha256(fa, &fac);
	zassert_false(rc == 0, "Flash area check int 256 wrong sha\n");

	flash_area_close(fa);
}

void test_flash_area_erased_val(void)
{
	const struct flash_parameters *param;
	const struct flash_area *fa;
	uint8_t val;
	int rc;

	rc = flash_area_open(FLASH_AREA_ID(image_1), &fa);
	zassert_true(rc == 0, "flash_area_open() fail");

	val = flash_area_erased_val(fa);

	param = flash_get_parameters(fa->fa_dev);

	zassert_equal(param->erase_value, val,
		      "value different than the flash erase value");

	flash_area_close(fa);
}

void test_main(void)
{
	ztest_test_suite(test_flash_map,
			ztest_unit_test(test_flash_area_disabled_device),
			 ztest_unit_test(test_flash_area_erased_val),
			 ztest_unit_test(test_flash_area_get_sectors),
			 ztest_unit_test(test_flash_area_check_int_sha256)
			);
	ztest_run_test_suite(test_flash_map);
}
