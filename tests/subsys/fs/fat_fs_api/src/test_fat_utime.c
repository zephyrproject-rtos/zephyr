/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fat.h"

#if defined(CONFIG_FS_DIRENT_EXT) && defined(CONFIG_FS_FATFS_FF_USE_CHMOD)

/* 2026-08-07 12:34:56 UTC, even seconds so the FAT 2 s granularity is exact */
#define TEST_MTIME 1786106096LL

static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.fs_data = &fat_fs,
};

ZTEST(fat_fs_basic, test_fat_utime_rewinddir)
{
	struct fs_file_t fs;
	struct fs_dir_t dir;
	struct fs_dirent entry;
	const struct timespec times[2] = {
		{.tv_sec = TEST_MTIME},
		{.tv_sec = TEST_MTIME},
	};
	int seen;

	fs_file_t_init(&fs);
	fs_dir_t_init(&dir);
	zassert_equal(fs_mount(&fatfs_mnt), 0);
	zassert_equal(fs_open(&fs, FATFS_MNTP"/utime.txt", FS_O_CREATE), 0);
	zassert_equal(fs_close(&fs), 0);

	/* explicit timestamp round-trips through the FAT date conversion */
	zassert_equal(fs_utime(FATFS_MNTP"/utime.txt", times), 0);
	zassert_equal(fs_stat(FATFS_MNTP"/utime.txt", &entry), 0);
	zassert_equal(entry.mtime.tv_sec, TEST_MTIME,
		      "expected %lld got %lld", (long long)TEST_MTIME,
		      (long long)entry.mtime.tv_sec);
	zassert_equal(entry.mtime.tv_nsec, 0);
	zassert_equal(entry.atime.tv_sec, entry.mtime.tv_sec);

	/* pre-epoch-of-FAT timestamps are rejected */
	zassert_equal(fs_utime(FATFS_MNTP"/utime.txt",
			       (const struct timespec[2]){{.tv_sec = 0}, {.tv_sec = 0}}),
		      -EINVAL);

	/* readdir reports the same timestamp, and rewinddir restarts the walk */
	zassert_equal(fs_opendir(&dir, FATFS_MNTP"/"), 0);
	for (seen = 0;; seen++) {
		zassert_equal(fs_readdir(&dir, &entry), 0);
		if (entry.name[0] == '\0') {
			break;
		}
		if (strcmp(entry.name, "utime.txt") == 0) {
			zassert_equal(entry.mtime.tv_sec, TEST_MTIME);
		}
	}
	zassert_true(seen >= 1);
	zassert_equal(fs_rewinddir(&dir), 0);
	zassert_equal(fs_readdir(&dir, &entry), 0);
	zassert_not_equal(entry.name[0], '\0',
			  "rewound directory should list entries again");
	zassert_equal(fs_closedir(&dir), 0);

	zassert_equal(fs_unlink(FATFS_MNTP"/utime.txt"), 0);
	zassert_true(fs_unmount(&fatfs_mnt) >= 0);
}

#endif /* CONFIG_FS_DIRENT_EXT && CONFIG_FS_FATFS_FF_USE_CHMOD */
