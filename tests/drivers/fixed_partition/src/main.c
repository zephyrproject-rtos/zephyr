/*
 * Copyright (c) 2021 Laczen
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <drivers/fixed_partition.h>

/**
 * @brief Test fixed partition read/write
 */
void test_rw_on(const struct fxp_info *fxp)
{
	int rc;
	off_t off, fxp_off;
	size_t sec_size = 4096;
	uint8_t wd[256];
	uint8_t rd[256];

	/* First erase the area so it's ready for use. */
	rc = flash_erase(fxp->device, fxp->off, fxp->size);
	zassert_true(rc == 0, "hal_flash_erase() fail [rc: %d]", rc);

	(void)memset(wd, 0xa5, sizeof(wd));

	/* write stuff to beginning of every sector */
	off = 0;
	while (off < fxp->size) {
		rc = fxp_write(fxp, off, wd, sizeof(wd));
		zassert_true(rc == 0, "fxp_write() fail [rc: %d]", rc);

		/* read it back via hal_flash_read() */
		fxp_off = fxp->off + off;
		rc = flash_read(fxp->device, fxp_off, rd, sizeof(rd));
		zassert_true(rc == 0, "hal_flash_read() fail [rc: %d]", rc);

		rc = memcmp(wd, rd, sizeof(wd));
		zassert_true(rc == 0, "read data != write data");

		/* write stuff to end of area via hal_flash_write */
		fxp_off = fxp->off + off + sec_size - sizeof(wd);
		rc = flash_write(fxp->device, fxp_off, wd, sizeof(wd));
		zassert_true(rc == 0, "hal_flash_write() fail [rc: %d]", rc);

		/* and read it back */
		(void)memset(rd, 0, sizeof(rd));
		rc = fxp_read(fxp, off + sec_size - sizeof(rd), rd, sizeof(rd));
		zassert_true(rc == 0, "fxp_read() fail [rc: %d]", rc);

		rc = memcmp(wd, rd, sizeof(rd));
		zassert_true(rc == 0, "read data != write data");

		off += sec_size;
	}

	/* erase it */
	rc = fxp_erase(fxp, 0, fxp->size);
	zassert_true(rc == 0, "fxp_erase() fail");

	/* should read back ff all throughout*/
	(void)memset(wd, 0xff, sizeof(wd));
	for (off = 0; off < fxp->size; off += sizeof(rd)) {
		rc = fxp_read(fxp, off, rd, sizeof(rd));
		zassert_true(rc == 0, "fxp_read() fail");

		rc = memcmp(wd, rd, sizeof(rd));
		zassert_true(rc == 0, "area not erased");
	}
}

void test_rw(void)
{
	test_rw_on(FIXED_PARTITION_GET(image_0));
	test_rw_on(FIXED_PARTITION_GET(mcuboot));
}

void test_get_parameters(void)
{
	const struct fxp_info *fxp = FIXED_PARTITION_GET(image_0);
	const struct flash_parameters *param1, *param2;

	param1 = fxp_get_parameters(fxp);
	param2 = flash_get_parameters(fxp->device);

	zassert_equal(param1->write_block_size, param2->write_block_size,
		      "write-block-size differs");
	zassert_equal(param1->erase_value, param2->erase_value,
		      "erase-value differs");

}

static bool get_page_cb(const struct flash_pages_info *info, void *data)
{
	uint32_t *counter = (uint32_t *)data;

	(*counter) += 1;
	return true;
}

void test_foreach_page_on(const struct fxp_info *fxp)
{
	uint32_t page_counter = 0;

	fxp_page_foreach(fxp, get_page_cb, &page_counter);
	zassert_true(page_counter, "No pages found [%d]", page_counter);
}

void test_foreach_page(void)
{
	test_foreach_page_on(FIXED_PARTITION_GET(image_0));
	test_foreach_page_on(FIXED_PARTITION_GET(mcuboot));
}

void test_get_pages_info_on(const struct fxp_info *fxp)
{
	int rc;
	uint32_t pages;
	struct flash_pages_info info;
	uint32_t first_page_size;

	pages = fxp_get_page_count(fxp);
	zassert_true(pages, "No pages found [%d]", pages);

	rc = fxp_get_page_info_by_offs(fxp, 0, &info);
	zassert_false(rc, "get_page_info_by_offs returned [%d]", rc);
	zassert_equal(info.start_offset, 0, "wrong start offset");
	zassert_equal(info.index, 0, "wrong index [%d]", info.index);

	first_page_size = info.size;
	rc = fxp_get_page_info_by_offs(fxp, first_page_size, &info);
	zassert_false(rc, "get_page_info_by_offs returned [%d]", rc);
	zassert_equal(info.start_offset, first_page_size, "wrong start offset");
	zassert_equal(info.index, 1, "wrong index");

	/* request invalid (negative) offset */
	rc = fxp_get_page_info_by_offs(fxp, -1, &info);
	zassert_true(rc, "invalid get_page_info_by_offs [%d]", rc);

	/* request invalid (too large) offset */
	rc = fxp_get_page_info_by_offs(fxp, fxp->size, &info);
	zassert_true(rc, "invalid get_page_info_by_offs [%d]", rc);

	rc = fxp_get_page_info_by_idx(fxp, 0, &info);
	zassert_false(rc, "get_page_info_by_offs returned [%d]", rc);
	zassert_equal(info.start_offset, 0, "wrong start offset");
	zassert_equal(info.index, 0, "wrong index");

	first_page_size = info.size;
	rc = fxp_get_page_info_by_idx(fxp, 1, &info);
	zassert_false(rc, "get_page_info_by_offs returned [%d]", rc);
	zassert_equal(info.start_offset, first_page_size, "wrong start offset");
	zassert_equal(info.index, 1, "wrong index");

	/* request invalid (too large) index*/
	rc = fxp_get_page_info_by_idx(fxp, pages, &info);
	zassert_true(rc, "invalid get_page_info_by_idx [%d]", rc);
}

void test_get_pages_info(void)
{
	test_get_pages_info_on(FIXED_PARTITION_GET(image_0));
	test_get_pages_info_on(FIXED_PARTITION_GET(mcuboot));
}
void test_main(void)
{
	ztest_test_suite(test_fixed_partition,
			 ztest_unit_test(test_rw),
			 ztest_unit_test(test_get_parameters),
			 ztest_unit_test(test_foreach_page),
			 ztest_unit_test(test_get_pages_info)
	);
	ztest_run_test_suite(test_fixed_partition);
}
