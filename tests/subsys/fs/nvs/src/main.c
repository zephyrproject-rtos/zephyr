/*
 * Copyright (c) 2018 Laczen
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <stdio.h>
#include <zephyr.h>
#include <board.h>
#include <device.h>
#include <string.h>
#include <errno.h>


#include "nvs/nvs.h"

#define STORAGE_MAGIC 0x4e565354 /* hex for "NVST" */
#define NVS_SECTOR_SIZE FLASH_ERASE_BLOCK_SIZE /* Multiple of FLASH_PAGE_SIZE */
#define NVS_SECTOR_COUNT 2 /* At least 2 sectors */
#define NVS_STORAGE_OFFSET FLASH_AREA_STORAGE_OFFSET /* Start address of the
						      * filesystem in flash
						      */
#define NVS_MAX_ELEM_SIZE 256 /* Largest item that can be stored */

static struct nvs_fs fs = {
	.sector_size = NVS_SECTOR_SIZE,
	.sector_count = NVS_SECTOR_COUNT,
	.offset = NVS_STORAGE_OFFSET,
	.max_len = NVS_MAX_ELEM_SIZE,
};

void nvs_test_clear(void)
{
	int rc;

	rc = nvs_clear(&fs);
	zassert_true(rc == 0, "nvs_clear call failure");
}

void nvs_test_init(void)
{
	int rc;

	/* initialize the file system */
	rc = nvs_init(&fs, FLASH_DEV_NAME, STORAGE_MAGIC);
	zassert_true(rc == 0, "init call failure");
}

void nvs_test_write(void)
{
	int rc;
	char buf[8];

	/* write data */
	strcpy(buf, "DATA");
	rc = nvs_write(&fs, 0, &buf, strlen(buf) + 1);
	zassert_true(rc == strlen(buf) + 1, "write call failure");
	strcpy(buf, "DAT");
	rc = nvs_write(&fs, 0, &buf, strlen(buf) + 1);
	zassert_true(rc == strlen(buf) + 1, "write call failure");
	strcpy(buf, "DA");
	rc = nvs_write(&fs, 0, &buf, strlen(buf) + 1);
	zassert_true(rc == strlen(buf) + 1, "write call failure");
	strcpy(buf, "D");
	rc = nvs_write(&fs, 0, &buf, strlen(buf) + 1);
	zassert_true(rc == strlen(buf) + 1, "write call failure");
}

void nvs_test_read(void)
{
	int rc;
	char buf[8];

	/* read data */
	rc = nvs_read(&fs, 0, &buf, 8);
	zassert_false(rc < 0, "read call failure");
	rc = strcmp(buf, "D");
	zassert_true(rc == 0, "read value failure");
}

void nvs_test_read_hist(void)
{
	int rc;
	char buf[8];

	/* read history data */
	rc = nvs_read_hist(&fs, 0, &buf, 8, 0);
	zassert_false(rc < 0, "read_hist call failure");
	rc = strcmp(buf, "D");
	zassert_true(rc == 0, "read_hist value failure");
	rc = nvs_read_hist(&fs, 0, &buf, 8, 1);
	zassert_false(rc < 0, "read_hist call failure");
	rc = strcmp(buf, "DA");
	zassert_true(rc == 0, "read_hist value failure");
	rc = nvs_read_hist(&fs, 0, &buf, 8, 2);
	zassert_false(rc < 0, "read_hist call failure");
	rc = strcmp(buf, "DAT");
	zassert_true(rc == 0, "read_hist value failure");
	rc = nvs_read_hist(&fs, 0, &buf, 8, 3);
	zassert_false(rc < 0, "read_hist call failure");
	rc = strcmp(buf, "DATA");
	zassert_true(rc == 0, "read_hist value failure");
}

void nvs_test_delete(void)
{
	int rc;
	char buf1[8], buf2[8];

	/* delete an item and see if its deleted */
	rc = nvs_delete(&fs, 0);
	zassert_true(rc == 0, "delete call failure");
	rc = nvs_read(&fs, 0, &buf1, 8);
	zassert_true(rc == -ENOENT, "deleted item read");

	/* create the item again and see if it can be read */
	strcpy(buf1, "DATA");
	rc = nvs_write(&fs, 0, &buf1, strlen(buf1) + 1);
	zassert_true(rc == strlen(buf1) + 1, "write call failure");
	rc = nvs_read(&fs, 0, &buf2, 8);
	zassert_false(rc < 0, "read call failed");
	rc = strcmp(buf2, "DATA");
	zassert_true(rc == 0, "read value failure");
}

void nvs_test_rotate(void)
{
	int rc, cnt, cnt_max;
	char buf[9];

	strcpy(buf, "DATADATA");
	cnt = 0;
	/* each item written to flash takes at least strlen(buf)+1 + size of
	 * hdr and slt, so it will never be possible to fit cnt_max items in
	 * one sector
	 */
	cnt_max = (fs.sector_size)/(strlen(buf) + 1);

	/* write data until nvs rotates to a new sector history data will then
	 * be deleted, so it is not possible to read the second but last
	 * history item. Why is it the second but last and not the first but
	 * last item ? NVS rotates when a flash sector is full, it will first
	 * copy what is inside the full sector. As there is only writing with
	 * item ID 0 it will first do a copy of this and afterwards it will do
	 * a new write with item ID 0. So when reading the history data there
	 * will always be at least two items, but after a rotate there will
	 * not be 3.
	 */
	while (1) {
		rc = nvs_read_hist(&fs, 0, &buf, 8, 2);
		if (rc == -ENOENT) {
			break;
		}
		rc = nvs_write(&fs, 0, &buf, strlen(buf) + 1);
		cnt++;
		if (cnt == cnt_max) {
			break;
		}

	}
	zassert_false(cnt == cnt_max, "rotate takes to long");
}

void nvs_test_full(void)
{
	int rc, cnt, cnt_max;
	char buf[9];

	strcpy(buf, "DATADATA");
	cnt = 0;

	/* each item written to flash takes at least strlen(buf)+1 + size of
	 * hdr and slt, so it will never be possible to fit cnt_max items in
	 * one the file system
	 */
	cnt_max = ((fs.sector_count-1)*(fs.sector_size))/(strlen(buf) + 1);


	/* write data to different items until the file
	 * system is full
	 */
	while (1) {
		rc = nvs_write(&fs, cnt, &buf, strlen(buf) + 1);
		if (rc == -ENOSPC) {
			break;
		}
		cnt++;
		if (cnt == cnt_max) {
			break;
		}
	}
	zassert_false(cnt == cnt_max, "taking up space takes to long");

	/* delete data until we can write again */
	while (1) {
		rc = nvs_delete(&fs, cnt);
		rc = nvs_write(&fs, cnt, &buf, strlen(buf) + 1);
		if (rc != -ENOSPC) {
			break;
		}
		cnt--;
		if (cnt == 0) {
			break;
		}
	}
	zassert_false(cnt == 0, "freeing space takes to long");
}

void test_main(void)
{
	ztest_test_suite(test_nvs,
			 ztest_unit_test(nvs_test_init),
			 ztest_unit_test(nvs_test_write),
			 ztest_unit_test(nvs_test_read),
			 ztest_unit_test(nvs_test_read_hist),
			 ztest_unit_test(nvs_test_rotate),
			 ztest_unit_test(nvs_test_delete),
			 ztest_unit_test(nvs_test_full),
			 ztest_unit_test(nvs_test_clear)
			 );

	ztest_run_test_suite(test_nvs);
}
