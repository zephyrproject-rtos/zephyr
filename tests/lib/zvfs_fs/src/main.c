/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/zvfs.h>
#include <zephyr/sys/zvfs_fs.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

/* zvfs_fs_priv.h is a ZVFS-private header; this white-box test reaches into it
 * for zvfs_resolve_path() via the include directory added in CMakeLists.txt.
 */
#include <zvfs_fs_priv.h>

#define MNT       "/RAM:"
#define TDIR      MNT "/tdir"
#define TFILE     MNT "/tfile.txt"
#define TSUB      TDIR "/sub.txt"
#define TNOENT    MNT "/nope.txt"
#define TCONTENT  "The quick brown fox jumps over the lazy dog!"

#ifdef CONFIG_USERSPACE
extern struct k_mem_partition zvfs_dir_partition;
#endif

static FATFS fat_fs;
static struct fs_mount_t fs_mnt = {
	.type = FS_FATFS,
	.mnt_point = MNT,
	.fs_data = &fat_fs,
};

static void tree_reset(void)
{
	int fd;

	(void)zvfs_unlink(TSUB);
	(void)zvfs_rmdir(TDIR);
	(void)zvfs_unlink(TFILE);
	(void)zvfs_unlink(TNOENT);

	fd = zvfs_open(TFILE, ZVFS_O_RDWR | ZVFS_O_CREAT | ZVFS_O_TRUNC, 0600);
	zassert_true(fd >= 0);
	zassert_equal(zvfs_write(fd, TCONTENT, strlen(TCONTENT)), strlen(TCONTENT));
	zassert_ok(zvfs_close(fd));

	zassert_ok(zvfs_mkdir(TDIR, 0777));
	fd = zvfs_open(TSUB, ZVFS_O_WRONLY | ZVFS_O_CREAT, 0600);
	zassert_true(fd >= 0);
	zassert_ok(zvfs_close(fd));
}

static void *suite_setup(void)
{
	zassert_ok(fs_mount(&fs_mnt));
#ifdef CONFIG_USERSPACE
	zassert_ok(k_mem_domain_add_partition(&k_mem_domain_default, &zvfs_dir_partition));
#endif
	return NULL;
}

static void suite_before(void *arg)
{
	ARG_UNUSED(arg);
	tree_reset();
	zassert_ok(zvfs_chdir("/"));
}

static void suite_teardown(void *arg)
{
	ARG_UNUSED(arg);
	(void)fs_unmount(&fs_mnt);
}

ZTEST_SUITE(zvfs_fs, NULL, suite_setup, suite_before, NULL, suite_teardown);

ZTEST_USER(zvfs_fs, test_zvfs_open)
{
	int fd;

	/* O_EXCL on an existing file fails with EEXIST */
	errno = 0;
	fd = zvfs_open(TFILE, ZVFS_O_RDWR | ZVFS_O_CREAT | ZVFS_O_EXCL, 0600);
	zassert_equal(fd, -1);
	zassert_equal(errno, EEXIST);

	/* O_EXCL on a new name succeeds */
	fd = zvfs_open(TNOENT, ZVFS_O_RDWR | ZVFS_O_CREAT | ZVFS_O_EXCL, 0600);
	zassert_true(fd >= 0);
	zassert_ok(zvfs_close(fd));
	zassert_ok(zvfs_unlink(TNOENT));

	/* O_DIRECTORY yields a directory fd that rejects read and write */
	fd = zvfs_open(TDIR, ZVFS_O_RDONLY | ZVFS_O_DIRECTORY, 0);
	zassert_true(fd >= 0);
	errno = 0;
	zassert_equal(zvfs_write(fd, "x", 1), -1);
	zassert_equal(errno, EISDIR);
	zassert_ok(zvfs_close(fd));
}

ZTEST_USER(zvfs_fs, test_zvfs_unlink)
{
	errno = 0;
	zassert_equal(zvfs_unlink(TDIR), -1);
	zassert_equal(errno, EISDIR, "expected EISDIR got %d", errno);

	zassert_ok(zvfs_unlink(TFILE));
	errno = 0;
	zassert_equal(zvfs_unlink(TFILE), -1);
	zassert_equal(errno, ENOENT);
}

