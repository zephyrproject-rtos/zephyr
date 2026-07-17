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

/**
 * @file
 * @brief Data returned by the stat() function.
 * @ingroup posix
 *
 * Defines the stat structure describing file status, the S_I* file type and
 * permission bits, and functions such as stat(), fstat(), mkdir(), and umask().
 *
 * @posix_header{sys_stat.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_STAT_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_STAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <sys/cdefs.h>

#include <zephyr/posix/posix_types.h>

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
/**
 * @brief 64-bit file status information.
 */
#define stat64 stat
#endif
#else
/**
 * @brief File status information.
 */
struct stat {
	/**
	 * @brief Device ID of the device containing the file.
	 */
	dev_t st_dev;
	/**
	 * @brief File serial number.
	 */
	ino_t st_ino;
	/**
	 * @brief Mode of file (see below).
	 */
	mode_t st_mode;
	/**
	 * @brief Number of hard links to the file.
	 */
	nlink_t st_nlink;
	/**
	 * @brief User ID of the file owner.
	 */
	uid_t st_uid;
	/**
	 * @brief Group ID of the file's group.
	 */
	gid_t st_gid;
#if defined(__linux) && defined(__x86_64__)
	/**
	 * @brief Padding for alignment.
	 */
	int __pad0;
#endif
	/**
	 * @brief Device ID (for character or block special files).
	 */
	dev_t st_rdev;
#if defined(__linux) && !defined(__x86_64__)
	/**
	 * @brief Padding for alignment.
	 */
	unsigned short int __pad2;
#endif
	/**
	 * @brief For regular files, the file size in bytes.
	 */
	off_t st_size;
#if defined(__linux)
	/**
	 * @brief A file system-specific preferred I/O block size.
	 */
	blksize_t st_blksize;
	/**
	 * @brief Number of blocks allocated for this object.
	 */
	blkcnt_t st_blocks;
	/**
	 * @brief Last data access timestamp.
	 */
	struct timespec st_atim;
	/**
	 * @brief Last data modification timestamp.
	 */
	struct timespec st_mtim;
	/**
	 * @brief Last file status change timestamp.
	 */
	struct timespec st_ctim;
	/**
	 * @brief Last data access timestamp in seconds.
	 */
#define st_atime st_atim.tv_sec /* Backward compatibility */
	/**
	 * @brief Last data modification timestamp in seconds.
	 */
#define st_mtime st_mtim.tv_sec
	/**
	 * @brief Last file status change timestamp in seconds.
	 */
#define st_ctime st_ctim.tv_sec
#if defined(__linux) && defined(__x86_64__)
	/**
	 * @brief Reserved for future use.
	 */
	uint64_t __glibc_reserved[3];
#endif
#else
#if defined(__rtems__)
	/**
	 * @brief Last data access timestamp.
	 */
	struct timespec st_atim;
	/**
	 * @brief Last data modification timestamp.
	 */
	struct timespec st_mtim;
	/**
	 * @brief Last file status change timestamp.
	 */
	struct timespec st_ctim;
	/**
	 * @brief A file system-specific preferred I/O block size.
	 */
	blksize_t st_blksize;
	/**
	 * @brief Number of blocks allocated for this object.
	 */
	blkcnt_t st_blocks;
#else
	/* SysV/sco doesn't have the rest... But Solaris, eabi does.  */
#if defined(__svr4__) && !defined(__PPC__) && !defined(__sun__)
	/**
	 * @brief Last data access timestamp.
	 */
	time_t st_atime;
	/**
	 * @brief Last data modification timestamp.
	 */
	time_t st_mtime;
	/**
	 * @brief Last file status change timestamp.
	 */
	time_t st_ctime;
#else
	/**
	 * @brief Last data access timestamp.
	 */
	struct timespec st_atim;
	/**
	 * @brief Last data modification timestamp.
	 */
	struct timespec st_mtim;
	/**
	 * @brief Last file status change timestamp.
	 */
	struct timespec st_ctim;
	/**
	 * @brief A file system-specific preferred I/O block size.
	 */
	blksize_t st_blksize;
	/**
	 * @brief Number of blocks allocated for this object.
	 */
	blkcnt_t st_blocks;
#if !defined(__rtems__)
	/**
	 * @brief Reserved spare fields.
	 */
	long st_spare4[2];
#endif
#endif
#endif
#endif
};

#if !(defined(__svr4__) && !defined(__PPC__) && !defined(__sun__))
/**
 * @brief Last data access timestamp in seconds.
 */
