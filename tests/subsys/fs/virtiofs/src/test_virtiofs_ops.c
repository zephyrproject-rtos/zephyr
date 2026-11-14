/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/virtiofs.h>
#include "test_virtiofs.h"

#define PATH(name) VIRTIOFS_MNT "/" name

/* Create a file, write to it, read it back and check the contents survive a
 * close/re-open round trip through the host file system.
 */
ZTEST(virtiofs, test_create_write_read)
{
	static const char payload[] = "virtiofs round trip";
	char buf[sizeof(payload)];
	struct fs_file_t file;

	fs_file_t_init(&file);

	zassert_ok(fs_open(&file, PATH("rw_file"), FS_O_CREATE | FS_O_RDWR),
		   "open for write failed");
	zassert_equal(fs_write(&file, payload, sizeof(payload)), sizeof(payload),
		      "short write");
	zassert_ok(fs_close(&file), "close after write failed");

	fs_file_t_init(&file);
	zassert_ok(fs_open(&file, PATH("rw_file"), FS_O_READ), "open for read failed");
	zassert_equal(fs_read(&file, buf, sizeof(buf)), sizeof(buf), "short read");
	zassert_ok(fs_close(&file), "close after read failed");

	zassert_mem_equal(buf, payload, sizeof(payload), "read back mismatch");
}

/* A device that reports max_write == 0 must still be mountable: zero-length
 * writes are no-ops, while non-empty writes are rejected.
 */
ZTEST(virtiofs, test_zero_max_write)
{
	struct virtiofs_fs_data *fs_data = testfs_mnt.fs_data;
	const char byte = 'x';
	struct fs_file_t file;
	uint32_t saved;
	ssize_t zero_ret;
	ssize_t write_ret;

	fs_file_t_init(&file);
	zassert_ok(fs_open(&file, PATH("zero_max_write"), FS_O_CREATE | FS_O_RDWR), "open failed");

	saved = fs_data->max_write;
	fs_data->max_write = 0U;

	zero_ret = fs_write(&file, &byte, 0U);
	write_ret = fs_write(&file, &byte, 1U);

	fs_data->max_write = saved;
	zassert_ok(fs_close(&file), "close failed");

	zassert_equal(zero_ret, 0, "zero-length write failed");
	zassert_equal(write_ret, -ENOTSUP, "write with max_write == 0 was not rejected");
}

/* stat a freshly written file and confirm type and size. */
ZTEST(virtiofs, test_stat)
{
	static const char payload[] = "0123456789";
	struct fs_file_t file;
	struct fs_dirent stat;

	fs_file_t_init(&file);
	zassert_ok(fs_open(&file, PATH("stat_file"), FS_O_CREATE | FS_O_WRITE),
		   "open failed");
	zassert_equal(fs_write(&file, payload, sizeof(payload)), sizeof(payload),
		      "short write");
	zassert_ok(fs_close(&file), "close failed");

	zassert_ok(fs_stat(PATH("stat_file"), &stat), "stat failed");
	zassert_equal(stat.type, FS_DIR_ENTRY_FILE, "not a file");
	zassert_equal(stat.size, sizeof(payload), "unexpected size");
}

/* Create a directory with a couple of files and verify they show up in a
 * readdir, exercising mkdir/opendir/readdir over virtiofs.
 */
ZTEST(virtiofs, test_mkdir_readdir)
{
	struct fs_file_t file;
	struct fs_dir_t dir;
	struct fs_dirent entry;
	bool seen_a = false;
	bool seen_b = false;

	zassert_ok(fs_mkdir(PATH("d")), "mkdir failed");

	fs_file_t_init(&file);
	zassert_ok(fs_open(&file, PATH("d/a"), FS_O_CREATE | FS_O_WRITE), "create a failed");
	zassert_ok(fs_close(&file), "close a failed");

	fs_file_t_init(&file);
	zassert_ok(fs_open(&file, PATH("d/b"), FS_O_CREATE | FS_O_WRITE), "create b failed");
	zassert_ok(fs_close(&file), "close b failed");

	fs_dir_t_init(&dir);
	zassert_ok(fs_opendir(&dir, PATH("d")), "opendir failed");

	while (true) {
		zassert_ok(fs_readdir(&dir, &entry), "readdir failed");
		if (entry.name[0] == '\0') {
			break;
		}
		if (strcmp(entry.name, "a") == 0) {
			seen_a = true;
		} else if (strcmp(entry.name, "b") == 0) {
			seen_b = true;
		}
	}

	zassert_ok(fs_closedir(&dir), "closedir failed");
	zassert_true(seen_a && seen_b, "created entries not listed");
}

/* Unlink removes a file so a subsequent stat fails. */
ZTEST(virtiofs, test_unlink)
{
	struct fs_file_t file;
	struct fs_dirent stat;

	fs_file_t_init(&file);
	zassert_ok(fs_open(&file, PATH("victim"), FS_O_CREATE | FS_O_WRITE), "create failed");
	zassert_ok(fs_close(&file), "close failed");
	zassert_ok(fs_stat(PATH("victim"), &stat), "stat before unlink failed");

	zassert_ok(fs_unlink(PATH("victim")), "unlink failed");
	zassert_not_equal(fs_stat(PATH("victim"), &stat), 0, "file still present after unlink");
}

/*
 * Enough files and name lengths to span several READDIR response buffers.
 * Each entry also triggers a LOOKUP for its size (and a FORGET), so this
 * exercises the batched parser and the asynchronous FORGET path together.
 */