ZTEST_USER(zvfs_fs, test_zvfs_rename)
{
	struct zvfs_stat st;

	zassert_ok(zvfs_rename(TFILE, TNOENT));
	zassert_ok(zvfs_stat(TNOENT, &st));
	errno = 0;
	zassert_equal(zvfs_stat(TFILE, &st), -1);
	zassert_equal(errno, ENOENT);
	zassert_ok(zvfs_rename(TNOENT, TFILE));
}

ZTEST_USER(zvfs_fs, test_zvfs_remove)
{
	/* remove() semantics: files and empty directories both go */
	zassert_ok(zvfs_remove(TSUB));
	zassert_ok(zvfs_remove(TDIR));
	errno = 0;
	zassert_equal(zvfs_remove(TDIR), -1);
	zassert_equal(errno, ENOENT);
}

ZTEST_USER(zvfs_fs, test_zvfs_stat)
{
	struct zvfs_stat st;

	zassert_ok(zvfs_stat(TFILE, &st));
	zassert_equal(st.size, strlen(TCONTENT));
	zassert_equal(st.mode & ZVFS_MODE_IFMT, ZVFS_MODE_IFREG);
	zassert_true((st.mode & ZVFS_S_IWUSR) != 0, "writable mount reports write bit");
	zassert_equal(st.nlink, 1);

	zassert_ok(zvfs_stat(TDIR, &st));
	zassert_equal(st.mode & ZVFS_MODE_IFMT, ZVFS_MODE_IFDIR);
	zassert_true((st.mode & ZVFS_S_IXUSR) != 0, "directories are searchable");

	errno = 0;
	zassert_equal(zvfs_stat(TNOENT, &st), -1);
	zassert_equal(errno, ENOENT);
}

ZTEST_USER(zvfs_fs, test_zvfs_statvfs)
{
	struct zvfs_statvfs sv;

	zassert_ok(zvfs_statvfs(TFILE, &sv));
	zassert_true(sv.f_bsize > 0);
	zassert_true(sv.f_blocks > 0);
	zassert_true((sv.f_flag & ZVFS_ST_NOSUID) != 0);
	zassert_true((sv.f_flag & ZVFS_ST_RDONLY) == 0);
	zassert_equal(sv.f_namemax, MAX_FILE_NAME);

	/* the root is a read-only directory of mount points without blocks of its own */
	zassert_ok(zvfs_statvfs("/", &sv));
	zassert_true(sv.f_bsize > 0);
	zassert_equal(sv.f_blocks, 0);
	zassert_true(sv.f_files >= 1);
	zassert_true((sv.f_flag & ZVFS_ST_RDONLY) != 0);
	zassert_equal(sv.f_namemax, MAX_FILE_NAME);
}

ZTEST_USER(zvfs_fs, test_zvfs_fstat)
{
	struct zvfs_stat st;
	struct zvfs_stat by_path;
	int fd;

	/* a file: the type, length and mode that stat() of the path reports */
	fd = zvfs_open(TFILE, ZVFS_O_RDONLY, 0);
	zassert_true(fd >= 0);
	zassert_ok(zvfs_fstat(fd, &st));
	zassert_ok(zvfs_close(fd));
	zassert_equal(st.mode & ZVFS_MODE_IFMT, ZVFS_MODE_IFREG);
	zassert_equal(st.size, (off_t)strlen(TCONTENT));
	zassert_ok(zvfs_stat(TFILE, &by_path));
	zassert_equal(st.mode, by_path.mode);

	/* a directory, through its descriptor */
	fd = zvfs_open(TDIR, ZVFS_O_RDONLY | ZVFS_O_DIRECTORY, 0);
	zassert_true(fd >= 0);
	zassert_ok(zvfs_fstat(fd, &st));
	zassert_ok(zvfs_close(fd));
	zassert_equal(st.mode & ZVFS_MODE_IFMT, ZVFS_MODE_IFDIR);
	zassert_equal(st.mode & ZVFS_S_IXUSR, ZVFS_S_IXUSR);
	zassert_ok(zvfs_stat(TDIR, &by_path));
	zassert_equal(st.mode, by_path.mode);

	/* not a descriptor */
	errno = 0;
	zassert_equal(zvfs_fstat(-1, &st), -1);
	zassert_equal(errno, EBADF);
}

