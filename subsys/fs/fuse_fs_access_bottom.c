/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_FUSE_LIBRARY_V3)
#define FUSE_USE_VERSION 30
#else
#define FUSE_USE_VERSION 26
#endif

#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <fuse.h>
#include <libgen.h>
#include <linux/limits.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <nsi_tracing.h>
#include <nsi_utils.h>
#include <nsi_errno.h>
#include "fuse_fs_access_bottom.h"


#define S_IRWX_DIR (0775)
#define S_IRW_FILE (0664)

#define DIR_END '\0'

static pthread_t fuse_thread;
static struct ffa_op_callbacks *op_callbacks;
#if defined(CONFIG_FUSE_LIBRARY_V3)
static sem_t fuse_started; /* semaphore to signal fuse_main has started */
#endif

/* Pending operation the bottom/fuse thread is queuing into the Zephyr thread */
struct {
	int op;        /* One of OP_**/
	void *args;    /* Pointer to arguments structure, one of op_args_* or a simple argument */
	int ret;       /* Return from the operation */
	bool pending;  /* Is there a pending operation */
	sem_t op_done; /* semaphore to signal the job is done */
} op_queue;

#define OP_STAT              offsetof(struct ffa_op_callbacks, stat)
#define OP_READMOUNT         offsetof(struct ffa_op_callbacks, readmount)
#define OP_READDIR_START     offsetof(struct ffa_op_callbacks, readdir_start)
#define OP_READDIR_READ_NEXT offsetof(struct ffa_op_callbacks, readdir_read_next)
#define OP_READDIR_END       offsetof(struct ffa_op_callbacks, readdir_end)
#define OP_MKDIR             offsetof(struct ffa_op_callbacks, mkdir)
#define OP_CREATE            offsetof(struct ffa_op_callbacks, create)
#define OP_RELEASE           offsetof(struct ffa_op_callbacks, release)
#define OP_READ              offsetof(struct ffa_op_callbacks, read)
#define OP_WRITE             offsetof(struct ffa_op_callbacks, write)
#define OP_FTRUNCATE         offsetof(struct ffa_op_callbacks, ftruncate)
#define OP_TRUNCATE          offsetof(struct ffa_op_callbacks, truncate)
#define OP_UNLINK            offsetof(struct ffa_op_callbacks, unlink)
#define OP_RMDIR             offsetof(struct ffa_op_callbacks, rmdir)

struct op_args_truncate {
	const char *path;
	off_t size;
};
struct op_args_ftruncate {
	uint64_t fh;
	off_t size;
};
struct op_args_readwrite {
	uint64_t fh;
	char *buf;
	off_t size;
	off_t off;
};
struct op_args_create {
	const char *path;
	uint64_t *fh_p;
};
struct op_args_readmount {
	int *mnt_nbr_p;
	const char **mnt_name_p;
};
struct op_args_stat {
	const char *path;
	struct ffa_dirent *entry_p;
};

static inline int queue_op(int op, void *args)
{
	op_queue.op = op;
	op_queue.args = args;
	op_queue.pending = true;

	sem_wait(&op_queue.op_done);

	return op_queue.ret;
}

bool ffa_is_op_pended(void)
{
	return op_queue.pending;
}

