/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Memory management declarations.
 * @ingroup posix
 *
 * Provides memory mapping, shared memory objects, and memory locking.
 *
 * @posix_header{sys_mman.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_MMAN_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_MMAN_H_

#include <stddef.h>
#include <sys/types.h>

/**
 * @brief Page cannot be accessed.
 */
#define PROT_NONE  0x0

/**
 * @brief Page can be read.
 */
#define PROT_READ  0x1

/**
 * @brief Page can be written.
 */
#define PROT_WRITE 0x2

/**
 * @brief Page can be executed.
 */
#define PROT_EXEC  0x4

/**
 * @brief Changes are shared between all mappings of the same object.
 */
#define MAP_SHARED  0x1

/**
 * @brief Changes are private (copy-on-write).
 */
#define MAP_PRIVATE 0x2

/**
 * @brief Map at the exact address given to mmap().
 */
#define MAP_FIXED   0x4

/* for Linux compatibility */

/**
 * @brief Anonymous mapping; the file descriptor argument is ignored.
 */
#define MAP_ANONYMOUS 0x20

/**
 * @brief Flush modified pages to the underlying storage synchronously.
 */
#define MS_SYNC       0x0

/**
 * @brief Schedule writes and return immediately.
 */
#define MS_ASYNC      0x1

/**
 * @brief Invalidate cached data so subsequent reads reflect the file.
 */
#define MS_INVALIDATE 0x2

/**
 * @brief Value returned by mmap() on failure.
 */
#define MAP_FAILED ((void *)-1)

/**
 * @brief Lock all currently mapped pages into memory.
 */
#define MCL_CURRENT 0

/**
 * @brief Lock all future mappings into memory.
 */
#define MCL_FUTURE  1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Lock a range of the calling process's address space into memory.
 *
 * @param addr Base address of the region to lock.
 * @param len  Length of the region in bytes.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mlock}
 */
int mlock(const void *addr, size_t len);

/**
 * @brief Lock all current and/or future memory mappings of the calling process.
 *
 * @param flags @c MCL_CURRENT, @c MCL_FUTURE, or both.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mlockall}
 */
int mlockall(int flags);

/**
 * @brief Map a file or device into memory.
 *
 * @param addr   Suggested address (hint), or NULL.
 * @param len    Length of the mapping in bytes.
 * @param prot   Memory protection (@c PROT_* flags).
 * @param flags  Mapping type and options (@c MAP_* flags).
 * @param fildes File descriptor (-1 for anonymous mappings).
 * @param off    Offset within the file (must be page-aligned).
 *
 * @return Base address of the mapping, or @c MAP_FAILED on failure.
 *
 * @posix_func{mmap}
 */
void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);

/**
 * @brief Synchronize a memory mapping with the underlying storage.
 *
 * @param addr   Base address of the region (must be page-aligned).
 * @param length Length of the region in bytes.
 * @param flags  @c MS_SYNC, @c MS_ASYNC, or @c MS_INVALIDATE.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{msync}
 */
int msync(void *addr, size_t length, int flags);

/**
 * @brief Unlock a range of the calling process's address space.
 *
 * @param addr Base address of the region to unlock.
 * @param len  Length of the region in bytes.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{munlock}
 */
int munlock(const void *addr, size_t len);

/**
 * @brief Unlock all memory locked by the calling process.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{munlockall}
 */
int munlockall(void);

/**
 * @brief Unmap a previously mapped region.
 *
 * @param addr Base address of the mapping.
 * @param len  Length of the region in bytes.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{munmap}
 */
int munmap(void *addr, size_t len);

/**
 * @brief Open or create a shared memory object.
 *
 * @param name  Shared memory object name (must begin with '/').
 * @param oflag Open flags (@c O_RDONLY, @c O_RDWR, @c O_CREAT, @c O_EXCL, @c O_TRUNC).
 * @param mode  Permission bits applied if the object is created.
 *
 * @return File descriptor for the shared memory object, or -1 on failure.
 *
 * @posix_func{shm_open}
 */
int shm_open(const char *name, int oflag, mode_t mode);

/**
 * @brief Remove a shared memory object.
 *
 * @param name Shared memory object name.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{shm_unlink}
 */
int shm_unlink(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_MMAN_H_ */
