/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 1982, 1986, 1993
 * The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef ZEPHYR_POSIX_SYS_STAT_H_
#define ZEPHYR_POSIX_SYS_STAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/_timespec.h>

#ifndef _DEV_T_DECLARED
typedef int dev_t;
#define _DEV_T_DECLARED
#endif

#ifndef _INO_T_DECLARED
typedef int ino_t;
#define _INO_T_DECLARED
#endif

#ifndef _NLINK_T_DECLARED
typedef unsigned short nlink_t;
#define _NLINK_T_DECLARED
#endif

#ifndef _UID_T_DECLARED
typedef unsigned short uid_t;
#define _UID_T_DECLARED
#endif

#ifndef _GID_T_DECLARED
typedef unsigned short gid_t;
#define _GID_T_DECLARED
#endif

#ifndef _BLKSIZE_T_DECLARED
typedef unsigned long blksize_t;
#define _BLKSIZE_T_DECLARED
#endif

#ifndef _BLKCNT_T_DECLARED
typedef unsigned long blkcnt_t;
#define _BLKCNT_T_DECLARED
#endif

/* dj's stat defines _STAT_H_ */
#ifndef _STAT_H_

/*
 * It is intended that the layout of this structure not change when the
 * sizes of any of the basic types change (short, int, long) [via a compile
 * time option].
 */

#ifdef __CYGWIN__
#include <cygwin/stat.h>
#ifdef _LIBC
#define stat64 stat
#endif
#else
struct stat {
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
#if defined(__linux) && defined(__x86_64__)
	int __pad0;
#endif
	dev_t st_rdev;
#if defined(__linux) && !defined(__x86_64__)
	unsigned short int __pad2;
#endif
	off_t st_size;
#if defined(__linux)
	blksize_t st_blksize;
	blkcnt_t st_blocks;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
#define st_atime st_atim.tv_sec /* Backward compatibility */
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
#if defined(__linux) && defined(__x86_64__)
	uint64_t __glibc_reserved[3];
#endif
#else
#if defined(__rtems__)
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
#else
	/* SysV/sco doesn't have the rest... But Solaris, eabi does.  */
#if defined(__svr4__) && !defined(__PPC__) && !defined(__sun__)
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
#else
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
#if !defined(__rtems__)
	long st_spare4[2];
#endif
#endif
#endif
#endif
};

#if !(defined(__svr4__) && !defined(__PPC__) && !defined(__sun__))
#define st_atime st_atim.tv_sec
#define st_ctime st_ctim.tv_sec
#define st_mtime st_mtim.tv_sec
#endif

#endif

#define _IFMT	0170000 /* type of file */
#define _IFDIR	0040000 /* directory */
#define _IFCHR	0020000 /* character special */
#define _IFBLK	0060000 /* block special */
#define _IFREG	0100000 /* regular */
#define _IFLNK	0120000 /* symbolic link */
#define _IFSOCK 0140000 /* socket */
#define _IFIFO	0010000 /* fifo */

#define S_BLKSIZE 1024 /* size of a block */

#define S_ISUID 0004000 /* set user id on execution */
#define S_ISGID 0002000 /* set group id on execution */
#define S_ISVTX 0001000 /* save swapped text even after use */
#if __BSD_VISIBLE
#define S_IREAD	 0000400 /* read permission, owner */
#define S_IWRITE 0000200 /* write permission, owner */
#define S_IEXEC	 0000100 /* execute/search permission, owner */
#define S_ENFMT	 0002000 /* enforcement-mode locking */
#endif			 /* !_BSD_VISIBLE */

#define S_IFMT	 _IFMT
#define S_IFDIR	 _IFDIR
#define S_IFCHR	 _IFCHR
#define S_IFBLK	 _IFBLK
#define S_IFREG	 _IFREG
#define S_IFLNK	 _IFLNK
#define S_IFSOCK _IFSOCK
#define S_IFIFO	 _IFIFO

#ifdef _WIN32
/*
 * The Windows header files define _S_ forms of these, so we do too
 * for easier portability.
 */