void ffa_run_pending_op(void)
{
	switch ((intptr_t)op_queue.op) {
	case OP_RMDIR:
		op_queue.ret = op_callbacks->rmdir((const char *)op_queue.args);
		break;
	case OP_UNLINK:
		op_queue.ret = op_callbacks->unlink((const char *)op_queue.args);
		break;
	case OP_TRUNCATE: {
		struct op_args_truncate *args = op_queue.args;

		op_queue.ret = op_callbacks->truncate(args->path, args->size);
		break;
	}
	case OP_FTRUNCATE: {
		struct op_args_ftruncate *args = op_queue.args;

		op_queue.ret = op_callbacks->ftruncate(args->fh, args->size);
		break;
	}
	case OP_WRITE: {
		struct op_args_readwrite *args = op_queue.args;

		op_queue.ret = op_callbacks->write(args->fh, args->buf, args->size, args->off);
		break;
	}
	case OP_READ: {
		struct op_args_readwrite *args = op_queue.args;

		op_queue.ret = op_callbacks->read(args->fh, args->buf, args->size, args->off);
		break;
	}
	case OP_RELEASE:
		op_queue.ret = op_callbacks->release(*(uint64_t *)op_queue.args);
		break;
	case OP_CREATE: {
		struct op_args_create *args = op_queue.args;

		op_queue.ret = op_callbacks->create(args->path, args->fh_p);
		break;
	}
	case OP_MKDIR:
		op_queue.ret = op_callbacks->mkdir((const char *)op_queue.args);
		break;
	case OP_READDIR_END:
		op_callbacks->readdir_end();
		break;
	case OP_READDIR_READ_NEXT:
		op_queue.ret = op_callbacks->readdir_read_next((struct ffa_dirent *)op_queue.args);
		break;
	case OP_READDIR_START:
		op_queue.ret = op_callbacks->readdir_start((const char *)op_queue.args);
		break;
	case OP_READMOUNT: {
		struct op_args_readmount *args = op_queue.args;

		op_queue.ret = op_callbacks->readmount(args->mnt_nbr_p, args->mnt_name_p);
		break;
	}
	case OP_STAT: {
		struct op_args_stat *args = op_queue.args;

		op_queue.ret = op_callbacks->stat(args->path, args->entry_p);
		break;
	}
	default:
		nsi_print_error_and_exit("Programming error, unknown queued operation\n");
		break;
	}
	op_queue.pending = false;
	sem_post(&op_queue.op_done);
}

static bool is_mount_point(const char *path)
{
	char dir_path[PATH_MAX];
	size_t len;

	len = strlen(path);
	if (len >= sizeof(dir_path)) {
		return false;
	}

	memcpy(dir_path, path, len);
	dir_path[len] = '\0';
	return strcmp(dirname(dir_path), "/") == 0;
}

#if defined(CONFIG_FUSE_LIBRARY_V3)
static int fuse_fs_access_getattr(const char *path, struct stat *st, struct fuse_file_info *fi)
{
	NSI_ARG_UNUSED(fi);
#else
static int fuse_fs_access_getattr(const char *path, struct stat *st)
{
#endif
	struct ffa_dirent entry;
	int err;

	st->st_dev = 0;
	st->st_ino = 0;
	st->st_nlink = 0;
	st->st_uid = getuid();
	st->st_gid = getgid();
	st->st_rdev = 0;
	st->st_blksize = 0;
	st->st_blocks = 0;
	st->st_atime = 0;
	st->st_mtime = 0;
	st->st_ctime = 0;

	if ((strcmp(path, "/") == 0) || is_mount_point(path)) {
		if (strstr(path, "/.") != NULL) {
			return -ENOENT;
		}
		st->st_mode = S_IFDIR | S_IRWX_DIR;
		st->st_size = 0;
		return 0;
	}

	struct op_args_stat args;

	args.path = path;
	args.entry_p = &entry;

	err = queue_op(OP_STAT, (void *)&args);

	if (err != 0) {
		return -nsi_errno_from_mid(err);
	}

	if (entry.is_directory) {
		st->st_mode = S_IFDIR | S_IRWX_DIR;
		st->st_size = 0;
	} else {
		st->st_mode = S_IFREG | S_IRW_FILE;
		st->st_size = entry.size;
	}

	return 0;
}

static int fuse_fs_access_readmount(void *buf, fuse_fill_dir_t filler)
{
	int mnt_nbr = 0;
	const char *mnt_name;
	struct stat st;
	int err;

	st.st_dev = 0;
	st.st_ino = 0;
	st.st_nlink = 0;
	st.st_uid = getuid();
	st.st_gid = getgid();
	st.st_rdev = 0;
	st.st_atime = 0;
	st.st_mtime = 0;
	st.st_ctime = 0;
	st.st_mode = S_IFDIR | S_IRWX_DIR;
	st.st_size = 0;
	st.st_blksize = 0;
	st.st_blocks = 0;

#if defined(CONFIG_FUSE_LIBRARY_V3)
	filler(buf, ".", &st, 0, 0);
	filler(buf, "..", NULL, 0, 0);
#else
	filler(buf, ".", &st, 0);
	filler(buf, "..", NULL, 0);
#endif

	do {
		struct op_args_readmount args;

		args.mnt_nbr_p = &mnt_nbr;
		args.mnt_name_p = &mnt_name;

		err = queue_op(OP_READMOUNT, (void *)&args);
		err = -nsi_errno_from_mid(err);

		if (err < 0) {
			break;
		}

#if defined(CONFIG_FUSE_LIBRARY_V3)
		filler(buf, &mnt_name[1], &st, 0, 0);
#else
		filler(buf, &mnt_name[1], &st, 0);
#endif

	} while (true);

	if (err == -ENOENT) {
		err = 0;
	}

	return err;
}

#if defined(CONFIG_FUSE_LIBRARY_V3)
static int fuse_fs_access_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off,
				  struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
	NSI_ARG_UNUSED(flags);
#else
static int fuse_fs_access_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off,
				  struct fuse_file_info *fi)
{
#endif
	NSI_ARG_UNUSED(off);
	NSI_ARG_UNUSED(fi);

