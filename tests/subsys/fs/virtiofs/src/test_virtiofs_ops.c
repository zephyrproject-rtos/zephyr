/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
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