#define st_atime st_atim.tv_sec

/**
 * @brief Last file status change timestamp in seconds.
 */
#define st_ctime st_ctim.tv_sec

/**
 * @brief Last data modification timestamp in seconds.
 */
#define st_mtime st_mtim.tv_sec
#endif

#endif

/**
 * @brief File type bit mask.
 */
#define _IFMT	0170000 /* type of file */

/**
 * @brief Directory file type bit.
 */
#define _IFDIR	0040000 /* directory */

/**
 * @brief Character special file type bit.
 */
#define _IFCHR	0020000 /* character special */

/**
 * @brief Block special file type bit.
 */
#define _IFBLK	0060000 /* block special */

/**
 * @brief Regular file type bit.
 */
#define _IFREG	0100000 /* regular */

/**
 * @brief Symbolic link file type bit.
 */
#define _IFLNK	0120000 /* symbolic link */

/**
 * @brief Socket file type bit.
 */
#define _IFSOCK 0140000 /* socket */

/**
 * @brief FIFO file type bit.
 */
#define _IFIFO	0010000 /* fifo */

/**
 * @brief Size of a block in bytes.
 */
#define S_BLKSIZE 1024 /* size of a block */

/**
 * @brief Set-user-ID on execution bit.
 */
#define S_ISUID 0004000 /* set user id on execution */

/**
 * @brief Set-group-ID on execution bit.
 */
#define S_ISGID 0002000 /* set group id on execution */

/**
 * @brief Sticky bit (restricted deletion).
 */
#define S_ISVTX 0001000 /* save swapped text even after use */
#if __BSD_VISIBLE
/**
 * @brief Read permission for owner (BSD name for @c S_IRUSR).
 */
#define S_IREAD	 0000400 /* read permission, owner */

/**
 * @brief Write permission for owner (BSD name for @c S_IWUSR).
 */
#define S_IWRITE 0000200 /* write permission, owner */

/**
 * @brief Execute/search permission for owner (BSD name for @c S_IXUSR).
 */
#define S_IEXEC	 0000100 /* execute/search permission, owner */

/**
 * @brief Enforcement-mode record locking.
 */
#define S_ENFMT	 0002000 /* enforcement-mode locking */
#endif			 /* !_BSD_VISIBLE */

/**
 * @brief Bit mask for the file type bits in st_mode.
 */
#define S_IFMT	 _IFMT

/**
 * @brief Directory.
 */
#define S_IFDIR	 _IFDIR

/**
 * @brief Character special file.
 */
#define S_IFCHR	 _IFCHR

/**
 * @brief Block special file.
 */
#define S_IFBLK	 _IFBLK

/**
 * @brief Regular file.
 */
#define S_IFREG	 _IFREG

/**
 * @brief Symbolic link.
 */
#define S_IFLNK	 _IFLNK

/**
 * @brief Socket.
 */
#define S_IFSOCK _IFSOCK

/**
 * @brief FIFO special file.
 */
#define S_IFIFO	 _IFIFO

#ifdef _WIN32
/*
 * The Windows header files define _S_ forms of these, so we do too
 * for easier portability.
 */

/**
 * @brief File type bit mask.
 */
#define _S_IFMT	  _IFMT

/**
 * @brief Directory file type bit.
 */
#define _S_IFDIR  _IFDIR

/**
 * @brief Character special file type bit.
 */
#define _S_IFCHR  _IFCHR

/**
 * @brief FIFO file type bit.
 */
#define _S_IFIFO  _IFIFO

/**
 * @brief Regular file type bit.
 */
#define _S_IFREG  _IFREG

/**
 * @brief Owner read permission bit.
 */
#define _S_IREAD  0000400

/**
 * @brief Owner write permission bit.
 */
#define _S_IWRITE 0000200

/**
 * @brief Owner execute permission bit.
 */
#define _S_IEXEC  0000100
#endif

/**
 * @brief Read, write, execute/search permission for owner.
 */
#define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)

/**
 * @brief Read permission for owner.
 */
#define S_IRUSR 0000400 /* read permission, owner */

/**
 * @brief Write permission for owner.
 */
#define S_IWUSR 0000200 /* write permission, owner */

/**
 * @brief Execute/search permission for owner.
 */
#define S_IXUSR 0000100 /* execute/search permission, owner */

/**
 * @brief Read, write, execute/search permission for group.
 */
#define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)

/**
 * @brief Read permission for group.
 */
#define S_IRGRP 0000040 /* read permission, group */

/**
 * @brief Write permission for group.
 */