ZTEST_USER(zvfs_fs, test_zvfs_exe_resolve)
{
	char path[CONFIG_ZVFS_PATH_MAX];
	char small[sizeof("tdir/sub")];
	struct zvfs_stat st;
	int fd;

	/* the list is colon-separated, so a FAT mount name is reached through the working directory */
	zassert_ok(zvfs_chdir(MNT));

	/* the second directory holds the file: its path comes back with the descriptor */
	fd = zvfs_exe_resolve("nope.txt:tdir", "sub.txt", path, sizeof(path));
	zassert_true(fd >= 0, "resolve failed: %d", fd);
	zassert_str_equal(path, "tdir/sub.txt");
	zassert_ok(zvfs_fstat(fd, &st));
	zassert_equal(st.mode & ZVFS_MODE_IFMT, ZVFS_MODE_IFREG);
	zassert_ok(zvfs_close(fd));

	/* an empty entry is the working directory */
	fd = zvfs_exe_resolve("nope.txt:", "tfile.txt", path, sizeof(path));
	zassert_true(fd >= 0, "resolve in the working directory failed: %d", fd);
	zassert_str_equal(path, "tfile.txt");
	zassert_ok(zvfs_close(fd));

	/* nothing of that name anywhere */
	zassert_equal(zvfs_exe_resolve("tdir:.", "nope.txt", path, sizeof(path)), -ENOENT);
	/* a directory of that name is not an executable file */
	zassert_equal(zvfs_exe_resolve(".", "tdir", path, sizeof(path)), -EPERM);
	/* the path does not fit the buffer */
	zassert_equal(zvfs_exe_resolve("tdir", "sub.txt", small, sizeof(small)), -ERANGE);
	/* bad arguments */
	zassert_equal(zvfs_exe_resolve(NULL, "sub.txt", path, sizeof(path)), -EINVAL);
	zassert_equal(zvfs_exe_resolve("tdir", NULL, path, sizeof(path)), -EINVAL);
	zassert_equal(zvfs_exe_resolve("tdir", "", path, sizeof(path)), -EINVAL);
	zassert_equal(zvfs_exe_resolve("tdir", "a/b", path, sizeof(path)), -EINVAL);
	zassert_equal(zvfs_exe_resolve("tdir", "sub.txt", NULL, sizeof(path)), -EINVAL);
	zassert_equal(zvfs_exe_resolve("tdir", "sub.txt", NULL, 0), -EINVAL);
	zassert_equal(zvfs_exe_resolve("tdir", "sub.txt", path, 0), -EINVAL);

	zassert_ok(zvfs_chdir("/"));
}

ZTEST_USER(zvfs_fs, test_zvfs_fstatvfs)
{
	int fd;
	struct zvfs_statvfs sv, sv2;

	fd = zvfs_open(TFILE, ZVFS_O_RDONLY, 0);
	zassert_true(fd >= 0);
	zassert_ok(zvfs_fstatvfs(fd, &sv));
	zassert_ok(zvfs_statvfs(TFILE, &sv2));
	zassert_equal(sv.f_bsize, sv2.f_bsize);
	zassert_equal(sv.f_blocks, sv2.f_blocks);
	zassert_ok(zvfs_close(fd));

	/* a descriptor of the root directory answers like statvfs("/") */
	fd = zvfs_open("/", ZVFS_O_RDONLY | ZVFS_O_DIRECTORY, 0);
	zassert_true(fd >= 0);
	zassert_ok(zvfs_fstatvfs(fd, &sv));
	zassert_ok(zvfs_statvfs("/", &sv2));
	zassert_equal(sv.f_blocks, 0);
	zassert_equal(sv.f_flag, sv2.f_flag);
	zassert_equal(sv.f_files, sv2.f_files);
	zassert_ok(zvfs_close(fd));
}