	struct ffa_dirent entry;
	int err;
	struct stat st;

	if (strcmp(path, "/") == 0) {
		err = fuse_fs_access_readmount(buf, filler);
		return -nsi_errno_from_mid(err);
	}

	if (is_mount_point(path)) {
		/* File system API expects trailing slash for a mount point
		 * directory but FUSE strips the trailing slashes from
		 * directory names so add it back.
		 */
		char mount_path[PATH_MAX] = {0};
		size_t len = strlen(path);

		if (len >= (PATH_MAX - 2)) {
			return -ENOMEM;
		}

		memcpy(mount_path, path, len);
		mount_path[len] = '/';
		err = queue_op(OP_READDIR_START, (void *)mount_path);
	} else {
		err = queue_op(OP_READDIR_START, (void *)path);
	}

	if (err) {
		return -ENOEXEC;
	}

	st.st_dev = 0;
	st.st_ino = 0;
	st.st_nlink = 0;
	st.st_uid = getuid();
	st.st_gid = getgid();
	st.st_rdev = 0;
	st.st_atime = 0;
	st.st_mtime = 0;
	st.st_ctime = 0;
	st.st_mode = S_IFDIR | S_IRWX_DIR;
	st.st_size = 0;
	st.st_blksize = 0;
	st.st_blocks = 0;

#if defined(CONFIG_FUSE_LIBRARY_V3)
	filler(buf, ".", &st, 0, 0);
	filler(buf, "..", &st, 0, 0);
#else
	filler(buf, ".", &st, 0);
	filler(buf, "..", &st, 0);
#endif

	do {
		err = queue_op(OP_READDIR_READ_NEXT, (void *)&entry);
		if (err) {
			break;
		}

		if (entry.name[0] == DIR_END) {
			break;
		}

		if (entry.is_directory) {
			st.st_mode = S_IFDIR | S_IRWX_DIR;
			st.st_size = 0;
		} else {
			st.st_mode = S_IFREG | S_IRW_FILE;
			st.st_size = entry.size;
		}

#if defined(CONFIG_FUSE_LIBRARY_V3)
		bool full = filler(buf, entry.name, &st, 0, 0);
#else
		bool full = filler(buf, entry.name, &st, 0);
#endif

		if (full) {
			break;
		}
	} while (1);

	queue_op(OP_READDIR_END, NULL);

	return -nsi_errno_from_mid(err);
}

static int fuse_fs_access_mkdir(const char *path, mode_t mode)
{
	NSI_ARG_UNUSED(mode);

	int err = queue_op(OP_MKDIR, (void *)path);

	return -nsi_errno_from_mid(err);
}

static int fuse_fs_access_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int err;
	struct op_args_create args;

	NSI_ARG_UNUSED(mode);

	if (is_mount_point(path)) {
		return -ENOENT;
	}

	args.path = path;
	args.fh_p = &fi->fh;

	err = queue_op(OP_CREATE, (void *)&args);

	return -nsi_errno_from_mid(err);
}

static int fuse_fs_access_open(const char *path, struct fuse_file_info *fi)
{
	int err = fuse_fs_access_create(path, 0, fi);

	return -nsi_errno_from_mid(err);
}

