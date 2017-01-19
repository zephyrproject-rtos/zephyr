/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _debug__mem_safe__h_
#define _debug__mem_safe__h_

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Safe memory access routines
 *
 * This module provides functions for safely writing to and reading from
 * memory, as well as probing if a memory address is accessible. For the
 * latter, permissions can be specified (read/write).
 */

#define SYS_MEM_SAFE_READ  0
#define SYS_MEM_SAFE_WRITE 1

/**
 * @brief verify memory at an address is accessible
 *
 * Probe memory for read or write access by specifying the @a perm, either
 * SYS_MEM_SAFE_WRITE or SYS_MEM_SAFE_READ.  A size @a num_bytes specifying
 * the number of bytes to access must also be specified: for a 32-bit system,
 * it can only take the values 1,2 or 4 (8/16/32 bits). Both @a p and @a buf
 * must be naturally aligned to @a num_bytes.
 *
 * On a read, the value read from the memory location @a p is returned through
 * @a buf; on a write, the value contained in @a buf is written to memory at
 * memory location @a p.
 *
 * @param p The pointer (address) to the location to probe
 * @param perm Either SYS_MEM_PROBE_READ or SYS_MEM_PROBE_WRITE
 * @param num_bytes The number of bytes to probe, must be either 1, 2 or 4
 * @param buf On reads, will contain the value read; on writes, contains the
 *            value to write
 *
 * @retval 0 OK
 * @retval -EINVAL If an invalid parameter is passed
 * @retval -EFAULT If the address is not accessible.
 */

extern int _mem_probe(void *p, int perm, size_t num_bytes, void *buf);

/**
 * @brief safely read memory
 *
 * @details Read @a num_bytes bytes at address @a src from buffer pointed to
 * by @a buf. The @a width parameter specifies bus width of each memory access
 * in bytes. If @a width is 0 a target optimal access width is used. All other
 * values of access width are target dependent and optional. When @a width is
 * non-zero, the both @a num_bytes and @a src must be a multiple of @a width.
 *
 * @param src The address to read from
 * @param buf The destination buffer to receive the data read
 * @param num_bytes The number of bytes to read
 * @param width The access width
 *
 * @retval 0 OK
 * @retval -EFAULT If there was an error reading memory
 * @retval -EINVAL If access width, num_bytes and addresses are not compatible.
 */

extern int _mem_safe_read(void *src, char *buf, size_t num_bytes,
								int width);

/**
 * @brief safely write memory
 *
 * @details Write @a num_bytes bytes to address @a dest from buffer pointed to
 * by @a buf. The @a width parameter specifies bus width of each memory access
 * in bytes. If @a width is 0 a target optimal access width is used. All other
 * values of access width are target dependent and optional. When @a width is
 * non-zero, the both @a num_bytes and @a dest must be a multiple of @a width.
 *
 * @param dest The address to write to
 * @param buf The source buffer to write in memory
 * @param num_bytes The number of bytes to write
 * @param width The access width
 *
 * @retval 0 OK
 * @retval -EFAULT If there was an error writing memory
 * @retval -EINVAL If access width, num_bytes and addresses are not compatible.
 */

extern int _mem_safe_write(void *dest, char *buf, size_t num_bytes,
								int width);

/**
 * @brief write to text section
 *
 * @details Write @a num_bytes bytes to address @a dest from buffer pointed to
 * by @a buf.
 *
 * @param dest The address to write to
 * @param buf The source buffer to write in memory
 * @param num_bytes The number of bytes to write
 *
 * @retval 0 Success
 * @retval -EFAULT If there was an error writing memory
 */

extern int _mem_safe_write_to_text_section(void *dest, char *buf,
		size_t num_bytes);

/**
 * @brief add to the table of valid regions
 *
 * @details Add a new region that is considered valid to read from or both
 * read from and write to. The region starts at @a addr and its size is @a
 * num_bytes. The read/write permissions are specified via @a perm and can
 * take the values either SYS_MEM_SAFE_READ or SYS_MEM_SAFE_WRITE.
 *
 * The table size is specified via the CONFIG_MEM_SAFE_NUM_REGIONS kconfig
 * option.
 *
 * If the implementation of safe memory access chosen does not need this API,
 * it is still available, but results in a no-op and always returns success
 * (0).
 *
 * @param addr The address to write to
 * @param num_bytes The size of the region in bytes
 * @param perm The access permissions, either SYS_MEM_SAFE_WRITE or
 *             SYS_MEM_SAFE_READ
 *
 * @retval 0 OK
 * @retval -ENOMEM If there there is no space left in the table of regions
 * @retval -EINVAL If passing invalid permissions
 */

#ifdef CONFIG_MEM_SAFE_CHECK_BOUNDARIES
extern int _mem_safe_region_add(void *addr, size_t num_bytes, int perm);
#else
static inline int _mem_safe_region_add(void *addr, size_t num_bytes,
											int perm)
{
	return 0;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _debug__mem_safe__h_ */
