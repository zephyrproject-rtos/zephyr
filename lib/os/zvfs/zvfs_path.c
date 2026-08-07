/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Path-based ZVFS operations over a mounted Zephyr file system.
 */

#include <errno.h>

#include <zephyr/fs/fs.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/zvfs_fs.h>

#include "zvfs_fs_priv.h"

#ifdef CONFIG_USERSPACE
int zvfs_copy_user_path(char *dst, const char *src)
{
	int err;

	if (src == NULL) {
		errno = EFAULT;
		return -1;
	}

	if (k_usermode_string_nlen(src, CONFIG_ZVFS_PATH_MAX, &err) >= CONFIG_ZVFS_PATH_MAX) {
		K_OOPS(err != 0);
		errno = ENAMETOOLONG;
		return -1;
	}
	K_OOPS(err != 0);
	K_OOPS(k_usermode_string_copy(dst, src, CONFIG_ZVFS_PATH_MAX));

	return 0;
}
#endif /* CONFIG_USERSPACE */

/*
 * Bridge an fs_*() negative-errno return to the -1/errno convention.
 *
 * TODO: the ZVFS functions themselves should return a negative error number
 * directly instead of setting errno, so callers (and the syscall layer) are not
 * forced through thread-local errno. Until that refactor lands, this helper
 * converts at the boundary.
 */
static int zvfs_err(int res)
{
	if (res < 0) {
		errno = -res;
		return -1;
	}
	return 0;
}

/* Zephyr's file system subsystem exposes no permission bits; synthesize a fixed mode. */
uint32_t zvfs_synth_mode(bool is_dir, bool rdonly)
{
	uint32_t mode;

	if (is_dir) {
		/* directories are always searchable */
		mode = ZVFS_MODE_IFDIR | ZVFS_S_IRUSR | ZVFS_S_IXUSR | ZVFS_S_IRGRP |
		       ZVFS_S_IXGRP | ZVFS_S_IROTH | ZVFS_S_IXOTH;
	} else {
		/* no execute permission exists: every file is runnable */
		mode = ZVFS_MODE_IFREG | ZVFS_S_IRUSR | ZVFS_S_IXUSR | ZVFS_S_IRGRP |
		       ZVFS_S_IXGRP | ZVFS_S_IROTH | ZVFS_S_IXOTH;
	}

	if (!rdonly) {
		mode |= ZVFS_S_IWUSR | ZVFS_S_IWGRP | ZVFS_S_IWOTH;
	}

	return mode;
}

static bool zvfs_mount_rdonly(const char *path)
{
	uint8_t flags = 0;

	if (fs_mount_flags(path, &flags) < 0) {
		return false;
	}

	return (flags & FS_MOUNT_FLAG_READ_ONLY) != 0;
}