static int fuse_fs_access_release(const char *path, struct fuse_file_info *fi)
{
	NSI_ARG_UNUSED(path);

	if (fi->fh == INVALID_FILE_HANDLE) {
		return -EINVAL;
	}

	(void)queue_op(OP_RELEASE, (void *)&fi->fh);

	return 0;
}

static int fuse_fs_access_read(const char *path, char *buf, size_t size, off_t off,
			       struct fuse_file_info *fi)
{
	int err;
	struct op_args_readwrite args;

	NSI_ARG_UNUSED(path);

	if (fi->fh == INVALID_FILE_HANDLE) {
		return -EINVAL;
	}

	args.fh = fi->fh;
	args.buf = buf;
	args.size = size;
	args.off = off;

	err = queue_op(OP_READ, (void *)&args);

	return -nsi_errno_from_mid(err);
}

static int fuse_fs_access_write(const char *path, const char *buf, size_t size, off_t off,
				struct fuse_file_info *fi)
{
	int err;
	struct op_args_readwrite args;

	NSI_ARG_UNUSED(path);

	if (fi->fh == INVALID_FILE_HANDLE) {
		return -EINVAL;
	}

	args.fh = fi->fh;
	args.buf = (char *)buf;
	args.size = size;
	args.off = off;

	err = queue_op(OP_WRITE, (void *)&args);

	return -nsi_errno_from_mid(err);
}

#if !defined(CONFIG_FUSE_LIBRARY_V3)
static int fuse_fs_access_ftruncate(const char *path, off_t size, struct fuse_file_info *fi)
{
	struct op_args_ftruncate args;
	int err;

	NSI_ARG_UNUSED(path);

	if (fi->fh == INVALID_FILE_HANDLE) {
		return -EINVAL;
	}

	args.fh = fi->fh;
	args.size = size;

	err = queue_op(OP_FTRUNCATE, (void *)&args);

	return -nsi_errno_from_mid(err);
}
#endif

