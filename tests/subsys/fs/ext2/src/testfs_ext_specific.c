/*
 * Copyright (c) 2023 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/ext2.h>
#include "utils.h"
#include "../../common/test_fs_util.h"

uint32_t calculate_blocks(uint32_t freeb, uint32_t B)
{
	uint32_t blocks = 0;

	if (freeb <= 12) {
		return freeb;
	}

	blocks += 12;
	freeb -= 12 + 1; /* direct blocks + top block of first level table */

	if (freeb <= B) {
		return blocks + freeb;
	}

	blocks += B;
	freeb -= B + 1; /* 1st level blocks + top block of second level table */

	if (freeb <= B * (B + 1)) {
		uint32_t n = freeb / (B + 1);
		uint32_t r = freeb % (B + 1);
		uint32_t partial = r > 0 ? 1 : 0;

		return blocks + n * B + r - partial;
	}
	return blocks;
	/* TODO: revisit this and extend when 3rd level blocks will be possible */
}

void writing_test(struct ext2_cfg *config)
{
	int64_t ret = 0;
	struct fs_file_t file;
	struct fs_statvfs sbuf;
	struct fs_dirent entry;
	struct fs_mount_t *mp = &testfs_mnt;
	static const char *file_path = "/sml/file";

	ret = fs_mkfs(FS_EXT2, (uintptr_t)mp->storage_dev, config, 0);
	zassert_equal(ret, 0, "Failed to mkfs with 2K blocks");

	mp->flags = FS_MOUNT_FLAG_NO_FORMAT;
	ret = fs_mount(mp);
	zassert_equal(ret, 0, "Mount failed (ret=%d)", ret);

	fs_file_t_init(&file);
	ret = fs_open(&file, file_path, FS_O_RDWR | FS_O_CREATE);
	zassert_equal(ret, 0, "File open failed (ret=%d)", ret);

	ret = fs_statvfs(mp->mnt_point, &sbuf);
	zassert_equal(ret, 0, "Expected success (ret=%d)", ret);


	/* Calculate how many numbers will be written (use all available memory) */
	uint32_t freeb = sbuf.f_bfree;
	uint32_t bsize = sbuf.f_bsize;
	uint32_t available_blocks = calculate_blocks(freeb, bsize / sizeof(uint32_t));

	uint32_t bytes_to_write = bsize * available_blocks;

	TC_PRINT("Available blocks: %d\nBlock size: %d\nBytes_to_write: %d\n",
			available_blocks, bsize, bytes_to_write);

	ret = testfs_write_incrementing(&file, 0, available_blocks * bsize);
	zassert_equal(ret, bytes_to_write, "Different number of bytes written %ld (expected %ld)",
			ret, bytes_to_write);

	ret = fs_close(&file);
	zassert_equal(ret, 0, "File close failed (ret=%d)", ret);


	/* Check file size */
	ret = fs_stat(file_path, &entry);
	zassert_equal(ret, 0, "File stat failed (ret=%d)", ret);
	zassert_equal(entry.size, bytes_to_write,
			"Wrong file size %d (expected %d)", entry.size,
			bytes_to_write);

	fs_file_t_init(&file);
	ret = fs_open(&file, file_path, FS_O_READ);
	zassert_equal(ret, 0, "File open failed (ret=%d)", ret);


	ret = testfs_verify_incrementing(&file, 0, available_blocks * bsize);
	zassert_equal(ret, bytes_to_write, "Different number of bytes read %ld (expected %ld)",
			ret, bytes_to_write);

	ret = fs_close(&file);
	zassert_equal(ret, 0, "File close failed (ret=%d)", ret);

	uint32_t new_size = bytes_to_write;

	while (new_size > 1) {
		new_size = new_size / 8 * 3;

		TC_PRINT("Truncating to %d\n", new_size);

		ret = fs_open(&file, file_path, FS_O_RDWR);
		zassert_equal(ret, 0, "File open failed (ret=%d)", ret);

		ret = fs_truncate(&file, new_size);
		zassert_equal(ret, 0, "File truncate failed (ret=%d)", ret);

		ret = fs_stat(file_path, &entry);
		zassert_equal(ret, 0, "File stat failed (ret=%d)", ret);
		zassert_equal(entry.size, new_size,
				"Wrong file size %d (expected %d)", entry.size, new_size);

		ret = fs_seek(&file, 0, FS_SEEK_SET);
		zassert_equal(ret, 0, "File seek failed (ret=%d)", ret);

		ret = testfs_verify_incrementing(&file, 0, new_size);
		zassert_equal(ret, new_size, "Different number of bytes read %ld (expected %ld)",
				ret, new_size);

		ret = fs_close(&file);
		zassert_equal(ret, 0, "File close failed (ret=%d)", ret);
	}

	ret = fs_unmount(mp);
	zassert_equal(ret, 0, "Unmount failed (ret=%d)", ret);
}

ZTEST(ext2tests, test_write_big_file)
{
	writing_test(NULL);
}

#if defined(CONFIG_APP_TEST_BIG)
ZTEST(ext2tests, test_write_big_file_2K)
{
	struct ext2_cfg config = {
		.block_size = 2048,
		.fs_size = 0x2000000,
		.bytes_per_inode = 0,
		.volume_name[0] = 0,
		.set_uuid = false,
	};

	writing_test(&config);
}

ZTEST(ext2tests, test_write_big_file_4K)
{
	struct ext2_cfg config = {
		.block_size = 4096,
		.fs_size = 0x8000000,
		.bytes_per_inode = 0,
		.volume_name[0] = 0,
		.set_uuid = false,
	};

	writing_test(&config);
}
#endif
