/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_POSIX_SYS_STAT_H_
#define ZEPHYR_INCLUDE_ZEPHYR_POSIX_SYS_STAT_H_

#include <time.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/fdtable.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

/* slightly out of order w.r.t. the specification */
#if !defined(_BLKCNT_T_DECLARED) && !defined(__blkcnt_t_defined)
typedef long blkcnt_t;
#define _BLKCNT_T_DECLARED
#define __blkcnt_t_defined
#endif

#if !defined(_BLKSIZE_T_DECLARED) && !defined(__blksize_t_defined)
typedef unsigned long blksize_t;
#define _BLKSIZE_T_DECLARED
#define __blksize_t_defined
#endif

#if !defined(_DEV_T_DECLARED) && !defined(__dev_t_defined)
typedef int dev_t;
#define _DEV_T_DECLARED
#define __dev_t_defined
#endif

#if !defined(_GID_T_DECLARED) && !defined(__gid_t_defined)
typedef unsigned short gid_t;
#define _GID_T_DECLARED
#define __gid_t_defined
#endif

#if !defined(_INO_T_DECLARED) && !defined(__ino_t_defined)
typedef long ino_t;
#define _INO_T_DECLARED
#define __ino_t_defined
#endif

#if !defined(_MODE_T_DECLARED) && !defined(__mode_t_defined)
typedef int mode_t;
#define _MODE_T_DECLARED
#define __mode_t_defined
#endif

#if !defined(_NLINK_T_DECLARED) && !defined(__nlink_t_defined)
typedef unsigned short nlink_t;
#define _NLINK_T_DECLARED
#define __nlink_t_defined
#endif

#if !defined(_OFF_T_DECLARED) && !defined(__off_t_defined)
typedef long off_t;
#define _OFF_T_DECLARED
#define __off_t_defined
#endif

/* time_t must be defined by the libc time.h */
#include <time.h>

#if __STDC_VERSION__ >= 201112L
/* struct timespec must be defined in the libc time.h */
#else
/*
 * there is a workaround needed for picolibc because it doesn't have guards around the definition
 * of struct timespec
 */
#if !defined(_TIMESPEC_DECLARED) && !defined(__timespec_defined) && !defined(CONFIG_PICOLIBC)
struct timespec {
	time_t tv_sec;
	long tv_nsec;
};
#define _TIMESPEC_DECLARED
#define __timespec_defined
#endif
#endif

#if !defined(_UID_T_DECLARED) && !defined(__uid_t_defined)
typedef unsigned short uid_t;
#define _UID_T_DECLARED
#define __uid_t_defined
#endif

#if !(defined(_STAT_DECLARED) || defined(__stat_defined)) || defined(__DOXYGEN__)
struct stat {
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
	dev_t st_rdev;
#endif
	off_t st_size;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
	blkcnt_t st_blksize;
	blkcnt_t st_blocks;
#endif
};
#define _STAT_DECLARED
#define __stat_defined
#endif

#define S_IFMT 00170000
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
#define S_IFBLK  ZVFS_MODE_IFBLK
#define S_IFCHR  ZVFS_MODE_IFCHR
#define S_IFIFO  ZVFS_MODE_IFIFO
#define S_IFREG  ZVFS_MODE_IFREG
#define S_IFDIR  ZVFS_MODE_IFDIR
#define S_IFLNK  ZVFS_MODE_IFLNK
#define S_IFSOCK ZVFS_MODE_IFSOCK
#define S_IFSHM  ZVFS_MODE_IFSHM
#endif

#define S_IRWXU 0000700
#define S_IRUSR 0000400
#define S_IWUSR 0000200
#define S_IXUSR 0000100
#define S_IRWXG 0000070
#define S_IRGRP 0000040
#define S_IWGRP 0000020
#define S_IXGRP 0000010
#define S_IRWXO 0000007
#define S_IROTH 0000004
#define S_IWOTH 0000002
#define S_IXOTH 0000001
#define S_ISUID 0004000
#define S_ISGID 0002000
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
#define S_ISVTX 0001000
#endif

#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
#define S_TYPEISMQ(buf)  (0)
#define S_TYPEISSEM(buf) (0)
#define S_TYPEISSHM(st)  (((st)->st_mode & S_IFMT) == S_IFSHM)
#endif

#if defined(_POSIX_TYPED_MEMORY_OBJECTS) || defined(__DOXYGEN__)
#define S_TYPEISTMO(buf) (0)
#endif

#define UTIME_NOW  -1
#define UTIME_OMIT -2

int chmod(const char *path, mode_t mode);
int fchmod(int fildes, mode_t mode);
int fchmodat(int fd, const char *path, mode_t mode, int flag);
int fstat(int fildes, struct stat *buf);
int fstatat(int fd, const char *ZRESTRICT path, struct stat *ZRESTRICT buf, int flag);
int futimens(int fildes, const struct timespec times[2]);
int lstat(const char *ZRESTRICT path, struct stat *ZRESTRICT buf);
int mkdir(const char *path, mode_t mode);
int mkdirat(int fd, const char *path, mode_t mode);
int mkfifo(const char *path, mode_t mode);
int mkfifoat(int fd, const char *path, mode_t mode);
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
int mknod(const char *path, mode_t mode, dev_t dev);
int mknodat(int fd, const char *path, mode_t mode, dev_t dev);
#endif
TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_SHADOW);
int stat(const char *ZRESTRICT path, struct stat *ZRESTRICT buf);
TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_SHADOW);
mode_t umask(mode_t cmask);
int utimensat(int fd, const char *path, const struct timespec times[2], int flag);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_POSIX_SYS_STAT_H_ */