#if defined(CONFIG_FUSE_LIBRARY_V3)
static int fuse_fs_access_truncate(const char *path, off_t size, struct fuse_file_info *fi)
{
	NSI_ARG_UNUSED(fi);
#else
static int fuse_fs_access_truncate(const char *path, off_t size)
{
#endif
	struct op_args_truncate args;
	int err;

	args.path = path;
	args.size = size;

	err = queue_op(OP_TRUNCATE, (void *)&args);

	return -nsi_errno_from_mid(err);
}

static int fuse_fs_access_rmdir(const char *path)
{
	int err = queue_op(OP_RMDIR, (void *)path);

	return -nsi_errno_from_mid(err);
}

static int fuse_fs_access_unlink(const char *path)
{
	int err = queue_op(OP_UNLINK, (void *)path);

	return -nsi_errno_from_mid(err);
}

static int fuse_fs_access_statfs(const char *path, struct statvfs *buf)
{
	NSI_ARG_UNUSED(path);
	NSI_ARG_UNUSED(buf);
	return 0;
}

#if defined(CONFIG_FUSE_LIBRARY_V3)
static int fuse_fs_access_utimens(const char *path, const struct timespec tv[2],
				  struct fuse_file_info *fi)
{
	NSI_ARG_UNUSED(fi);
#else
static int fuse_fs_access_utimens(const char *path, const struct timespec tv[2])
{
#endif
	/* dummy */
	NSI_ARG_UNUSED(path);
	NSI_ARG_UNUSED(tv);
	return 0;
}

#if defined(CONFIG_FUSE_LIBRARY_V3)
static void *fuse_fs_access_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	NSI_ARG_UNUSED(conn);
	NSI_ARG_UNUSED(cfg);

	sem_post(&fuse_started);
	return NULL;
}
#endif

static struct fuse_operations fuse_fs_access_oper = {
	.getattr = fuse_fs_access_getattr,
	.readlink = NULL,
	.mknod = NULL,
	.mkdir = fuse_fs_access_mkdir,
	.unlink = fuse_fs_access_unlink,
	.rmdir = fuse_fs_access_rmdir,
	.symlink = NULL,
	.rename = NULL,
	.link = NULL,
	.chmod = NULL,
	.chown = NULL,
	.truncate = fuse_fs_access_truncate,
	.open = fuse_fs_access_open,
	.read = fuse_fs_access_read,
	.write = fuse_fs_access_write,
	.statfs = fuse_fs_access_statfs,
	.flush = NULL,
	.release = fuse_fs_access_release,
	.fsync = NULL,
	.setxattr = NULL,
	.getxattr = NULL,
	.listxattr = NULL,
	.removexattr = NULL,
	.opendir = NULL,
	.readdir = fuse_fs_access_readdir,
	.releasedir = NULL,
	.fsyncdir = NULL,
#if defined(CONFIG_FUSE_LIBRARY_V3)
	.init = fuse_fs_access_init,
#endif
	.destroy = NULL,
	.access = NULL,
	.create = fuse_fs_access_create,
#if !defined(CONFIG_FUSE_LIBRARY_V3)
	.ftruncate = fuse_fs_access_ftruncate,
#endif
	.lock = NULL,
	.utimens = fuse_fs_access_utimens,
	.bmap = NULL,
#if !defined(CONFIG_FUSE_LIBRARY_V3)
	.flag_nullpath_ok = 0,
	.flag_nopath = 0,
	.flag_utime_omit_ok = 0,
	.flag_reserved = 0,
#endif
	.ioctl = NULL,
	.poll = NULL,
	.write_buf = NULL,
	.read_buf = NULL,
	.flock = NULL,
	.fallocate = NULL,
};

static void *ffsa_main(void *fuse_mountpoint)
{
	char *argv[] = {
		"",
		"-f",
		"-s",
		(char *)fuse_mountpoint
	};
	int argc = NSI_ARRAY_SIZE(argv);

	nsi_print_trace("FUSE mounting flash in host %s/\n", (char *)fuse_mountpoint);

	fuse_main(argc, argv, &fuse_fs_access_oper, NULL);

	pthread_exit(0);
	return NULL;
}

void ffsa_init_bottom(const char *fuse_mountpoint, struct ffa_op_callbacks *op_cbs)
{
	struct stat st;
	int err;

	op_callbacks = op_cbs;

	if (stat(fuse_mountpoint, &st) < 0) {
		if (mkdir(fuse_mountpoint, 0700) < 0) {
			nsi_print_error_and_exit("Failed to create directory for flash mount point "
						 "(%s): %s\n",
						 fuse_mountpoint, strerror(errno));
		}
	} else if (!S_ISDIR(st.st_mode)) {
		nsi_print_error_and_exit("%s is not a directory\n", fuse_mountpoint);
	}

#if defined(CONFIG_FUSE_LIBRARY_V3)
	/* Fuse3's fuse_daemonize() changes the cwd to "/" which is a quite undesirable
	 * side-effect, so we will revert back to where we were once it has initialized
	 */
	char *cwd = getcwd(NULL, 0);

	err = sem_init(&fuse_started, 0, 0);
	if (err) {
		nsi_print_error_and_exit("Failed to initialize semaphore\n");
	}
#endif

	err = sem_init(&op_queue.op_done, 0, 0);
	if (err) {
		nsi_print_error_and_exit("Failed to initialize semaphore\n");
	}

	err = pthread_create(&fuse_thread, NULL, ffsa_main, (void *)fuse_mountpoint);
	if (err < 0) {
		nsi_print_error_and_exit("Failed to create thread for ffsa_main()\n");
	}

#if defined(CONFIG_FUSE_LIBRARY_V3)
	if (cwd != NULL) {
		sem_wait(&fuse_started);
		if (chdir(cwd)) {
			nsi_print_error_and_exit("Failed to change directory back to %s "
						 "after starting FUSE\n");
		}
		free(cwd);
	}
#endif
}

void ffsa_cleanup_bottom(const char *fuse_mountpoint)
{
	char *full_cmd;
	static const char cmd[] = "fusermount -uz ";

	full_cmd = malloc(strlen(cmd) + strlen(fuse_mountpoint) + 1);

	sprintf(full_cmd, "%s%s", cmd, fuse_mountpoint);
	if (system(full_cmd) < -1) {
		nsi_print_trace("Failed to unmount fuse mount point\n");
	}
	free(full_cmd);

	pthread_join(fuse_thread, NULL);
}
