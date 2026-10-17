/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/fs/fs.h>
#include <zephyr/sys/clock.h>
#include <zephyr/ztest.h>

#include "utils.h"

#define TS_FILE "/sml/stamped.txt"
#define TS_DIR  "/sml/stamped.d"

#ifdef CONFIG_FS_DIRENT_EXT
static void set_clock(time_t sec)
{
	struct timespec ts = {.tv_sec = sec};

	zassert_ok(sys_clock_settime(SYS_CLOCK_REALTIME, &ts));
}

static void stat_times(const char *path, struct timespec *atime, struct timespec *mtime)
{
	struct fs_dirent ent;

	zassert_ok(fs_stat(path, &ent), "stat %s", path);
	*atime = ent.atime;
	*mtime = ent.mtime;
}
#endif /* CONFIG_FS_DIRENT_EXT */

ZTEST(ext2tests, test_timestamps)
{
#ifdef CONFIG_FS_DIRENT_EXT
	const time_t t0 = 1700000000;
	const struct timespec times[2] = {
		{.tv_sec = 1600000000},
		{.tv_sec = 1600000100},
	};
	struct fs_file_t file;
	struct fs_dir_t dir;
	struct fs_dirent ent;
	struct timespec atime, mtime;
	bool seen = false;

	zassert_ok(fs_mount(&testfs_mnt));

	/* creation stamps the new inode with the current time */
	set_clock(t0);
	fs_file_t_init(&file);
	zassert_ok(fs_open(&file, TS_FILE, FS_O_CREATE | FS_O_RDWR));
	stat_times(TS_FILE, &atime, &mtime);
	zassert_true(atime.tv_sec >= t0 && atime.tv_sec < t0 + 60, "atime %lld", (long long)atime.tv_sec);
	zassert_true(mtime.tv_sec >= t0 && mtime.tv_sec < t0 + 60, "mtime %lld", (long long)mtime.tv_sec);

	/* a write moves the modification time forward, and close puts it on disk */
	set_clock(t0 + 100);
	zassert_equal(fs_write(&file, "abc", 3), 3);
	zassert_ok(fs_close(&file));
	stat_times(TS_FILE, &atime, &mtime);
	zassert_true(mtime.tv_sec >= t0 + 100, "mtime %lld", (long long)mtime.tv_sec);

	/* explicit times are stored as given and reported by stat and readdir alike */
	zassert_ok(fs_utime(TS_FILE, times));
	stat_times(TS_FILE, &atime, &mtime);
	zassert_equal(atime.tv_sec, times[0].tv_sec);
	zassert_equal(mtime.tv_sec, times[1].tv_sec);

	fs_dir_t_init(&dir);
	zassert_ok(fs_opendir(&dir, "/sml"));
	while (fs_readdir(&dir, &ent) == 0 && ent.name[0] != '\0') {
		if (strcmp(ent.name, "stamped.txt") == 0) {
			zassert_equal(ent.mtime.tv_sec, times[1].tv_sec);
			seen = true;
		}
	}
	zassert_ok(fs_closedir(&dir));
	zassert_true(seen, "readdir did not list the file");

	/* a directory is stamped at creation, and a new entry modifies it */
	set_clock(t0 + 200);
	zassert_ok(fs_mkdir(TS_DIR));
	stat_times(TS_DIR, &atime, &mtime);
	zassert_true(mtime.tv_sec >= t0 + 200, "dir mtime %lld", (long long)mtime.tv_sec);

	set_clock(t0 + 300);
	fs_file_t_init(&file);
	zassert_ok(fs_open(&file, TS_DIR "/entry", FS_O_CREATE | FS_O_WRITE));
	zassert_ok(fs_close(&file));
	stat_times(TS_DIR, &atime, &mtime);
	zassert_true(mtime.tv_sec >= t0 + 300, "dir mtime %lld", (long long)mtime.tv_sec);

	/* the times survive a remount */
	zassert_ok(fs_unmount(&testfs_mnt));
	zassert_ok(fs_mount(&testfs_mnt));
	stat_times(TS_FILE, &atime, &mtime);
	zassert_equal(atime.tv_sec, times[0].tv_sec);
	zassert_equal(mtime.tv_sec, times[1].tv_sec);

	zassert_ok(fs_utime(TS_FILE, NULL));
	stat_times(TS_FILE, &atime, &mtime);
	zassert_true(mtime.tv_sec >= t0 + 300, "mtime %lld", (long long)mtime.tv_sec);

	zassert_equal(fs_utime("/sml/nope", times), -ENOENT);
#else
	ztest_test_skip();
#endif /* CONFIG_FS_DIRENT_EXT */
}