ZTEST_USER(zvfs_fs, test_zvfs_mkdir)
{
	struct zvfs_stat st;

	zassert_ok(zvfs_mkdir(MNT "/newdir", 0777));
	zassert_ok(zvfs_stat(MNT "/newdir", &st));
	zassert_equal(st.mode & ZVFS_MODE_IFMT, ZVFS_MODE_IFDIR);

	errno = 0;
	zassert_equal(zvfs_mkdir(MNT "/newdir", 0777), -1);
	zassert_equal(errno, EEXIST);
	zassert_ok(zvfs_rmdir(MNT "/newdir"));
}

ZTEST_USER(zvfs_fs, test_zvfs_rmdir)
{
	errno = 0;
	zassert_equal(zvfs_rmdir(TFILE), -1);
	zassert_equal(errno, ENOTDIR, "expected ENOTDIR got %d", errno);

	/* FatFs reports a non-empty directory as EACCES rather than ENOTEMPTY */
	errno = 0;
	zassert_equal(zvfs_rmdir(TDIR), -1);
	zassert_true(errno == ENOTEMPTY || errno == EACCES, "unexpected errno %d", errno);

	zassert_ok(zvfs_unlink(TSUB));
	zassert_ok(zvfs_rmdir(TDIR));
}

ZTEST_USER(zvfs_fs, test_zvfs_truncate)
{
	struct zvfs_stat st;

	zassert_ok(zvfs_truncate(TFILE, 4));
	zassert_ok(zvfs_stat(TFILE, &st));
	zassert_equal(st.size, 4);

	errno = 0;
	zassert_equal(zvfs_truncate(TFILE, -1), -1);
	zassert_equal(errno, EINVAL);

	errno = 0;
	zassert_equal(zvfs_truncate(TNOENT, 0), -1);
	zassert_equal(errno, ENOENT);
}

ZTEST_USER(zvfs_fs, test_zvfs_access)
{
	zassert_ok(zvfs_access(TFILE, ZVFS_F_OK));
	zassert_ok(zvfs_access(TFILE, ZVFS_R_OK | ZVFS_W_OK));
	zassert_ok(zvfs_access(TDIR, ZVFS_X_OK));
	/* no execute permission exists: every file is runnable */
	zassert_ok(zvfs_access(TFILE, ZVFS_X_OK));

	errno = 0;
	zassert_equal(zvfs_access(TNOENT, ZVFS_F_OK), -1);
	zassert_equal(errno, ENOENT);

	errno = 0;
	zassert_equal(zvfs_access(TFILE, 0x1000), -1);
	zassert_equal(errno, EINVAL);
}

ZTEST_USER(zvfs_fs, test_zvfs_utime)
{
	/* FatFs without FF_USE_CHMOD reports ENOTSUP through the core CHECKIF */
	int rc = zvfs_utime(TFILE, NULL);

	if (rc < 0) {
		zassert_equal(errno, ENOTSUP, "expected ENOTSUP got %d", errno);
	}

	errno = 0;
	zassert_equal(zvfs_utime(TNOENT, NULL), -1);
	zassert_true(errno == ENOENT || errno == ENOTSUP);
}

ZTEST_USER(zvfs_fs, test_zvfs_futime)
{
	const struct timespec times[2] = {
		{.tv_sec = 1700000000},
		{.tv_sec = 1700000100},
	};
	struct zvfs_stat st;
	int fd = zvfs_open(TFILE, ZVFS_O_RDWR, 0);
	int rc;

	zassert_true(fd >= 0);

	/* FatFs without FF_USE_CHMOD reports ENOTSUP through the core CHECKIF */
	rc = zvfs_futime(fd, times);
	if (rc < 0) {
		zassert_equal(errno, ENOTSUP, "expected ENOTSUP got %d", errno);
	} else if (IS_ENABLED(CONFIG_FS_DIRENT_EXT)) {
		/* what was set through the descriptor is what the descriptor reports */
		zassert_ok(zvfs_fstat(fd, &st));
		zassert_equal(st.mtime.tv_sec, times[1].tv_sec);
		zassert_ok(zvfs_stat(TFILE, &st));
		zassert_equal(st.mtime.tv_sec, times[1].tv_sec);
	}

	rc = zvfs_futime(fd, NULL);
	if (rc < 0) {
		zassert_equal(errno, ENOTSUP, "expected ENOTSUP got %d", errno);
	}
	zassert_ok(zvfs_close(fd));

	/* a directory descriptor is a descriptor too */
	fd = zvfs_open(TDIR, ZVFS_O_RDONLY | ZVFS_O_DIRECTORY, 0);
	zassert_true(fd >= 0);
	rc = zvfs_futime(fd, NULL);
	if (rc < 0) {
		zassert_equal(errno, ENOTSUP, "expected ENOTSUP got %d", errno);
	}
	zassert_ok(zvfs_close(fd));

	errno = 0;
	zassert_equal(zvfs_futime(fd, NULL), -1);
	zassert_equal(errno, EBADF);
}