int z_impl_zvfs_unlink(const char *path)
{
	int res;
	struct fs_dirent entry;

	res = fs_stat(path, &entry);
	if (res == 0 && entry.type == FS_DIR_ENTRY_DIR) {
		errno = EISDIR;
		return -1;
	}

	return zvfs_err(fs_unlink(path));
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_unlink(const char *path)
{
	char kpath[CONFIG_ZVFS_PATH_MAX];

	if (zvfs_copy_user_path(kpath, path) < 0) {
		return -1;
	}

	return z_impl_zvfs_unlink(kpath);
}
#include <zephyr/syscalls/zvfs_unlink_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_rename(const char *old, const char *newp)
{
	return zvfs_err(fs_rename(old, newp));
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_rename(const char *old, const char *newp)
{
	char kold[CONFIG_ZVFS_PATH_MAX];
	char knew[CONFIG_ZVFS_PATH_MAX];

	if (zvfs_copy_user_path(kold, old) < 0 || zvfs_copy_user_path(knew, newp) < 0) {
		return -1;
	}

	return z_impl_zvfs_rename(kold, knew);
}
#include <zephyr/syscalls/zvfs_rename_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_remove(const char *path)
{
	/* Zephyr fs_unlink() removes files and empty directories alike */
	return zvfs_err(fs_unlink(path));
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_remove(const char *path)
{
	char kpath[CONFIG_ZVFS_PATH_MAX];

	if (zvfs_copy_user_path(kpath, path) < 0) {
		return -1;
	}

	return z_impl_zvfs_remove(kpath);
}
#include <zephyr/syscalls/zvfs_remove_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_stat(const char *path, struct zvfs_stat *buf)
{
	int res;
	struct fs_dirent entry;
	struct fs_statvfs vfs;
	bool is_dir;

	res = fs_stat(path, &entry);
	if (res < 0) {
		return zvfs_err(res);
	}

	is_dir = (entry.type == FS_DIR_ENTRY_DIR);

	*buf = (struct zvfs_stat){0};
	buf->size = entry.size;
	buf->mode = zvfs_synth_mode(is_dir, zvfs_mount_rdonly(path));
	buf->nlink = 1;
#ifdef CONFIG_FS_DIRENT_EXT
	buf->atime = entry.atime;
	buf->mtime = entry.mtime;
	/* Zephyr file systems track no status-change time */
	buf->ctime = entry.mtime;
#endif

	/* block accounting is best-effort: not every backend supports statvfs */
	if (fs_statvfs(path, &vfs) == 0) {
		buf->blksize = vfs.f_bsize;
		buf->blocks = DIV_ROUND_UP(entry.size, 512);
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_stat(const char *path, struct zvfs_stat *buf)
{
	char kpath[CONFIG_ZVFS_PATH_MAX];

	K_OOPS(K_SYSCALL_MEMORY_WRITE(buf, sizeof(*buf)));
	if (zvfs_copy_user_path(kpath, path) < 0) {
		return -1;
	}

	return z_impl_zvfs_stat(kpath, buf);
}
#include <zephyr/syscalls/zvfs_stat_mrsh.c>
#endif /* CONFIG_USERSPACE */

void zvfs_statvfs_from_fs(struct zvfs_statvfs *buf, const struct fs_statvfs *vfs, bool rdonly)
{
	*buf = (struct zvfs_statvfs){0};
	buf->f_bsize = vfs->f_bsize;
	buf->f_frsize = vfs->f_frsize;
	buf->f_blocks = vfs->f_blocks;
	buf->f_bfree = vfs->f_bfree;
	buf->f_bavail = vfs->f_bfree;
	buf->f_flag = ZVFS_ST_NOSUID | (rdonly ? ZVFS_ST_RDONLY : 0);
	buf->f_namemax = MAX_FILE_NAME;
}

/*
 * The root is a read-only directory of mount points, not a file system: no blocks, one
 * "file" per mount, and the block size Linux reports for its pseudo file systems.
 */
void zvfs_statvfs_root(struct zvfs_statvfs *buf)
{
	const char *mnt;
	unsigned long mounts = 0;

	for (int i = 0; fs_readmount(&i, &mnt) == 0;) {
		mounts++;
	}
	*buf = (struct zvfs_statvfs){
		.f_bsize = 4096,
		.f_frsize = 4096,
		.f_files = mounts,
		.f_flag = ZVFS_ST_RDONLY | ZVFS_ST_NOSUID,
		.f_namemax = MAX_FILE_NAME,
	};
}

int z_impl_zvfs_statvfs(const char *path, struct zvfs_statvfs *buf)
{
	int res;
	struct fs_statvfs vfs;

	if ((path[0] == '/') && (path[1] == '\0')) {
		zvfs_statvfs_root(buf);
		return 0;
	}

	res = fs_statvfs(path, &vfs);
	if (res < 0) {
		return zvfs_err(res);
	}

	zvfs_statvfs_from_fs(buf, &vfs, zvfs_mount_rdonly(path));

	return 0;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_statvfs(const char *path, struct zvfs_statvfs *buf)
{
	char kpath[CONFIG_ZVFS_PATH_MAX];

	K_OOPS(K_SYSCALL_MEMORY_WRITE(buf, sizeof(*buf)));
	if (zvfs_copy_user_path(kpath, path) < 0) {
		return -1;
	}

	return z_impl_zvfs_statvfs(kpath, buf);
}
#include <zephyr/syscalls/zvfs_statvfs_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_fstatvfs(int fd, struct zvfs_statvfs *buf)
{
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

	if (entry == NULL) {
		return -1;
	}

	return zvfs_fdtable_call_ioctl(entry->vtable, entry->obj, ZFD_IOCTL_STATVFS, buf);
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_fstatvfs(int fd, struct zvfs_statvfs *buf)
{
	if (zvfs_fd_syscall_verify(fd) == NULL) {
		return -1;
	}

	K_OOPS(K_SYSCALL_MEMORY_WRITE(buf, sizeof(*buf)));

	return z_impl_zvfs_fstatvfs(fd, buf);
}
#include <zephyr/syscalls/zvfs_fstatvfs_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_mkdir(const char *path, uint32_t mode)
{
	ARG_UNUSED(mode);

	return zvfs_err(fs_mkdir(path));
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_mkdir(const char *path, uint32_t mode)
{
	char kpath[CONFIG_ZVFS_PATH_MAX];

	if (zvfs_copy_user_path(kpath, path) < 0) {
		return -1;
	}

	return z_impl_zvfs_mkdir(kpath, mode);
}
#include <zephyr/syscalls/zvfs_mkdir_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_rmdir(const char *path)
{
	int res;
	struct fs_dirent entry;

	res = fs_stat(path, &entry);
	if (res < 0) {
		return zvfs_err(res);
	}

	if (entry.type != FS_DIR_ENTRY_DIR) {
		errno = ENOTDIR;
		return -1;
	}

	return zvfs_err(fs_unlink(path));
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_rmdir(const char *path)
{
	char kpath[CONFIG_ZVFS_PATH_MAX];

	if (zvfs_copy_user_path(kpath, path) < 0) {
		return -1;
	}

	return z_impl_zvfs_rmdir(kpath);
}
#include <zephyr/syscalls/zvfs_rmdir_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_truncate(const char *path, off_t length)
{
	int res;
	int close_res;
	struct zvfs_fs_desc *ptr;

	if (length < 0) {
		errno = EINVAL;
		return -1;
	}

	/* struct fs_file_t is too large for the privileged stack */
	ptr = zvfs_fs_desc_alloc(false);
	if (ptr == NULL) {
		errno = EMFILE;
		return -1;
	}

	fs_file_t_init(&ptr->file);

	res = fs_open(&ptr->file, path, FS_O_WRITE);
	if (res < 0) {
		zvfs_fs_desc_free(ptr);
		return zvfs_err(res);
	}

	res = fs_truncate(&ptr->file, length);
	close_res = fs_close(&ptr->file);
	zvfs_fs_desc_free(ptr);

	if (res == 0) {
		res = close_res;
	}

	return zvfs_err(res);
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_truncate(const char *path, off_t length)
{
	char kpath[CONFIG_ZVFS_PATH_MAX];

	if (zvfs_copy_user_path(kpath, path) < 0) {
		return -1;
	}

	return z_impl_zvfs_truncate(kpath, length);
}
#include <zephyr/syscalls/zvfs_truncate_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_access(const char *path, int amode)
{
	struct zvfs_stat st;

	if ((amode & ~(ZVFS_R_OK | ZVFS_W_OK | ZVFS_X_OK)) != 0 && amode != ZVFS_F_OK) {
		errno = EINVAL;
		return -1;
	}

	if (z_impl_zvfs_stat(path, &st) < 0) {
		return -1;
	}

	if ((amode & ZVFS_R_OK) != 0 && (st.mode & ZVFS_S_IRUSR) == 0) {
		errno = EACCES;
		return -1;
	}

	if ((amode & ZVFS_W_OK) != 0 && (st.mode & ZVFS_S_IWUSR) == 0) {
		errno = EROFS;
		return -1;
	}

	if ((amode & ZVFS_X_OK) != 0 && (st.mode & ZVFS_S_IXUSR) == 0) {
		errno = EACCES;
		return -1;
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_access(const char *path, int amode)
{
	char kpath[CONFIG_ZVFS_PATH_MAX];

	if (zvfs_copy_user_path(kpath, path) < 0) {
		return -1;
	}

	return z_impl_zvfs_access(kpath, amode);
}
#include <zephyr/syscalls/zvfs_access_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_utime(const char *path, const struct timespec *times)
{
	return zvfs_err(fs_utime(path, times));
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_utime(const char *path, const struct timespec *times)
{
	char kpath[CONFIG_ZVFS_PATH_MAX];
	struct timespec ktimes[2];
	const struct timespec *tp = NULL;

	if (times != NULL) {
		K_OOPS(k_usermode_from_copy(ktimes, times, sizeof(ktimes)));
		tp = ktimes;
	}

	if (zvfs_copy_user_path(kpath, path) < 0) {
		return -1;
	}

	return z_impl_zvfs_utime(kpath, tp);
}
#include <zephyr/syscalls/zvfs_utime_mrsh.c>
#endif /* CONFIG_USERSPACE */
