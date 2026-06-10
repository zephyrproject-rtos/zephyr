/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Standard symbolic constants and types.
 * @ingroup posix
 *
 * Provides the POSIX miscellaneous functions: file I/O (read, write, close, lseek),
 * file system operations, process identification, and process environment queries.
 *
 * @posix_header{unistd.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_UNISTD_H_
#define ZEPHYR_INCLUDE_POSIX_UNISTD_H_

#include <time.h>

#include <zephyr/posix/posix_types.h>

#ifdef CONFIG_POSIX_API
#include <zephyr/fs/fs.h>
#endif
#include <zephyr/posix/sys/confstr.h>
#include <zephyr/posix/sys/stat.h>
#include <zephyr/posix/sys/sysconf.h>

#include "posix_features.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_POSIX_API
/* File related operations */

/**
 * @brief Close a file descriptor.
 *
 * @param file File descriptor to close.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{close}
 */
int close(int file);

/**
 * @brief Write to a file descriptor.
 *
 * @param file   File descriptor.
 * @param buffer Buffer containing the data to write.
 * @param count  Number of bytes to write.
 *
 * @return Number of bytes written on success, or -1 with errno set on failure.
 *
 * @posix_func{write}
 */
ssize_t write(int file, const void *buffer, size_t count);

/**
 * @brief Read from a file descriptor.
 *
 * @param file   File descriptor.
 * @param buffer Buffer into which the data is read.
 * @param count  Maximum number of bytes to read.
 *
 * @return Number of bytes read on success, 0 at end-of-file, or -1 with errno set on failure.
 *
 * @posix_func{read}
 */
ssize_t read(int file, void *buffer, size_t count);

/**
 * @brief Reposition the file offset of an open file descriptor.
 *
 * @param file   File descriptor.
 * @param offset New offset, relative to the position selected by @p whence.
 * @param whence One of @c SEEK_SET, @c SEEK_CUR, or @c SEEK_END.
 *
 * @return Resulting offset, in bytes from the beginning of the file, or -1 with errno set on
 *         failure.
 *
 * @posix_func{lseek}
 */
off_t lseek(int file, off_t offset, int whence);

/**
 * @brief Synchronize an open file's data and metadata to storage.
 *
 * @param fd File descriptor.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{fsync}
 */
int fsync(int fd);

/**
 * @brief Truncate an open file to a specified length.
 *
 * @param fd     File descriptor open for writing.
 * @param length New length of the file in bytes.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{ftruncate}
 */
int ftruncate(int fd, off_t length);

#ifdef CONFIG_POSIX_SYNCHRONIZED_IO
/**
 * @brief Synchronize the data of an open file to storage (without metadata).
 *
 * @param fd File descriptor.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{fdatasync}
 */
int fdatasync(int fd);
#endif /* CONFIG_POSIX_SYNCHRONIZED_IO */

/* File System related operations */

/**
 * @brief Rename a file.
 *
 * @param old  Pathname of the file to rename.
 * @param newp New pathname of the file.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{rename}
 */
int rename(const char *old, const char *newp);

/**
 * @brief Remove a directory entry.
 *
 * @param path Pathname of the file to remove.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{unlink}
 */
int unlink(const char *path);

/**
 * @brief Get file status.
 *
 * @param path Pathname of the file to query.
 * @param buf  Structure filled in with the file status.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{stat}
 */
int stat(const char *path, struct stat *buf);

/**
 * @brief Make a directory.
 *
 * @param path Pathname of the directory to create.
 * @param mode Permission bits of the new directory.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mkdir}
 */
int mkdir(const char *path, mode_t mode);

/**
 * @brief Remove an empty directory.
 *
 * @param path Pathname of the directory to remove.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{rmdir}
 */
int rmdir(const char *path);

/**
 * @brief Terminate the calling process without calling atexit() handlers.
 *
 * This function does not return.
 *
 * @param status Exit status of the process.
 *
 * @posix_func{_exit}
 */
FUNC_NORETURN void _exit(int status);

/**
 * @brief Get the name of the current host.
 *
 * @param buf Buffer to receive the null-terminated host name.
 * @param len Size of @p buf in bytes.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{gethostname}
 */
int gethostname(char *buf, size_t len);

#endif /* CONFIG_POSIX_API */

#ifdef CONFIG_POSIX_C_LIB_EXT
/**
 * @brief Parse command-line options.
 *
 * @param argc      Argument count, as passed to main().
 * @param argv      Argument array, as passed to main().
 * @param optstring String of recognized option characters; a character followed by ':' takes
 *                  an argument.
 *
 * @return The next option character, '?' or ':' on error, or -1 when option parsing is complete.
 *
 * @posix_func{getopt}
 */
int getopt(int argc, char *const argv[], const char *optstring);
extern char *optarg;
extern int opterr, optind, optopt;
#endif

/**
 * @brief Fill a buffer with cryptographically secure random bytes.
 * @ingroup posix_extensions
 *
 * @param buffer Buffer to fill with random bytes.
 * @param length Number of bytes to write into @p buffer (at most 256).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @compat_api{getentropy}
 */
int getentropy(void *buffer, size_t length);

/**
 * @brief Get the process ID of the calling process.
 *
 * @return Process ID of the calling process. This function is always successful.
 *
 * @posix_func{getpid}
 */
pid_t getpid(void);

/**
 * @brief Suspend execution for at least the specified number of seconds.
 *
 * @param seconds Number of seconds to sleep.
 *
 * @return 0 if the requested time has elapsed, or the number of unslept seconds if
 *         interrupted by a signal.
 *
 * @posix_func{sleep}
 */
unsigned sleep(unsigned int seconds);

/**
 * @brief Suspend execution for at least the specified number of microseconds.
 *
 * @param useconds Number of microseconds to sleep (must be less than 1000000).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{usleep}
 */
int usleep(useconds_t useconds);
#if _POSIX_C_SOURCE >= 2
/**
 * @brief Determine the value of a configurable system variable by name string.
 *
 * @param name Variable to query (@c _CS_* constant).
 * @param buf  Buffer to receive the null-terminated value, or NULL.
 * @param len  Size of @p buf in bytes.
 *
 * @return Size of the buffer needed to hold the entire value (including the terminating
 *         null byte), or 0 if @p name is invalid or has no configuration-defined value.
 *
 * @posix_func{confstr}
 */
size_t confstr(int name, char *buf, size_t len);
#endif

#ifdef CONFIG_POSIX_SYSCONF_IMPL_MACRO
/**
 * @brief Get a configurable system variable.
 */
#define sysconf(x) (long)CONCAT(__z_posix_sysconf, x)
#else
/**
 * @brief Get a configurable system variable.
 *
 * @param opt Variable to query (@c _SC_* constant).
 *
 * @return Value of the system variable, or -1 if @p opt is invalid or the value is
 *         indeterminate.
 *
 * @posix_func{sysconf}
 */
long sysconf(int opt);
#endif /* CONFIG_POSIX_SYSCONF_IMPL_FULL */

#if _XOPEN_SOURCE >= 500
/**
 * @brief Get an identifier for the current host.
 *
 * @return 32-bit identifier for the current host.
 *
 * @posix_func{gethostid}
 */
long gethostid(void);
#endif

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_UNISTD_H_ */
