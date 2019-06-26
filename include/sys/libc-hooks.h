/*
 * Copyright (c) 2018, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_LIBC_HOOKS_H_
#define ZEPHYR_INCLUDE_SYS_LIBC_HOOKS_H_

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

__syscall int z_zephyr_read_stdin(char *buf, int nbytes);

__syscall int z_zephyr_write_stdout(const void *buf, int nbytes);

#else
/* Minimal libc */

__syscall int zephyr_fputc(int c, FILE * stream);

__syscall size_t zephyr_fwrite(const void *_MLIBC_RESTRICT ptr, size_t size,
				size_t nitems, FILE *_MLIBC_RESTRICT stream);
#endif /* CONFIG_NEWLIB_LIBC */

#ifdef CONFIG_USERSPACE
#if defined(CONFIG_NEWLIB_LIBC) || (CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE > 0)
#define Z_MALLOC_PARTITION_EXISTS 1

/* Memory partition containing the libc malloc arena */
extern struct k_mem_partition z_malloc_partition;
#endif

#if defined(CONFIG_NEWLIB_LIBC) || defined(CONFIG_STACK_CANARIES)
/* Minimal libc has no globals. We do put the stack canary global in the
 * libc partition since it is not worth placing in a partition of its own.
 */
#define Z_LIBC_PARTITION_EXISTS 1

/* C library globals, except the malloc arena */
extern struct k_mem_partition z_libc_partition;
#endif
#endif /* CONFIG_USERSPACE */

#include <syscalls/libc-hooks.h>

#endif /* ZEPHYR_INCLUDE_SYS_LIBC_HOOKS_H_ */
