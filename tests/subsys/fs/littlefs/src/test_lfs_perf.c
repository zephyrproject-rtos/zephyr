/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* littlefs performance testing */

#include <string.h>
#include <stdlib.h>
#include <kernel.h>
#include <ztest.h>
#include "testfs_tests.h"
#include "testfs_lfs.h"
#include <lfs.h>

#include <fs/littlefs.h>

#define HELLO "hello"
#define GOODBYE "goodbye"

static int write_read(const char *tag,
		      struct fs_mount_t *mp,
		      size_t buf_size,
		      size_t nbuf)
{
	const struct lfs_config *lcp = &((const struct fs_littlefs *)mp->fs_data)->cfg;
	struct testfs_path path;
	struct fs_statvfs vfs;
	struct fs_dirent stat;
	struct fs_file_t file;
	size_t total = nbuf * buf_size;
	uint32_t t0;
	uint32_t t1;
	uint8_t *buf;
	int rc;
	int rv = TC_FAIL;

	TC_PRINT("clearing %s for %s write/read test\n",
		 mp->mnt_point, tag);
	if (testfs_lfs_wipe_partition(mp) != TC_PASS) {
		return TC_FAIL;
	}

	rc = fs_mount(mp);
	if (rc != 0) {
		TC_PRINT("Mount %s failed: %d\n", mp->mnt_point, rc);
		return TC_FAIL;
	}

	rc = fs_statvfs(mp->mnt_point, &vfs);
	if (rc != 0) {
		TC_PRINT("statvfs %s failed: %d\n", mp->mnt_point, rc);
		goto out_mnt;
	}

	TC_PRINT("%s: bsize %lu ; frsize %lu ; blocks %lu ; bfree %lu\n",
		 mp->mnt_point,
		 vfs.f_bsize, vfs.f_frsize, vfs.f_blocks, vfs.f_bfree);
	TC_PRINT("read_size %u ; prog_size %u ; cache_size %u ; lookahead_size %u\n",
		 lcp->read_size, lcp->prog_size, lcp->cache_size, lcp->lookahead_size);

	testfs_path_init(&path, mp,
			 "data",
			 TESTFS_PATH_END);

	buf = calloc(buf_size, sizeof(uint8_t));
	if (buf == NULL) {
		TC_PRINT("Failed to allocate %zu-byte buffer\n", buf_size);
		goto out_mnt;
	}

	for (size_t i = 0; i < buf_size; ++i) {
		buf[i] = i;
	}

	TC_PRINT("creating and writing %zu %zu-byte blocks\n",
		 nbuf, buf_size);

	rc = fs_open(&file, path.path, FS_O_CREATE | FS_O_RDWR);
	if (rc != 0) {
		TC_PRINT("Failed to open %s for write: %d\n", path.path, rc);
		goto out_buf;
	}

	t0 = k_uptime_get_32();
	for (size_t i = 0; i < nbuf; ++i) {
		rc = fs_write(&file, buf, buf_size);
		if (buf_size != rc) {
			TC_PRINT("Failed to write buf %zu: %d\n", i, rc);
			goto out_file;
		}
	}
	t1 = k_uptime_get_32();

	if (t1 == t0) {
		t1++;
	}

	(void)fs_close(&file);

	rc = fs_stat(path.path, &stat);
	if (rc != 0) {
		TC_PRINT("Failed to stat %s: %d\n", path.path, rc);
		goto out_buf;
	}

	if (stat.size != total) {
		TC_PRINT("File size %zu not %zu\n", stat.size, total);
		goto out_buf;
	}

	TC_PRINT("%s write %zu * %zu = %zu bytes in %u ms: "
		 "%u By/s, %u KiBy/s\n",
		 tag, nbuf, buf_size, total, (t1 - t0),
		 (uint32_t)(total * 1000U / (t1 - t0)),
		 (uint32_t)(total * 1000U / (t1 - t0) / 1024U));

	rc = fs_open(&file, path.path, FS_O_CREATE | FS_O_RDWR);
	if (rc != 0) {
		TC_PRINT("Failed to open %s for write: %d\n", path.path, rc);
		goto out_buf;
	}

	t0 = k_uptime_get_32();
	for (size_t i = 0; i < nbuf; ++i) {
		rc = fs_read(&file, buf, buf_size);
		if (buf_size != rc) {
			TC_PRINT("Failed to read buf %zu: %d\n", i, rc);
			goto out_file;
		}
	}
	t1 = k_uptime_get_32();

	if (t1 == t0) {
		t1++;
	}

	TC_PRINT("%s read %zu * %zu = %zu bytes in %u ms: "
		 "%u By/s, %u KiBy/s\n",
		 tag, nbuf, buf_size, total, (t1 - t0),
		 (uint32_t)(total * 1000U / (t1 - t0)),
		 (uint32_t)(total * 1000U / (t1 - t0) / 1024U));

	rv = TC_PASS;

out_file:
	(void)fs_close(&file);

out_buf:
	free(buf);

out_mnt:
	(void)fs_unmount(mp);

	return rv;
}