#define S_IWGRP 0000020 /* write permission, grougroup */

/**
 * @brief Execute/search permission for group.
 */
#define S_IXGRP 0000010 /* execute/search permission, group */

/**
 * @brief Read, write, execute/search permission for others.
 */
#define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)

/**
 * @brief Read permission for others.
 */
#define S_IROTH 0000004 /* read permission, other */

/**
 * @brief Write permission for others.
 */
#define S_IWOTH 0000002 /* write permission, other */

/**
 * @brief Execute/search permission for others.
 */
#define S_IXOTH 0000001 /* execute/search permission, other */

#if __BSD_VISIBLE
/**
 * @brief Access permission bits.
 */
#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO)				/* 0777 */

/**
 * @brief All file permission and mode bits.
 */
#define ALLPERMS    (S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO) /* 07777 */

/**
 * @brief Default file creation mode.
 */
#define DEFFILEMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) /* 0666 */
#endif

/**
 * @brief Test whether @p m is a block special file.
 */
#define S_ISBLK(m)  (((m)&_IFMT) == _IFBLK)

/**
 * @brief Test whether @p m is a character special file.
 */
#define S_ISCHR(m)  (((m)&_IFMT) == _IFCHR)

/**
 * @brief Test whether @p m is a directory.
 */
#define S_ISDIR(m)  (((m)&_IFMT) == _IFDIR)

/**
 * @brief Test whether @p m is a FIFO.
 */
#define S_ISFIFO(m) (((m)&_IFMT) == _IFIFO)

/**
 * @brief Test whether @p m is a regular file.
 */
#define S_ISREG(m)  (((m)&_IFMT) == _IFREG)

/**
 * @brief Test whether @p m is a symbolic link.
 */
#define S_ISLNK(m)  (((m)&_IFMT) == _IFLNK)

/**
 * @brief Test whether @p m is a socket.
 */
#define S_ISSOCK(m) (((m)&_IFMT) == _IFSOCK)

#if defined(__CYGWIN__) || defined(__rtems__)
/**
 * @brief Set file access and modification times.
 *
 * @posix_func{futimens}
 */
/* Special tv_nsec values for futimens(2) and utimensat(2). */

/**
 * @brief Set timestamp to the current time.
 */
#define UTIME_NOW  -2L

/**
 * @brief Leave timestamp unchanged.
 */
#define UTIME_OMIT -1L
#endif

/**
 * @brief Change the mode of a file.
 *
 * @param __path Pathname of the file.
 * @param __mode New file mode (S_I* file permission bits).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{chmod}
 */
int chmod(const char *__path, mode_t __mode);

/**
 * @brief Change the mode of an open file.
 *
 * @param __fd   File descriptor of an open file.
 * @param __mode New file mode (S_I* file permission bits).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{fchmod}
 */
int fchmod(int __fd, mode_t __mode);

/**
 * @brief Get status of an open file.
 *
 * @param __fd   File descriptor of an open file.
 * @param __sbuf Output: file status information.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{fstat}
 */
int fstat(int __fd, struct stat *__sbuf);

/**
 * @brief Create a directory.
 *
 * @param _path  Pathname of the directory to create.
 * @param __mode Permission bits for the new directory (S_I* file permission bits).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mkdir}
 */
int mkdir(const char *_path, mode_t __mode);

/**
 * @brief Create a FIFO special file.
 *
 * @param __path Pathname of the FIFO to create.
 * @param __mode Permission bits for the new FIFO (S_I* file permission bits).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mkfifo}
 */
int mkfifo(const char *__path, mode_t __mode);

/**
 * @brief Get status of a file by path (follows symbolic links).
 *
 * @param __path Pathname of the file.
 * @param __sbuf Output: file status information.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{stat}
 */
int stat(const char *__restrict __path, struct stat *__restrict __sbuf);

/**
 * @brief Set the file mode creation mask.
 *
 * @param __mask New file mode creation mask (S_I* file permission bits).
 *
 * @return The previous value of the file mode creation mask.
 *
 * @posix_func{umask}
 */
mode_t umask(mode_t __mask);

#if defined(__SPU__) || defined(__rtems__) || defined(__CYGWIN__) && !defined(__INSIDE_CYGWIN__)
/**
 * @brief Get status of a file (does not follow symbolic links).
 *
 * @param __path Pathname of the file.
 * @param __buf  Output: file status information.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{lstat}
 */
int lstat(const char *__restrict __path, struct stat *__restrict __buf);

