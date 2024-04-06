/*
 * Copyright (c) 2024 Laczen.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/storage/flash_partitions.h>

#define FLASH_PARTITION0 flash0_partition0
#define FLASH_PARTITION1 flash0_partition1

ZTEST_BMEM static const struct flash_partition *fp;

static void *flash_partition_api_setup(void)
{
	return NULL;
}

ZTEST_USER(flash_partition_api, test_read_write_erase)
{
	uint8_t *wr = "/a9/9a/a9/9a";
	uint8_t rd[sizeof(wr)];
	int rc = 0;

	memset(rd, 0, sizeof(rd));

	rc = flash_partition_open(fp);
	zassert_equal(rc, 0, "open returned [%d]", rc);

	TC_PRINT("flash partition: size %d, offset %d, erase-block-size %d\n",
		 fp->size, fp->offset, fp->erase_block_size);

	rc = flash_partition_read(fp, 0, rd, sizeof(rd));
	zassert_equal(rc, 0, "read returned [%d]", rc);

	if (fp->write != NULL) {
		rc = flash_partition_write(fp, 0, wr, sizeof(wr));
		zassert_equal(rc, 0, "write returned [%d]", rc);

		rc = flash_partition_read(fp, 0, rd, sizeof(rd));
		zassert_equal(rc, 0, "read returned [%d]", rc);

		zassert_equal(memcmp(wr, rd, sizeof(rd)), 0,
			      "read/write data differ");
	}

	if (fp->erase != NULL) {
		rc = flash_partition_erase(fp, 0, fp->erase_block_size);
		zassert_equal(rc, 0, "erase returned [%d]", rc);
	} else {
		TC_PRINT("no erase\n");
	}
}

ZTEST_SUITE(flash_partition_api, NULL, flash_partition_api_setup, NULL, NULL,
	    NULL);

/* Run all of our tests on the given flash partition */
static void run_tests_on_partition(const struct flash_partition *partition)
{
	fp = partition;
	ztest_run_all(NULL, false, 1, 1);
}

void test_main(void)
{
	run_tests_on_partition(FLASH_PARTITION_GET(FLASH_PARTITION0));
	run_tests_on_partition(FLASH_PARTITION_GET(FLASH_PARTITION1));
	ztest_verify_all_test_suites_ran();
}