static int custom_write_test(const char *tag,
			     const struct fs_mount_t *mp,
			     const struct lfs_config *cfgp,
			     size_t buf_size,
			     size_t nbuf)
{
	struct fs_littlefs data = {
		.cfg = *cfgp,
	};
	struct fs_mount_t lfs_mnt = {
		.type = FS_LITTLEFS,
		.fs_data = &data,
		.storage_dev = mp->storage_dev,
		.mnt_point = mp->mnt_point,
	};
	struct lfs_config *lcp = &data.cfg;
	int rv = TC_FAIL;

	if (lcp->cache_size == 0) {
		lcp->cache_size = CONFIG_FS_LITTLEFS_CACHE_SIZE;
	}
	if (lcp->lookahead_size == 0) {
		lcp->lookahead_size = CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE;
	}

	lcp->read_buffer = malloc(lcp->cache_size);
	lcp->prog_buffer = malloc(lcp->cache_size);
	lcp->lookahead_buffer = malloc(lcp->lookahead_size);

	TC_PRINT("bufs %p %p %p\n", lcp->read_buffer, lcp->prog_buffer, lcp->lookahead_buffer);

	if ((lcp->read_buffer == NULL)
	    || (lcp->prog_buffer == NULL)
	    || (lcp->lookahead_buffer == NULL)) {
		TC_PRINT("%s buffer allocation failed\n", tag);
		goto out_free;
	}

	rv = write_read(tag, &lfs_mnt, buf_size, nbuf);

out_free:
	if (lcp->read_buffer) {
		free(lcp->read_buffer);
	}
	if (lcp->prog_buffer) {
		free(lcp->prog_buffer);
	}
	if (lcp->lookahead_buffer) {
		free(lcp->lookahead_buffer);
	}

	return rv;
}

static int small_8_1K_cust(void)
{
	struct lfs_config cfg = {
		.read_size = LARGE_IO_SIZE,
		.prog_size = LARGE_IO_SIZE,
		.cache_size = LARGE_CACHE_SIZE,
		.lookahead_size = LARGE_LOOKAHEAD_SIZE
	};

	return custom_write_test("small 8x1K bigfile", &testfs_small_mnt, &cfg, 1024, 8);
}

void test_lfs_perf(void)
{
	k_sleep(K_MSEC(100));   /* flush log messages */
	zassert_equal(write_read("small 8x1K dflt",
				 &testfs_small_mnt,
				 1024, 8),
		      TC_PASS,
		      "failed");

	k_sleep(K_MSEC(100));   /* flush log messages */
	zassert_equal(small_8_1K_cust(), TC_PASS,
		      "failed");

	k_sleep(K_MSEC(100));   /* flush log messages */
	zassert_equal(write_read("medium 32x2K dflt",
				 &testfs_medium_mnt,
				 2048, 32),
		      TC_PASS,
		      "failed");

	k_sleep(K_MSEC(100));   /* flush log messages */
	zassert_equal(write_read("large 64x4K dflt",
				 &testfs_large_mnt,
				 4096, 64),
		      TC_PASS,
		      "failed");
}
