/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ZVFS_LIBC_H_
#define ZEPHYR_INCLUDE_SYS_ZVFS_LIBC_H_

/**
 * @file
 * @brief ZVFS C library FILE<->fd binding hooks and backing store.
 *
 * The binding between a C library @c FILE and a ZVFS file descriptor can only be
 * expressed in terms of each C library's stdio internals, so it lives inside the
 * C library (not in generic Zephyr code - that is what made the old
 * FILE-casting zvfs_fdopen()/zvfs_fileno() undefined behaviour). Each C library
 * provides two mandatory, non-POSIX, ISO-C-only hooks:
 *
 * - zvfs_libc_fdopen()
 * - zvfs_libc_fileno()
 *
 * Storage for the @c FILE is the C library's own store
 * (lib/libc/common/source/stdio/zvfs_libc_file.c), in its memory partition, so
 * user-mode processes open and close streams without kernel objects. The
 * dependency is strictly downward: POSIX -> libc hooks -> libc store; zvfs
 * never calls up into the C library.
 *
 * @note This is the small header introduced alongside the FILE/fd fix; its
 *       contents are intended to be folded into the consolidated zvfs.h later.
 */

#include <stddef.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_ZVFS_LIBC_FILE)

/** @cond INTERNAL */
#if CONFIG_ZVFS_LIBC_FILE_SIZE > 0
#define ZVFS_LIBC_FILE_SIZE CONFIG_ZVFS_LIBC_FILE_SIZE
#else
#define ZVFS_LIBC_FILE_SIZE sizeof(*((FILE *)0))
#endif

#if CONFIG_ZVFS_LIBC_FILE_ALIGN > 0
#define ZVFS_LIBC_FILE_ALIGN CONFIG_ZVFS_LIBC_FILE_ALIGN
#else
#define ZVFS_LIBC_FILE_ALIGN _Alignof(*((FILE *)0))
#endif
/** @endcond */

#endif /* CONFIG_ZVFS_LIBC_FILE */

/**
 * @brief Associate a C library @c FILE with an open file descriptor.
 *
 * Implemented by each C library. The returned @c FILE is backed by the
 * statically-allocated ZVFS FILE store (no dynamic allocation) and is released
 * when the underlying descriptor is closed via @ref zvfs_close (e.g. through
 * @c fclose() or @c close()).
 *
 * @param fd Open file descriptor.
 * @param mode Mode string as passed to @c fopen()/fdopen().
 *
 * @return Pointer to a @c FILE on success, or NULL on error (errno is set).
 */
FILE *zvfs_libc_fdopen(int fd, const char *mode);

/**
 * @brief Return the file descriptor associated with a C library @c FILE.
 *
 * Implemented by each C library.
 *
 * @param fp A @c FILE previously returned by @ref zvfs_libc_fdopen (or a
 *	     standard stream).
 *
 * @return The associated file descriptor, or -1 on error (errno is set).
 */
int zvfs_libc_fileno(FILE *fp);

#if defined(CONFIG_ZVFS_LIBC_FILE)
/**
 * @brief Claim a raw, uninitialized @c FILE-sized slot from the C library's store.
 *
 * Called by a C library's @ref zvfs_libc_fdopen hook, which initializes the
 * returned storage as a @c FILE using its own stdio internals. The slot is
 * returned by fclose() through @ref zvfs_libc_file_free; closing the
 * descriptor underneath a stream does not release it.
 *
 * @param fd Open file descriptor the slot will back.
 *
 * @return Pointer to @c ZVFS_LIBC_FILE_SIZE bytes of storage, or NULL if none
 *	   is available.
 */
void *zvfs_libc_file_alloc(int fd);

/**
 * @brief Return a previously claimed slot to the C library's store.
 *
 * Storage not owned by the store (e.g. standard streams) is silently ignored.
 *
 * @param fp Storage previously returned by @ref zvfs_libc_file_alloc.
 */
void zvfs_libc_file_free(void *fp);
#else
/* no store in this configuration: streams are whatever the C library provides */
static inline void *zvfs_libc_file_alloc(int fd)
{
	ARG_UNUSED(fd);
	return NULL;
}

static inline void zvfs_libc_file_free(void *fp)
{
	ARG_UNUSED(fp);
}
#endif /* CONFIG_ZVFS_LIBC_FILE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ZVFS_LIBC_H_ */