ZTEST_USER(zvfs_fs, test_zvfs_opendir)
{
	struct zvfs_dirstream *d, *d2;

	d = zvfs_opendir(TDIR);
	zassert_not_null(d);
	zassert_true(d->fd >= 0);

	/* pool exhausts at CONFIG_ZVFS_DIR_MAX */
	d2 = zvfs_opendir(MNT);
	zassert_not_null(d2);
	errno = 0;
	zassert_is_null(zvfs_opendir(MNT));
	zassert_equal(errno, ENOMEM);

	zassert_ok(zvfs_close(d2->fd));
	zassert_ok(zvfs_close(d->fd));

	/* FatFs reports a file-as-directory open as ENOENT rather than ENOTDIR */
	errno = 0;
	zassert_is_null(zvfs_opendir(TFILE));
	zassert_true(errno == ENOTDIR || errno == ENOENT, "unexpected errno %d", errno);

	errno = 0;
	zassert_is_null(zvfs_opendir(TNOENT));
	zassert_true(errno == ENOENT || errno == ENOTDIR);
}

ZTEST_USER(zvfs_fs, test_zvfs_readdir)
{
	struct zvfs_dirstream *d;
	int rc;
	bool saw_sub = false;

	d = zvfs_opendir(TDIR);
	zassert_not_null(d);

	for (;;) {
		errno = 0xdead;
		rc = zvfs_readdir(d->fd, &d->ent);
		zassert_true(rc >= 0, "readdir failed: %d", errno);
		if (rc == 1) {
			/* end of directory leaves errno untouched */
			zassert_equal(errno, 0xdead);
			break;
		}
		/* 8.3 FAT names are stored upper-case */
		if (strcmp(d->ent.d_name, "sub.txt") == 0 ||
		    strcmp(d->ent.d_name, "SUB.TXT") == 0) {
			saw_sub = true;
			zassert_equal(d->ent.d_type, ZVFS_MODE_IFREG);
		}
	}
	zassert_true(saw_sub, "sub.txt not listed");

	/* readdir on a non-directory fd reports ENOTDIR */
	rc = zvfs_open(TFILE, ZVFS_O_RDONLY, 0);
	zassert_true(rc >= 0);
	errno = 0;
	zassert_equal(zvfs_readdir(rc, &d->ent), -1);
	zassert_equal(errno, ENOTDIR);
	zassert_ok(zvfs_close(rc));

	zassert_ok(zvfs_close(d->fd));
}

ZTEST_USER(zvfs_fs, test_zvfs_rewinddir)
{
	struct zvfs_dirstream *d;
	int entries = 0;

	d = zvfs_opendir(TDIR);
	zassert_not_null(d);

	while (zvfs_readdir(d->fd, &d->ent) == 0) {
		entries++;
	}
	zassert_true(entries >= 1);

	zassert_ok(zvfs_rewinddir(d->fd));
	zassert_equal(zvfs_readdir(d->fd, &d->ent), 0, "rewound stream lists entries again");

	zassert_ok(zvfs_close(d->fd));
}