#define _S_IFMT	  _IFMT
#define _S_IFDIR  _IFDIR
#define _S_IFCHR  _IFCHR
#define _S_IFIFO  _IFIFO
#define _S_IFREG  _IFREG
#define _S_IREAD  0000400
#define _S_IWRITE 0000200
#define _S_IEXEC  0000100
#endif

#define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#define S_IRUSR 0000400 /* read permission, owner */
#define S_IWUSR 0000200 /* write permission, owner */
#define S_IXUSR 0000100 /* execute/search permission, owner */
#define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)
#define S_IRGRP 0000040 /* read permission, group */
#define S_IWGRP 0000020 /* write permission, grougroup */
#define S_IXGRP 0000010 /* execute/search permission, group */
#define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)
#define S_IROTH 0000004 /* read permission, other */
#define S_IWOTH 0000002 /* write permission, other */
#define S_IXOTH 0000001 /* execute/search permission, other */

#if __BSD_VISIBLE
#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO)				/* 0777 */
#define ALLPERMS    (S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO) /* 07777 */
#define DEFFILEMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) /* 0666 */
#endif

#define S_ISBLK(m)  (((m)&_IFMT) == _IFBLK)
#define S_ISCHR(m)  (((m)&_IFMT) == _IFCHR)
#define S_ISDIR(m)  (((m)&_IFMT) == _IFDIR)
#define S_ISFIFO(m) (((m)&_IFMT) == _IFIFO)
#define S_ISREG(m)  (((m)&_IFMT) == _IFREG)
#define S_ISLNK(m)  (((m)&_IFMT) == _IFLNK)
#define S_ISSOCK(m) (((m)&_IFMT) == _IFSOCK)

#if defined(__CYGWIN__) || defined(__rtems__)
/* Special tv_nsec values for futimens(2) and utimensat(2). */
#define UTIME_NOW  -2L
#define UTIME_OMIT -1L
#endif

int chmod(const char *__path, mode_t __mode);
int fchmod(int __fd, mode_t __mode);
int fstat(int __fd, struct stat *__sbuf);
int mkdir(const char *_path, mode_t __mode);
int mkfifo(const char *__path, mode_t __mode);
int stat(const char *__restrict __path, struct stat *__restrict __sbuf);
mode_t umask(mode_t __mask);

#if defined(__SPU__) || defined(__rtems__) || defined(__CYGWIN__) && !defined(__INSIDE_CYGWIN__)
int lstat(const char *__restrict __path, struct stat *__restrict __buf);
int mknod(const char *__path, mode_t __mode, dev_t __dev);
#endif

#if __ATFILE_VISIBLE && !defined(__INSIDE_CYGWIN__)
int fchmodat(int __fd, const char *__path, mode_t __mode, int __flag);
int fstatat(int __fd, const char *__restrict __path, struct stat *__restrict __buf, int __flag);
int mkdirat(int __fd, const char *__path, mode_t __mode);
int mkfifoat(int __fd, const char *__path, mode_t __mode);
int mknodat(int __fd, const char *__path, mode_t __mode, dev_t __dev);
int utimensat(int __fd, const char *__path, const struct timespec __times[2], int __flag);
#endif
#if __POSIX_VISIBLE >= 200809 && !defined(__INSIDE_CYGWIN__)
int futimens(int __fd, const struct timespec __times[2]);
#endif

/*
 * Provide prototypes for most of the _<systemcall> names that are
 * provided in newlib for some compilers.
 */
#ifdef _LIBC
int _fstat(int __fd, struct stat *__sbuf);
int _stat(const char *__restrict __path, struct stat *__restrict __sbuf);
int _mkdir(const char *_path, mode_t __mode);
#ifdef __LARGE64_FILES
struct stat64;
int _stat64(const char *__restrict __path, struct stat64 *__restrict __sbuf);
int _fstat64(int __fd, struct stat64 *__sbuf);
#endif
#endif

#endif /* !_STAT_H_ */
#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_POSIX_SYS_STAT_H_ */