/**
 * @brief Create a special or regular file.
 *
 * @param __path Pathname of the file to create.
 * @param __mode File type and permission bits for the new file.
 * @param __dev  Device ID (for block or character special files).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mknod}
 */
int mknod(const char *__path, mode_t __mode, dev_t __dev);
#endif

#if __ATFILE_VISIBLE && !defined(__INSIDE_CYGWIN__)
/**
 * @brief Change the mode of a file relative to a directory descriptor.
 *
 * @param __fd   Directory file descriptor that relative @p __path is resolved against.
 * @param __path Pathname of the file.
 * @param __mode New file mode (S_I* file permission bits).
 * @param __flag Flags controlling pathname resolution, such as @c AT_SYMLINK_NOFOLLOW.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{fchmodat}
 */
int fchmodat(int __fd, const char *__path, mode_t __mode, int __flag);

/**
 * @brief Get status of a file relative to a directory descriptor.
 *
 * @param __fd   Directory file descriptor that relative @p __path is resolved against.
 * @param __path Pathname of the file.
 * @param __buf  Output: file status information.
 * @param __flag Flags controlling pathname resolution, such as @c AT_SYMLINK_NOFOLLOW.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{fstatat}
 */
int fstatat(int __fd, const char *__restrict __path, struct stat *__restrict __buf, int __flag);

/**
 * @brief Create a directory relative to a directory descriptor.
 *
 * @param __fd   Directory file descriptor that relative @p __path is resolved against.
 * @param __path Pathname of the directory to create.
 * @param __mode Permission bits for the new directory (S_I* file permission bits).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mkdirat}
 */
int mkdirat(int __fd, const char *__path, mode_t __mode);

/**
 * @brief Create a FIFO special file relative to a directory descriptor.
 *
 * @param __fd   Directory file descriptor that relative @p __path is resolved against.
 * @param __path Pathname of the FIFO to create.
 * @param __mode Permission bits for the new FIFO (S_I* file permission bits).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mkfifoat}
 */
int mkfifoat(int __fd, const char *__path, mode_t __mode);

/**
 * @brief Create a special or regular file relative to a directory descriptor.
 *
 * @param __fd   Directory file descriptor that relative @p __path is resolved against.
 * @param __path Pathname of the file to create.
 * @param __mode File type and permission bits for the new file.
 * @param __dev  Device ID (for block or character special files).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mknodat}
 */
int mknodat(int __fd, const char *__path, mode_t __mode, dev_t __dev);

/**
 * @brief Set file access and modification times relative to a directory descriptor.
 *
 * @param __fd    Directory file descriptor that relative @p __path is resolved against.
 * @param __path  Pathname of the file.
 * @param __times Access and modification timestamps, or NULL to set both to the
 *                current time; @c tv_nsec may be @c UTIME_NOW or @c UTIME_OMIT.
 * @param __flag  Flags controlling pathname resolution, such as @c AT_SYMLINK_NOFOLLOW.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{utimensat}
 */
int utimensat(int __fd, const char *__path, const struct timespec __times[2], int __flag);
#endif
#if __POSIX_VISIBLE >= 200809 && !defined(__INSIDE_CYGWIN__)
/**
 * @brief Set file access and modification times of an open file.
 *
 * @param __fd    File descriptor of an open file.
 * @param __times Access and modification timestamps, or NULL to set both to the
 *                current time; @c tv_nsec may be @c UTIME_NOW or @c UTIME_OMIT.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{futimens}
 */
int futimens(int __fd, const struct timespec __times[2]);
#endif

/*
 * Provide prototypes for most of the _<systemcall> names that are
 * provided in newlib for some compilers.
 */
#ifdef _LIBC
/** @cond INTERNAL_HIDDEN */
int _fstat(int __fd, struct stat *__sbuf);
/** @endcond */
/** @cond INTERNAL_HIDDEN */
int _stat(const char *__restrict __path, struct stat *__restrict __sbuf);
/** @endcond */
/** @cond INTERNAL_HIDDEN */
int _mkdir(const char *_path, mode_t __mode);
/** @endcond */
#ifdef __LARGE64_FILES
struct stat64;
/** @cond INTERNAL_HIDDEN */
int _stat64(const char *__restrict __path, struct stat64 *__restrict __sbuf);
/** @endcond */
/** @cond INTERNAL_HIDDEN */
int _fstat64(int __fd, struct stat64 *__sbuf);
/** @endcond */
#endif
#endif

#endif /* !_STAT_H_ */
#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_INCLUDE_POSIX_SYS_STAT_H_ */