ZTEST_USER(zvfs_fs, test_zvfs_chdir)
{
	int fd;
	struct zvfs_stat st;

	zassert_ok(zvfs_chdir(TDIR));

	/* relative resolution against the new working directory */
	zassert_ok(zvfs_stat("sub.txt", &st));
	fd = zvfs_open("new.txt", ZVFS_O_WRONLY | ZVFS_O_CREAT, 0600);
	zassert_true(fd >= 0);
	zassert_ok(zvfs_close(fd));
	zassert_ok(zvfs_stat(TDIR "/new.txt", &st));
	zassert_ok(zvfs_unlink("new.txt"));

	/* dot and dot-dot */
	zassert_ok(zvfs_stat("./sub.txt", &st));
	zassert_ok(zvfs_stat(".." "/tfile.txt", &st));

	errno = 0;
	zassert_equal(zvfs_chdir(TFILE), -1);
	zassert_equal(errno, ENOTDIR);

	errno = 0;
	zassert_equal(zvfs_chdir(TNOENT), -1);
	zassert_equal(errno, ENOENT);

	zassert_ok(zvfs_chdir("/"));
}

ZTEST_USER(zvfs_fs, test_zvfs_fchdir)
{
	int fd;
	char buf[CONFIG_ZVFS_PATH_MAX];
	struct zvfs_stat st;

	fd = zvfs_open(TDIR, ZVFS_O_RDONLY | ZVFS_O_DIRECTORY, 0);
	zassert_true(fd >= 0);

	zassert_ok(zvfs_fchdir(fd));
	zassert_ok(zvfs_getcwd(buf, sizeof(buf)));
	zassert_equal(strcmp(buf, TDIR), 0, "cwd is %s", buf);
	zassert_ok(zvfs_stat("sub.txt", &st));
	zassert_ok(zvfs_close(fd));

	/* a regular file fd is not a directory */
	fd = zvfs_open(TFILE, ZVFS_O_RDONLY, 0);
	zassert_true(fd >= 0);
	errno = 0;
	zassert_equal(zvfs_fchdir(fd), -1);
	zassert_equal(errno, ENOTDIR);
	zassert_ok(zvfs_close(fd));

	zassert_ok(zvfs_chdir("/"));
}

ZTEST_USER(zvfs_fs, test_zvfs_getcwd)
{
	char buf[CONFIG_ZVFS_PATH_MAX];

	zassert_ok(zvfs_getcwd(buf, sizeof(buf)));
	zassert_equal(strcmp(buf, "/"), 0);

	zassert_ok(zvfs_chdir(TDIR));
	zassert_ok(zvfs_getcwd(buf, sizeof(buf)));
	zassert_equal(strcmp(buf, TDIR), 0);

	/* exact fit succeeds, one short reports ERANGE */
	zassert_ok(zvfs_getcwd(buf, strlen(TDIR) + 1));
	errno = 0;
	zassert_equal(zvfs_getcwd(buf, strlen(TDIR)), -1);
	zassert_equal(errno, ERANGE);

	zassert_ok(zvfs_chdir("/"));
}

/* zvfs_resolve_path() is kernel-internal; exercise the table from kernel mode */
ZTEST(zvfs_fs, test_zvfs_resolve_path)
{
	char out[CONFIG_ZVFS_PATH_MAX];
	static const struct {
		const char *cwd;
		const char *in;
		const char *want;
	} table[] = {
		{"/", "/a/b", "/a/b"},
		{"/", "//a//b//", "/a/b"},
		{"/", "/a/./b", "/a/b"},
		{"/", "/a/../b", "/b"},
		{"/", "/a/../../b", "/b"},
		{"/", "/..", "/"},
		{MNT, "a/b", MNT "/a/b"},
		{MNT, "./a", MNT "/a"},
		{MNT, "..", "/"},
		{MNT "/tdir", "../x", MNT "/x"},
	};

	for (size_t i = 0; i < ARRAY_SIZE(table); i++) {
		zassert_ok(zvfs_chdir(table[i].cwd), "chdir(%s) failed", table[i].cwd);
		zassert_ok(zvfs_resolve_path(table[i].in, out, sizeof(out)));
		zassert_equal(strcmp(out, table[i].want), 0, "resolve(%s) = %s, want %s",
			      table[i].in, out, table[i].want);
	}

	zassert_equal(zvfs_resolve_path(NULL, out, sizeof(out)), -EFAULT);
	zassert_equal(zvfs_resolve_path("", out, sizeof(out)), -ENOENT);
	zassert_equal(zvfs_resolve_path("/a/b", out, 4), -ENAMETOOLONG);

	zassert_ok(zvfs_chdir("/"));
}

