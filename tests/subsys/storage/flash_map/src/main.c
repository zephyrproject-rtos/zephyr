/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>

extern int flash_map_entries;
struct flash_sector fs_sectors[256];

/**
 * @brief Test flash_area_get_sectors()
 */
void test_flash_area_get_sectors(void)
{
	const struct flash_area *fa = FLASH_AREA(image_1);
	uint32_t sec_cnt;
	int i;
	int rc;
	off_t off;
	uint8_t wd[256];
	uint8_t rd[256];

	zassert_true(fa != NULL, "Flash area pointer nULL");

	rc = flash_erase(fa->fa_dev, fa->fa_off, fa->fa_size);
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
		rc = flash_read(fa->fa_dev, fa->fa_off + off, rd, sizeof(rd));
		zassert_true(rc == 0, "hal_flash_read() fail");

		rc = memcmp(wd, rd, sizeof(wd));
		zassert_true(rc == 0, "read data != write data");

		/* write stuff to end of area */
		rc = flash_write(fa->fa_dev, fa->fa_off + off +
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

void test_flash_area_page_info(void)
{
	const struct flash_area *fa0 = FLASH_AREA(image_1);
	const struct flash_area *fa1 = FLASH_AREA(image_scratch);
	const struct flash_area *fa2 = FLASH_AREA(storage);
	struct flash_pages_info pi_dev;
	struct flash_pages_info pi_fa;
	struct flash_pages_info pi_fa1;
	int ret, i;
	ssize_t count, count1;
	char val = flash_area_erased_val(fa1);
	char test_start[] = "Hello world";
	char test_end[] = "ByBye world";
	char buffer[256];

	/* Expect areas to be on the same device, and follow each other */
	zassert_equal(fa0->fa_dev, fa1->fa_dev, "fa0 and fa1 not on the same device");
	zassert_equal(fa1->fa_dev, fa2->fa_dev, "fa1 and fa2 not on the same device");
	zassert_equal(fa1->fa_off, fa0->fa_off + fa0->fa_size, "fa0 is not adjacent to fa1");
	zassert_equal(fa2->fa_off, fa1->fa_off + fa1->fa_size, "fa1 is not adjacent to fa2");

	flash_area_erase(fa0, 0, fa0->fa_size);
	flash_area_erase(fa1, 0, fa1->fa_size);
	flash_area_erase(fa2, 0, fa2->fa_size);

	/* Get page info for flash area and raw device */
	ret = flash_get_page_info_by_offs(fa0->fa_dev, fa1->fa_off, &pi_dev);
	zassert_true(ret == 0, "Failed to get flash area page info");
	ret = flash_area_get_page_info_by_offs(fa1, 0, &pi_fa);
	zassert_true(ret == 0, "Failed to get flash page info");

	/* Write test pattern at the beginning of flash area */
	ret = flash_area_write(fa1, pi_fa.start_offset, test_start, sizeof(test_start));
	zassert_true(ret == 0, "Failed to write the test pattern");

	/* Check if direct read from raw device confirms written data */
	ret = flash_read(fa1->fa_dev, pi_dev.start_offset, buffer, sizeof(test_start));
	zassert_true(ret == 0, "Failed to read the test pattern");
	zassert_equal(0, memcmp(buffer, test_start, sizeof(test_start)), "Data does not match");

	/* Check if not crossed to previous, raw, page */
	ret = flash_read(fa1->fa_dev, pi_dev.start_offset - sizeof(test_start),  buffer,
				sizeof(test_start));
	zassert_true(ret == 0, "Failed to read previous page");
	for (ret = 0, i = 0; i < sizeof(test_start); ++i) {
		if (buffer[i] != val) {
			ret = 1;
			break;
		}
	}
	zassert_equal(ret, 0, "Previous page has been overwritten");

	/* Write test pattern at the end of flash area */
	ret = flash_get_page_info_by_offs(fa1->fa_dev, fa1->fa_off + fa1->fa_size - 1, &pi_dev);
	zassert_true(ret == 0, "Failed to get flash area page info");
	ret = flash_area_get_page_info_by_offs(fa1, fa1->fa_size - 1, &pi_fa);
	zassert_true(ret == 0, "Failed to get flash page info");

	ret = flash_area_write(fa1, pi_fa.start_offset + pi_fa.size - sizeof(test_end), test_end,
		sizeof(test_end));
	zassert_true(ret == 0, "Failed to write the end test pattern");

	/* Check if direct read from raw device confirms written data */
	ret = flash_read(fa1->fa_dev, pi_dev.start_offset + pi_dev.size - sizeof(test_end), buffer,
		sizeof(test_end));
	zassert_true(ret == 0, "Failed to read test pattern");
	zassert_equal(0, memcmp(buffer, test_end, sizeof(test_end)), "Data does not match");

	/* Check if not crossed to page after the flash area, raw, page */
	ret = flash_read(fa1->fa_dev, pi_dev.start_offset + pi_dev.size,  buffer, sizeof(test_end));
	zassert_true(ret == 0, "Failed to read page after the flash area");
	for (ret = 0, i = 0; i < sizeof(test_end); ++i) {
		if (buffer[i] != val) {
			ret = 1;
			break;
		}
	}
	zassert_equal(ret, 0, "Page following the flash area has been overwritten");

	/* Test flash_area_get_page_by_info_idx */
	memset(&pi_fa, 0xff, sizeof(pi_fa));
	memset(&pi_fa1, 0xff, sizeof(pi_fa1));

	/* Fail to get negative index */
	ret = flash_area_get_page_info_by_idx(fa1, -1, &pi_fa);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL when index -1, got %d", ret);
	/* Provided flash_pages_info should not get modified */
	zassert_equal(0, memcmp(&pi_fa, &pi_fa1, sizeof(pi_fa)),
		"Unexpected flash_pages_info modification");

	/* Fail to get negative offset */
	ret = flash_area_get_page_info_by_offs(fa1, -1, &pi_fa);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL to get offset  -1, got %d", ret);
	/* Provided flash_pages_info should not get modified */
	zassert_equal(0, memcmp(&pi_fa, &pi_fa1, sizeof(pi_fa)),
		"Unexpected flash_pages_info modification");

	/* Get first page info by index */
	memset(&pi_fa, 0xff, sizeof(pi_fa));
	ret = flash_area_get_page_info_by_idx(fa1, 0, &pi_fa);
	zassert_equal(ret, 0, "Expected success to get index 0, got %d", ret);
	ret = flash_get_page_info_by_offs(fa1->fa_dev, fa1->fa_off, &pi_dev);
	zassert_true(ret == 0, "Failed to get flash area page info");
	zassert_equal(pi_fa.index, 0, "Unexpected index value");
	zassert_equal(pi_fa.size, pi_dev.size, "Unexpected size value");
	zassert_equal(pi_fa.start_offset + fa1->fa_off, pi_dev.start_offset,
		"Unexpected offset value");

	/* Get first page info by offset */
	memset(&pi_fa, 0xff, sizeof(pi_fa));
	ret = flash_area_get_page_info_by_offs(fa1, 0, &pi_fa);
	zassert_equal(ret, 0, "Expected success to get offset 0, got %d", ret);
	ret = flash_get_page_info_by_offs(fa1->fa_dev, fa1->fa_off, &pi_dev);
	zassert_true(ret == 0, "Failed to get flash area page info");
	zassert_equal(pi_fa.index, 0, "Unexpected index value");
	zassert_equal(pi_fa.size, pi_dev.size, "Unexpected size value");
	zassert_equal(pi_fa.start_offset + fa1->fa_off, pi_dev.start_offset,
		"Unexpected offset value");

	/* Get number of pages */
	ret = flash_area_get_page_info_by_offs(fa1, fa1->fa_size - 1, &pi_fa);
	zassert_equal(ret, 0, "Expected success");
	count = flash_area_get_page_count(fa1);
	zassert_true(count > 0, "Expected page count > 0");
	zassert_equal(count, pi_fa.index + 1, "Expected page count to be last page index + 1");
	/* Check against flash API */
	ret = flash_get_page_info_by_offs(fa1->fa_dev, fa1->fa_off, &pi_dev);
	zassert_equal(ret, 0, "Expected success");
	count1 = pi_dev.index;
	ret = flash_get_page_info_by_offs(fa1->fa_dev, fa1->fa_off + fa1->fa_size - 1, &pi_dev);
	zassert_equal(ret, 0, "Expected success");
	zassert_equal(count, pi_dev.index + 1 - count1, "Expected count of pages to be equal");

	/* Get last page by index */
	ret = flash_area_get_page_info_by_idx(fa1, count - 1, &pi_fa);
	zassert_equal(ret, 0, "Expected success");
	zassert_equal(pi_fa.index, count - 1, "Expected page index to match requested");
	zassert_equal(pi_fa.start_offset + pi_fa.size, fa1->fa_size,
		"Last page offset + size does not match area size");

	/* Get last page by offset */
	ret = flash_area_get_page_info_by_offs(fa1, fa1->fa_size - 1, &pi_fa);
	zassert_equal(ret, 0, "Expected success");
	zassert_equal(pi_fa.index, count - 1, "Expected page index to match number of pages - 1");
	zassert_equal(pi_fa.start_offset, fa1->fa_size - pi_fa.size,
		"Last page offset does not match (area size - size of page)");

	/* Get next page from 0 */
	ret = flash_area_get_page_info_by_idx(fa1, 0, &pi_fa);
	zassert_equal(ret, 0, "Expected success");
	pi_fa1 = pi_fa;
	ret = flash_area_get_next_page_info(fa1, &pi_fa1, false);
	zassert_equal(ret, 0, "Expected success");
	zassert_equal(pi_fa.index + 1, pi_fa1.index, "Should get next index");
	zassert_equal(pi_fa.start_offset + pi_fa.size, pi_fa1.start_offset,
		"Should get next page offset");

	/* wrap == true should not affect pages different than last */
	pi_fa1 = pi_fa;
	ret = flash_area_get_next_page_info(fa1, &pi_fa1, true);
	zassert_equal(ret, 0, "Expected success");
	zassert_equal(pi_fa.index + 1, pi_fa1.index, "Should get next index");
	zassert_equal(pi_fa.start_offset + pi_fa.size, pi_fa1.start_offset,
		"Should get next page offset");

	/* Get next page before last */
	ret = flash_area_get_page_info_by_idx(fa1, count - 2, &pi_fa);
	zassert_equal(ret, 0, "Expected success");
	pi_fa1 = pi_fa;
	ret = flash_area_get_next_page_info(fa1, &pi_fa1, false);
	zassert_equal(ret, 0, "Expected success");
	zassert_equal(pi_fa.index + 1, pi_fa1.index, "Should get next index");
	zassert_equal(pi_fa.start_offset + pi_fa.size, pi_fa1.start_offset,
		"Should get next page offset");

	/* Get next page from last, no wrap */
	ret = flash_area_get_page_info_by_idx(fa1, count - 1, &pi_fa);
	zassert_equal(ret, 0, "Expected success");
	pi_fa1 = pi_fa;
	ret = flash_area_get_next_page_info(fa1, &pi_fa1, false);
	zassert_equal(ret, -EINVAL, "Expected failure");
	zassert_equal(memcmp(&pi_fa, &pi_fa1, sizeof(pi_fa)), 0,
		"Structure should not get modified");

	/* Get next page from last, wrap */
	ret = flash_area_get_page_info_by_idx(fa1, count - 1, &pi_fa);
	zassert_equal(ret, 0, "Expected success");
	pi_fa1 = pi_fa;
	ret = flash_area_get_next_page_info(fa1, &pi_fa1, true);
	zassert_equal(ret, 0, "Expected success");
	zassert_equal(pi_fa1.index, 0, "Expected first page");
	zassert_equal(pi_fa1.start_offset, 0, "Expected first page offset");
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

	const struct flash_area *fa = FLASH_AREA(image_1);
	struct flash_area_check fac = { NULL, 0, -1, NULL, 0 };
	uint8_t buffer[16];
	int rc;

	zassert_true(fa != NULL, "Flash area pointer NULL\n");
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
}

void test_flash_area_erased_val(void)
{
	const struct flash_parameters *param;
	const struct flash_area *fa = FLASH_AREA(image_1);
	uint8_t val;

	zassert_true(fa != NULL, "Flash area pointer NULL");

	val = flash_area_erased_val(fa);

	param = flash_get_parameters(fa->fa_dev);

	zassert_equal(param->erase_value, val,
		      "value different than the flash erase value");
}

void test_main(void)
{
	ztest_test_suite(test_flash_map,
			 ztest_unit_test(test_flash_area_erased_val),
			 ztest_unit_test(test_flash_area_get_sectors),
			 ztest_unit_test(test_flash_area_check_int_sha256),
			 ztest_unit_test(test_flash_area_page_info)
			);
	ztest_run_test_suite(test_flash_map);
}