#define BATCH_FILES 40U

ZTEST(virtiofs, test_readdir_multiple_batches)
{
	struct fs_dir_t dir;
	struct fs_dirent entry;
	bool seen[BATCH_FILES] = {false};
	char name[32];
	size_t count = 0;

	zassert_ok(fs_mkdir(PATH("batch")), "mkdir failed");

	for (size_t i = 0; i < BATCH_FILES; i++) {
		struct fs_file_t file;
		char path[64];

		snprintf(name, sizeof(name), "file_%02zu_%s", i,
			 (i % 2) == 0 ? "n" : "longer_name");
		snprintf(path, sizeof(path), "%s/%s", PATH("batch"), name);

		fs_file_t_init(&file);
		zassert_ok(fs_open(&file, path, FS_O_CREATE | FS_O_WRITE), "create %s failed",
			   name);
		zassert_ok(fs_close(&file), "close %s failed", name);
	}

	fs_dir_t_init(&dir);
	zassert_ok(fs_opendir(&dir, PATH("batch")), "opendir failed");

	while (true) {
		unsigned int index;

		zassert_ok(fs_readdir(&dir, &entry), "readdir failed");
		if (entry.name[0] == '\0') {
			break;
		}

		zassert_equal(sscanf(entry.name, "file_%02u_", &index), 1,
			      "unexpected entry name %s", entry.name);
		zassert_true(index < BATCH_FILES, "entry index %u out of range", index);
		zassert_false(seen[index], "duplicate entry %s", entry.name);
		zassert_equal(entry.type, FS_DIR_ENTRY_FILE, "unexpected type for %s", entry.name);
		seen[index] = true;
		count++;
	}

	zassert_ok(fs_closedir(&dir), "closedir failed");
	zassert_equal(count, BATCH_FILES, "expected %u entries, got %zu", BATCH_FILES, count);
}

/* An empty directory must terminate the first readdir immediately. */
ZTEST(virtiofs, test_readdir_empty)
{
	struct fs_dir_t dir;
	struct fs_dirent entry;

	zassert_ok(fs_mkdir(PATH("empty")), "mkdir failed");
	fs_dir_t_init(&dir);
	zassert_ok(fs_opendir(&dir, PATH("empty")), "opendir failed");
	zassert_ok(fs_readdir(&dir, &entry), "readdir failed");
	zassert_equal(entry.name[0], '\0', "empty directory returned an entry");
	zassert_ok(fs_closedir(&dir), "closedir failed");
}

ZTEST(virtiofs, test_readdir_buffer_exhaustion)
{
	struct fs_dir_t dirs[CONFIG_VIRTIOFS_READDIR_BUFFERS + 1U];
	struct fs_file_t file;
	struct fs_dirent entry;
	size_t opened = 0U;
	int ret = 0;
	int close_ret = 0;
	bool entries_match = true;

	zassert_ok(fs_mkdir(PATH("fallback")), "mkdir failed");
	fs_file_t_init(&file);
	zassert_ok(fs_open(&file, PATH("fallback/file"), FS_O_CREATE | FS_O_WRITE), "open failed");
	zassert_ok(fs_close(&file), "close failed");

	while (opened < ARRAY_SIZE(dirs)) {
		fs_dir_t_init(&dirs[opened]);
		ret = fs_opendir(&dirs[opened], PATH("fallback"));
		if (ret != 0) {
			break;
		}
		opened++;
	}

	for (size_t i = 0U; i < opened && ret == 0; i++) {
		ret = fs_readdir(&dirs[i], &entry);
		if (ret != 0) {
			break;
		}
		entries_match = entries_match && strcmp(entry.name, "file") == 0 &&
				entry.type == FS_DIR_ENTRY_FILE;
		ret = fs_readdir(&dirs[i], &entry);
		entries_match = entries_match && ret == 0 && entry.name[0] == '\0';
	}

	for (size_t i = 0U; i < opened; i++) {
		int err = fs_closedir(&dirs[i]);

		if (err != 0) {
			close_ret = err;
		}
	}

	zassert_equal(opened, ARRAY_SIZE(dirs), "not all directories opened");
	zassert_ok(ret, "directory read failed");
	zassert_true(entries_match, "unexpected entry or missing end of directory");
	zassert_ok(close_ret, "closedir failed");
}

ZTEST(virtiofs, test_readdir_oversized_name)
{
	struct fs_file_t file;
	struct fs_dir_t dir;
	struct fs_dirent entry;
	char path[sizeof(PATH("long_name/")) + MAX_FILE_NAME + 1U] = PATH("long_name/");
	size_t prefix_len = strlen(path);
	int ret;

	BUILD_ASSERT(MAX_FILE_NAME < 255, "test name must fit the host filesystem");
	memset(path + prefix_len, 'x', MAX_FILE_NAME + 1U);
	path[sizeof(path) - 1U] = '\0';
	zassert_ok(fs_mkdir(PATH("long_name")), "mkdir failed");
	fs_file_t_init(&file);
	zassert_ok(fs_open(&file, path, FS_O_CREATE | FS_O_WRITE), "open failed");
	zassert_ok(fs_close(&file), "close failed");
	fs_dir_t_init(&dir);
	zassert_ok(fs_opendir(&dir, PATH("long_name")), "opendir failed");
	ret = fs_readdir(&dir, &entry);
	zassert_ok(fs_closedir(&dir), "closedir failed");
	zassert_equal(ret, -ENAMETOOLONG, "overlong entry name was not rejected");
}
