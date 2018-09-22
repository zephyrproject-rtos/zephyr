/*
 * Copyright (c) 2017 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <nffs/nffs.h>
#include <fs.h>
#include "nffs_test_utils.h"
#include <ztest_assert.h>
#include <timestamp.h>
#include <sys_clock.h>

#define TEST_NUM_FILES		500
#define RW_DATA_LENGTH		(128 * 1024)
#define RW_CHUNK_LENGTH		256

#define TEST_DATA_LEN (1024 * 24)

static const struct nffs_area_desc area_descs[] = {
	{ 0x00020000, 16384 },
	{ 0x00024000, 16384 },
	{ 0x00028000, 16384 },
	{ 0x0002c000, 16384 },
	{ 0x00030000, 16384 },
	{ 0x00034000, 16384 },
	{ 0x00038000, 16384 },
	{ 0x0003c000, 16384 },
	{ 0x00040000, 16384 },
	{ 0x00044000, 16384 },
	{ 0x00048000, 16384 },
	{ 0x0004c000, 16384 },
	{ 0, 0 },
};

void test_performance(void)
{
	char filename[16];
	struct fs_file_t file;
	s64_t reftime;
	u32_t delta;
	int i;
	int rc;

	/* Format to start on clean flash */
	rc = nffs_format_full(area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	/* 1. Create files benchmark */
	printk("1. Creating files...\n");
	nffs_cache_clear();
	k_uptime_delta(&reftime);

	for (i = 0; i < TEST_NUM_FILES; i++) {
		snprintf(filename, sizeof(filename), NFFS_MNTP"/file_%d", i);
		rc = fs_open(&file, filename);
		zassert_equal(rc, 0, "cannot open file");
		rc = fs_close(&file);
		zassert_equal(rc, 0, "cannot close file");
	}

	delta = k_uptime_delta_32(&reftime);
	printk("Created %d files in %d.%03d seconds\n", TEST_NUM_FILES,
	       delta / 1000, delta % 1000);

	/* 2. Unlink files benchmark */
	printk("2. Unlinking files...\n");
	nffs_cache_clear();
	k_uptime_delta(&reftime);

	for (i = 0; i < TEST_NUM_FILES; i++) {
		snprintf(filename, sizeof(filename), NFFS_MNTP"/file_%d", i);
		rc = fs_unlink(filename);
		zassert_equal(rc, 0, "cannot unlink file");
	}

	delta = k_uptime_delta_32(&reftime);
	printk("Unlinked %d files in %d.%03d seconds\n", TEST_NUM_FILES,
	       delta / 1000, delta % 1000);

	/* Format to start on clean flash */
	rc = nffs_format_full(area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	/* 3. Write file benchmark */
	printk("3. Writing file...\n");
	nffs_cache_clear();
	k_uptime_delta(&reftime);

	rc = fs_open(&file, NFFS_MNTP"/file");
	zassert_equal(rc, 0, "cannot open file");
	(void)memset(nffs_test_buf, 0, RW_CHUNK_LENGTH);
	for (i = 0; i < RW_DATA_LENGTH; ) {
		rc = fs_write(&file, nffs_test_buf, RW_CHUNK_LENGTH);
		zassert_equal(rc, RW_CHUNK_LENGTH, "cannot write file");
		i += rc;
	}
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	delta = k_uptime_delta_32(&reftime);
	printk("Written %d bytes in %d.%03d seconds\n", i, delta / 1000,
	       delta % 1000);

	/* 4. Read file benchmark */
	printk("4. Reading file...\n");
	nffs_cache_clear();
	k_uptime_delta(&reftime);

	rc = fs_open(&file, NFFS_MNTP"/file");
	zassert_equal(rc, 0, "cannot open file");
	for (i = 0; i < RW_DATA_LENGTH; ) {
		rc = fs_read(&file, nffs_test_buf, RW_CHUNK_LENGTH);
		zassert_equal(rc, RW_CHUNK_LENGTH, "cannot read file");
		i += rc;
	}
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	delta = k_uptime_delta_32(&reftime);
	printk("Read %d bytes in %d.%03d seconds\n", i, delta / 1000,
	       delta % 1000);

	/* Format to start on clean flash */
	rc = nffs_format_full(area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	/* 5. Write file benchmark */
	printk("5. Writing file (max block size)...\n");
	nffs_cache_clear();
	k_uptime_delta(&reftime);

	rc = fs_open(&file, NFFS_MNTP"/file");
	zassert_equal(rc, 0, "cannot open file");
	(void)memset(nffs_test_buf, 0, NFFS_BLOCK_MAX_DATA_SZ_MAX);
	for (i = 0; i < RW_DATA_LENGTH; ) {
		rc = fs_write(&file, nffs_test_buf, NFFS_BLOCK_MAX_DATA_SZ_MAX);
		zassert_equal(rc, NFFS_BLOCK_MAX_DATA_SZ_MAX,
			      "cannot write file");
		i += rc;
	}
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	delta = k_uptime_delta_32(&reftime);
	printk("Written %d bytes in %d.%03d seconds\n", i, delta / 1000,
	       delta % 1000);

	/* 6. Read file benchmark */
	printk("6. Reading file (max block size)...\n");
	nffs_cache_clear();
	k_uptime_delta(&reftime);

	rc = fs_open(&file, NFFS_MNTP"/file");
	zassert_equal(rc, 0, "cannot open file");
	for (i = 0; i < RW_DATA_LENGTH; ) {
		rc = fs_read(&file, nffs_test_buf, NFFS_BLOCK_MAX_DATA_SZ_MAX);
		zassert_equal(rc, NFFS_BLOCK_MAX_DATA_SZ_MAX,
			      "cannot read file");
		i += rc;
	}
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	delta = k_uptime_delta_32(&reftime);
	printk("Read %d bytes in %d.%03d seconds\n", i, delta / 1000,
	       delta % 1000);
}
