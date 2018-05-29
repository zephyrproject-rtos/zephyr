/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <fs.h>
#include <ztest.h>
#include <ztest_assert.h>
#include "nffs_test_utils.h"

extern struct k_mem_slab nffs_block_entry_pool;
extern struct k_mem_slab nffs_inode_entry_pool;

void test_nffs_open(void)
{
	struct fs_file_t file;
	struct fs_dir_t dir;
	int rc;

	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	/*** Fail to open an invalid path (not rooted). */
	rc = fs_open(&file, "file");
	zassert_equal(rc, -EINVAL, "failed to detect invalid path");

	/*** Fail to open a directory (root directory). */
	rc = fs_open(&file, "/");
	zassert_equal(rc, -EINVAL, "failed to detect invalid directory");

	/*** Fail to open a child of a nonexistent directory. */
	rc = fs_open(&file, "/dir/myfile.txt");
	zassert_equal(rc, -ENOENT, "failed to detect nonexistent directory");
	rc = fs_opendir(&dir, "/dir");
	zassert_equal(rc, -ENOENT, "failed to detect nonexistent directory");

	rc = fs_mkdir(NFFS_MNTP"/dir");
	zassert_equal(rc, 0, "failed to open directory");

	/*** Fail to open a directory. */
	rc = fs_open(&file, NFFS_MNTP"/dir");
	zassert_equal(rc, -EINVAL, "failed to open a directory");

	/*** Successfully open an existing file for reading. */
	nffs_test_util_create_file(NFFS_MNTP"/dir/file.txt", "1234567890", 10);
	rc = fs_open(&file, NFFS_MNTP"/dir/file.txt");
	zassert_equal(rc, 0, "failed to open a file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	/*** Successfully open an nonexistent file for writing. */
	rc = fs_open(&file, NFFS_MNTP"/dir/file2.txt");
	zassert_equal(rc, 0, "cannot open nonexistent file for writing");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	/*** Ensure the file can be reopened. */
	rc = fs_open(&file, NFFS_MNTP"/dir/file.txt");
	zassert_equal(rc, 0, "cannot reopen file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close reopened file");
}

void test_nffs_read(void)
{
	u8_t buf[16];
	struct fs_file_t file;
	int rc;

	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	nffs_test_util_create_file(NFFS_MNTP"/myfile.txt", "1234567890", 10);

	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(file.filep, 10);
	zassert_equal(fs_tell(&file), 0, "invalid pos in file");

	rc = fs_read(&file, &buf, 4);
	zassert_equal(rc, 4, "invalid bytes read");
	zassert_equal(memcmp(buf, "1234", 4), 0, "invalid buffer size");
	zassert_equal(fs_tell(&file), 4, "invalid pos in file");

	rc = fs_read(&file, buf + 4, sizeof(buf) - 4);
	zassert_equal(rc, 6, "invalid bytes read");
	zassert_equal(memcmp(buf, "1234567890", 10), 0, "invalid buffer size");
	zassert_equal(fs_tell(&file), 10, "invalid pos in file");

	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");
}

void test_nffs_write(void)
{
	struct fs_file_t file;
	struct nffs_file *nffs_file;
	int rc;

	/*** Setup. */
	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	nffs_test_util_append_file(NFFS_MNTP"/myfile.txt", "abcdefgh", 8);

	/*** Overwrite within one block (middle). */
	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	nffs_file = file.filep;
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 0, "invalid pos in file");

	rc = fs_seek(&file, 3, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 3, "invalid pos in file");

	rc = fs_write(&file, "12", 2);
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 5, "cannot get pos in file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	nffs_test_util_assert_contents(NFFS_MNTP"/myfile.txt", "abc12fgh", 8);
	nffs_test_util_assert_block_count(NFFS_MNTP"/myfile.txt", 1);

	/*** Overwrite within one block (start). */
	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 0, "invalid pos in file");

	rc = fs_write(&file, "xy", 2);
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 2, "invalid pos in file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	nffs_test_util_assert_contents(NFFS_MNTP"/myfile.txt", "xyc12fgh", 8);
	nffs_test_util_assert_block_count(NFFS_MNTP"/myfile.txt", 1);

	/*** Overwrite within one block (end). */
	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 0, "invalid pos in file");

	rc = fs_seek(&file, 6, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 6, "invalid pos in file");

	rc = fs_write(&file, "<>", 2);
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 8, "invalid pos in file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	nffs_test_util_assert_contents(NFFS_MNTP"/myfile.txt", "xyc12f<>", 8);
	nffs_test_util_assert_block_count(NFFS_MNTP"/myfile.txt", 1);

	/*** Overwrite one block middle, extend. */
	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 0, "invalid pos in file");

	rc = fs_seek(&file, 4, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 4, "invalid pos in file");

	rc = fs_write(&file, "abcdefgh", 8);
	nffs_test_util_assert_file_len(nffs_file, 12);
	zassert_equal(fs_tell(&file), 12, "invalid pos in file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	nffs_test_util_assert_contents(NFFS_MNTP"/myfile.txt",
						"xyc1abcdefgh", 12);
	nffs_test_util_assert_block_count(NFFS_MNTP"/myfile.txt", 1);

	/*** Overwrite one block start, extend. */
	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(nffs_file, 12);
	zassert_equal(fs_tell(&file), 0, "invalid pos in file");

	rc = fs_write(&file, "abcdefghijklmnop", 16);
	nffs_test_util_assert_file_len(nffs_file, 16);
	zassert_equal(fs_tell(&file), 16, "invalid pos in file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	nffs_test_util_assert_contents(NFFS_MNTP"/myfile.txt",
						"abcdefghijklmnop", 16);
	nffs_test_util_assert_block_count(NFFS_MNTP"/myfile.txt", 1);

	struct nffs_test_file_desc *expected_system =
		(struct nffs_test_file_desc[]) { {
			.filename = "",
			.is_dir = 1,
			.children = (struct nffs_test_file_desc[]) { {
				.filename = "myfile.txt",
				.contents = "abcdefghijklmnop",
				.contents_len = 16,
			}, {
				.filename = NULL,
			} },
		} };

	nffs_test_assert_system(expected_system, nffs_current_area_descs);
}

