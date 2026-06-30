/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ZVFS_H_
#define ZEPHYR_INCLUDE_SYS_ZVFS_H_

/**
 * @file
 * @brief ZVFS file-descriptor I/O accessors
 *
 * User/kernel boundary entry points for read, write, close, and related
 * operations on ZVFS file descriptors.
 */

#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

struct zvfs_stat {
	off_t size;
	uint32_t mode;
};

/**
 * @brief Read from a file descriptor at an optional explicit offset.
 *
 * @param fd File descriptor.
 * @param buf Destination buffer.
 * @param sz Number of bytes to read.
 * @param from_offset Optional explicit offset, or NULL to use the fd offset.
 *
 * @return Number of bytes read on success, or -1 on error (errno is set).
 */
__syscall ssize_t zvfs_read_offset(int fd, void *buf, size_t sz, const size_t *from_offset);

/**
 * @brief Write to a file descriptor at an optional explicit offset.
 *
 * @param fd File descriptor.
 * @param buf Source buffer.
 * @param sz Number of bytes to write.
 * @param from_offset Optional explicit offset, or NULL to use the fd offset.
 *
 * @return Number of bytes written on success, or -1 on error (errno is set).
 */
__syscall ssize_t zvfs_write_offset(int fd, const void *buf, size_t sz, const size_t *from_offset);

/**
 * @brief Close a file descriptor.
 *
 * @param fd File descriptor.
 *
 * @return 0 on success, or -1 on error (errno is set).
 */
__syscall int zvfs_close(int fd);

/**
 * @brief Reposition the offset of a file descriptor.
 *
 * @param fd File descriptor.
 * @param offset New offset value.
 * @param whence Seek origin.
 *
 * @return New offset on success, or -1 on error (errno is set).
 */
__syscall off_t zvfs_lseek(int fd, off_t offset, int whence);

/**
 * @brief Retrieve status information for a file descriptor.
 *
 * @param fd File descriptor.
 * @param buf Status buffer.
 *
 * @return 0 on success, or -1 on error (errno is set).
 */
__syscall int zvfs_fstat(int fd, struct zvfs_stat *buf);

/**
 * @brief Synchronize a file descriptor to its backing store.
 *
 * @param fd File descriptor.
 *
 * @return 0 on success, or -1 on error (errno is set).
 */
__syscall int zvfs_fsync(int fd);

/**
 * @brief Truncate a file descriptor to a given length.
 *
 * @param fd File descriptor.
 * @param length New length.
 *
 * @return 0 on success, or -1 on error (errno is set).
 */
__syscall int zvfs_ftruncate(int fd, off_t length);

/**
 * @brief Dispatch a control request on a file descriptor.
 *
 * Callers marshal arguments before dispatching through the fd vtable.
 *
 * @param fd File descriptor.
 * @param cmd Control command.
 * @param args Command arguments.
 *
 * @return Command-specific result, or -1 on error (errno is set).
 */
int zvfs_fcntl(int fd, int cmd, va_list args);

/**
 * @brief Duplicate a file descriptor to the lowest available slot >= minfd.
 *
 * @param fd File descriptor to duplicate.
 * @param minfd Minimum value for the new descriptor.
 *
 * @return New file descriptor on success, or -1 on error (errno is set).
 */
__syscall int zvfs_dup(int fd, int minfd);

/**
 * @brief Duplicate a file descriptor to a specific descriptor number.
 *
 * @param fd File descriptor to duplicate.
 * @param newfd Target descriptor number.
 *
 * @return @p newfd on success, or -1 on error (errno is set).
 */
__syscall int zvfs_dup2(int fd, int newfd);

/**
 * @brief Dispatch an I/O control request on a file descriptor.
 *
 * @param fd File descriptor.
 * @param request I/O control request number.
 * @param args Request arguments.
 *
 * @return Request-specific result, or -1 on error (errno is set).
 */
__syscall int zvfs_ioctl(int fd, unsigned long request, va_list args);

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/zvfs.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline ssize_t zvfs_read(int fd, void *buf, size_t sz)
{
	return zvfs_read_offset(fd, buf, sz, NULL);
}

static inline ssize_t zvfs_write(int fd, const void *buf, size_t sz)
{
	return zvfs_write_offset(fd, buf, sz, NULL);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ZVFS_H_ */
