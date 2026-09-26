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
#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

struct zvfs_stat {
	off_t size;
	uint32_t mode;
	/** Preferred I/O block size, 0 when unknown. */
	unsigned long blksize;
	/** Number of 512 B blocks allocated, 0 when unknown. */
	unsigned long blocks;
	/** Number of hard links. */
	unsigned long nlink;
	/** Time of last access, zero when the backend does not record it. */
	struct timespec atime;
	/** Time of last modification, zero when the backend does not record it. */
	struct timespec mtime;
	/** Time of last status change, zero when the backend does not record it. */
	struct timespec ctime;
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
 * @p arg is command-specific: an @c int for most requests, or a pointer to
 * @ref zvfs_flock for record-lock commands. Commands and flag values use ZVFS
 * constants (see @ref fdtable.h), not POSIX @c fcntl.h values.
 *
 * @param fd File descriptor.
 * @param cmd ZVFS control command.
 * @param arg Command argument.
 *
 * @return Command-specific result, or -1 on error (errno is set).
 */
__syscall int zvfs_fcntl(int fd, int cmd, uintptr_t arg);

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

/**
 * @name zvfs_mmap() page protections
 * @{
 */
/** Pages may not be accessed */
#define ZVFS_PROT_NONE  0
/** Pages may be read */
#define ZVFS_PROT_READ  BIT(0)
/** Pages may be written */
#define ZVFS_PROT_WRITE BIT(1)
/** Pages may be executed */
#define ZVFS_PROT_EXEC  BIT(2)
/** @} */

/**
 * @name zvfs_mmap() mapping flags
 * @{
 */
/** Modifications are shared with the file and with other shared mappings of it */
#define ZVFS_MAP_SHARED    BIT(0)
/** Modifications are private to the mapping */
#define ZVFS_MAP_PRIVATE   BIT(1)
/** The mapping is not backed by a file descriptor */
#define ZVFS_MAP_ANONYMOUS BIT(2)
/** @} */

/**
 * @name zvfs_msync() flags
 * @{
 */
/** Write modified pages back without waiting for completion */
#define ZVFS_MS_ASYNC      BIT(0)
/** Re-read the pages from the file */
#define ZVFS_MS_INVALIDATE BIT(1)
/** Write modified pages back and wait for completion */
#define ZVFS_MS_SYNC       BIT(2)
/** @} */

/**
 * @brief Map a file descriptor or anonymous memory into the address space.
 *
 * The mapping covers @p len bytes rounded up to the mapping page size, starting at
 * offset @p off of the file, and belongs to the calling thread: it is released when
 * that thread exits. A mapping keeps the file open until it is unmapped.
 *
 * @param len Number of bytes to map, greater than zero.
 * @param prot Page protection, a combination of ZVFS_PROT_* values.
 * @param flags Exactly one of ZVFS_MAP_SHARED and ZVFS_MAP_PRIVATE, optionally with
 *              ZVFS_MAP_ANONYMOUS.
 * @param fd File descriptor to map, ignored with ZVFS_MAP_ANONYMOUS.
 * @param off Offset into the file, a multiple of the mapping page size.
 * @param[out] addr Base address of the mapping.
 *
 * @return 0 on success, or -1 on error (errno is set).
 */
__syscall int zvfs_mmap(size_t len, int prot, int flags, int fd, size_t off, void **addr);

/**
 * @brief Unmap whole mappings inside an address range.
 *
 * Modifications to shared file mappings are written back first. A range that
 * covers only part of a mapping is rejected.
 *
 * @param addr Base address, a multiple of the mapping page size.
 * @param len Number of bytes, greater than zero.
 *
 * @return 0 on success, or -1 on error (errno is set).
 */
__syscall int zvfs_munmap(void *addr, size_t len);

/**
 * @brief Synchronize shared file mappings inside an address range with the file.
 *
 * @param addr Base address, a multiple of the mapping page size.
 * @param len Number of bytes.
 * @param flags ZVFS_MS_SYNC or ZVFS_MS_ASYNC to write back, optionally with
 *              ZVFS_MS_INVALIDATE to re-read the pages afterwards.
 *
 * @return 0 on success, or -1 on error (errno is set).
 */
__syscall int zvfs_msync(void *addr, size_t len, int flags);

/**
 * @brief Change the protection of whole mappings inside an address range.
 *
 * @param addr Base address, a multiple of the mapping page size.
 * @param len Number of bytes.
 * @param prot New page protection, a combination of ZVFS_PROT_* values.
 *
 * @return 0 on success, or -1 on error (errno is set).
 */
__syscall int zvfs_mprotect(void *addr, size_t len, int prot);

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