void test_nffs_unlink(void)
{
	struct fs_dirent file_stats;
	struct fs_file_t file0, file1;
	u8_t buf[64];
	struct nffs_file *nffs_file;
	u32_t bytes_read;
	int initial_num_blocks;
	int initial_num_inodes;
	int rc;

	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	initial_num_blocks = k_mem_slab_num_free_get(&nffs_block_entry_pool);
	initial_num_inodes = k_mem_slab_num_free_get(&nffs_inode_entry_pool);

	nffs_test_util_create_file(NFFS_MNTP"/file0.txt", "0", 1);

	rc = fs_open(&file0, NFFS_MNTP"/file0.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_file = file0.filep;
	zassert_equal(nffs_file->nf_inode_entry->nie_refcnt, 2, "inode error");

	rc = fs_unlink(NFFS_MNTP"/file0.txt");
	zassert_equal(rc, 0, "");
	zassert_equal(nffs_file->nf_inode_entry->nie_refcnt, 1, "inode error");

	rc = fs_stat(NFFS_MNTP"/file0.txt", &file_stats);
	zassert_not_equal(rc, 0, "no such file");

	rc = fs_write(&file0, "00", 2);

	rc = fs_seek(&file0, 0, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");

	bytes_read = fs_read(&file0, buf, sizeof(buf));
	zassert_equal(bytes_read, 2, "invalid bytes read");
	zassert_equal(memcmp(buf, "00", 2), 0, "invalid buffer size");

	rc = fs_close(&file0);
	zassert_equal(rc, 0, "cannot close file");


	rc = fs_stat(NFFS_MNTP"/file0.txt", &file_stats);
	zassert_not_equal(rc, 0, "no such file");

	/* Ensure the file was fully removed from RAM. */
	zassert_equal(k_mem_slab_num_free_get(&nffs_inode_entry_pool),
				initial_num_inodes, "file not remove entirely");
	zassert_equal(k_mem_slab_num_free_get(&nffs_block_entry_pool),
				initial_num_blocks, "file not remove entirely");

	/*** Nested unlink. */
	rc = fs_mkdir(NFFS_MNTP"/mydir");
	zassert_equal(rc, 0, "cannot make directory");
	nffs_test_util_create_file(NFFS_MNTP"/mydir/file1.txt", "1", 2);

	rc = fs_open(&file1, NFFS_MNTP"/mydir/file1.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_file = file1.filep;
	zassert_equal(nffs_file->nf_inode_entry->nie_refcnt, 2, "inode error");

	rc = fs_unlink(NFFS_MNTP"/mydir");
	zassert_equal(rc, 0, "cannot delete directory");
	zassert_equal(nffs_file->nf_inode_entry->nie_refcnt, 1, "inode error");

	rc = fs_stat(NFFS_MNTP"/mydir/file1.txt", &file_stats);
	zassert_not_equal(rc, 0, "unlink failed");

	rc = fs_write(&file1, "11", 2);

	rc = fs_seek(&file1, 0, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");

	bytes_read = fs_read(&file1, buf, sizeof(buf));
	zassert_equal(bytes_read, 2, "invalid bytes read");
	zassert_equal(memcmp(buf, "11", 2), 0, "invalid buffer size");

	rc = fs_close(&file1);
	zassert_equal(rc, 0, "cannot close file");

	rc = fs_stat(NFFS_MNTP"/mydir/file1.txt", &file_stats);
	zassert_not_equal(rc, 0, "unlink failed");

	struct nffs_test_file_desc *expected_system =
		(struct nffs_test_file_desc[]) { {
			.filename = "",
			.is_dir = 1,
		} };

	nffs_test_assert_system(expected_system, nffs_current_area_descs);

	/* Ensure the files and directories were fully removed from RAM. */
	zassert_equal(k_mem_slab_num_free_get(&nffs_inode_entry_pool),
				initial_num_inodes, "not all removed from RAM");
	zassert_equal(k_mem_slab_num_free_get(&nffs_block_entry_pool),
				initial_num_blocks, "not all removed from RAM");
}