ZTEST_USER(zvfs_fs, test_zvfs_path_max)
{
	char longpath[CONFIG_ZVFS_PATH_MAX + 8];
	struct zvfs_stat st;

	/* over-long paths are an error to report, not a fault */
	memset(longpath, 'a', sizeof(longpath) - 1);
	longpath[0] = '/';
	longpath[sizeof(longpath) - 1] = '\0';

	errno = 0;
	zassert_equal(zvfs_stat(longpath, &st), -1);
	zassert_equal(errno, ENAMETOOLONG, "expected ENAMETOOLONG got %d", errno);
}

ZTEST_USER(zvfs_fs, test_zvfs_write)
{
	int fd;

	fd = zvfs_open(TFILE, ZVFS_O_WRONLY, 0);
	zassert_true(fd >= 0);
	zassert_equal(zvfs_write(fd, "x", 1), 1);
	zassert_ok(zvfs_close(fd));

#ifdef CONFIG_ZVFS_RESERVE_STDIO_FDS
	/* stdout is shared by all threads, including user threads */
	zassert_true(zvfs_write(1, "\n", 1) >= 0);
#endif
}

#if defined(CONFIG_USERSPACE) && defined(CONFIG_ZTEST_FATAL_HOOK)
ZTEST_USER(zvfs_fs, test_zvfs_stat_fault)
{
	struct zvfs_stat st;

	/* an unmapped path pointer faults in the verifier */
	ztest_set_fault_valid(true);
	(void)zvfs_stat((const char *)0xFFFFF000UL, &st);
	ztest_test_fail();
}

static K_THREAD_STACK_DEFINE(fault_child_stack, 2048);
static struct k_thread fault_child_thread;

static void ungranted_read_entry(void *p1, void *p2, void *p3)
{
	char c;

	/* the fault must be armed by the faulting thread itself */
	ztest_set_fault_valid(true);
	(void)zvfs_read(POINTER_TO_INT(p1), &c, 1);
	zassert_unreachable("ungranted fd access did not fault");
}

static void spawn_ungranted_reader(int fd)
{
	/* no K_INHERIT_PERMS: the child holds no grant on the fd slot */
	k_thread_create(&fault_child_thread, fault_child_stack,
			K_THREAD_STACK_SIZEOF(fault_child_stack), ungranted_read_entry,
			INT_TO_POINTER(fd), NULL, NULL,
			k_thread_priority_get(k_current_get()), K_USER, K_NO_WAIT);
	zassert_ok(k_thread_join(&fault_child_thread, K_FOREVER));
}

ZTEST(zvfs_fs, test_zvfs_read_fault)
{
	int fd;

	fd = zvfs_open(TFILE, ZVFS_O_RDONLY, 0);
	zassert_true(fd >= 0);

	spawn_ungranted_reader(fd);

	zassert_ok(zvfs_close(fd));
}

ZTEST(zvfs_fs, test_zvfs_close_fault)
{
	int fd, fd2;
	void *kobj;

	fd = zvfs_open(TFILE, ZVFS_O_RDONLY, 0);
	zassert_true(fd >= 0);
	kobj = zvfs_test_fd_entry_kobj(fd);
	zassert_not_null(kobj);

	/* a public slot (e.g. stdio) must not leak K_OBJ_FLAG_PUBLIC through
	 * close into whatever fd is reserved into the slot next
	 */
	k_object_access_all_grant(kobj);
	zassert_ok(zvfs_close(fd));

	fd2 = zvfs_open(TFILE, ZVFS_O_RDONLY, 0);
	zassert_equal(fd2, fd, "expected lowest-slot reuse");

	spawn_ungranted_reader(fd2);

	zassert_ok(zvfs_close(fd2));
}
#endif
