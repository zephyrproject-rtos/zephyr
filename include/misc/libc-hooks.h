/*
 * Copyright (c) 2018, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MISC_LIBC_HOOKS_H_
#define ZEPHYR_INCLUDE_MISC_LIBC_HOOKS_H_

#include <toolchain.h>
#include <stdio.h>
#include <stddef.h>

/*
 * Private header for specifying accessory functions to the C library internals
 * that need to call into the kernel as system calls
 */

#ifdef CONFIG_NEWLIB_LIBC

/* syscall generation ignores preprocessor, ensure this is defined to ensure
 * we don't have compile errors
 */
#define _MLIBC_RESTRICT

__syscall int _zephyr_read(char *buf, int nbytes);

__syscall int _zephyr_write(const void *buf, int nbytes);

#ifdef CONFIG_APP_SHARED_MEM
/* Memory partition containing newlib's globals. This includes all the globals
 * within libc.a and the supporting zephyr hooks, but not the malloc arena.
 */
extern struct k_mem_partition z_newlib_partition;
#endif /* CONFIG_APP_SHARED_MEM */

#else
/* Minimal libc */

__syscall int _zephyr_fputc(int c, FILE *stream);

__syscall size_t _zephyr_fwrite(const void *_MLIBC_RESTRICT ptr, size_t size,
				size_t nitems, FILE *_MLIBC_RESTRICT stream);
#endif /* CONFIG_NEWLIB_LIBC */

#ifdef CONFIG_APP_SHARED_MEM
/* Memory partition containing the libc malloc arena */
extern struct k_mem_partition z_malloc_partition;
#endif

#include <syscalls/libc-hooks.h>

#endif /* ZEPHYR_INCLUDE_MISC_LIBC_HOOKS_H_ */
