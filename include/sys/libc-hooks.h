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
#if defined(CONFIG_NEWLIB_LIBC)
/* If we are using newlib, the heap arena is in one of two areas:
 *  - If we have an MPU that requires power of two alignment, the heap bounds
 *    must be specified in Kconfig via CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE.
 *  - Otherwise, the heap arena on most arches starts at a suitably
 *    aligned base addreess after the `_end` linker symbol, through to the end
 *    of system RAM.
 */
#if (!defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT) || \
     (defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT) && \
      CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE))
#define Z_MALLOC_PARTITION_EXISTS 1
extern struct k_mem_partition z_malloc_partition;
#endif
#elif defined(CONFIG_MINIMAL_LIBC)
#if (CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE > 0)
/* Minimal libc by default has no malloc arena, its size must be set in
 * Kconfig via CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE
 */
#define Z_MALLOC_PARTITION_EXISTS 1
#endif /* CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE > 0 */
#endif /* CONFIG_MINIMAL_LIBC */

#ifdef Z_MALLOC_PARTITION_EXISTS
/* Memory partition containing the libc malloc arena. Configuration controls
 * whether this is available, and an arena size may need to be set.
 */
extern struct k_mem_partition z_malloc_partition;
#endif

#if defined(CONFIG_NEWLIB_LIBC) || defined(CONFIG_STACK_CANARIES) || \
    defined(CONFIG_NEED_LIBC_MEM_PARTITION)
/* Minimal libc has no globals. We do put the stack canary global in the
 * libc partition since it is not worth placing in a partition of its own.
 *
 * Some architectures require a global pointer for thread local storage,
 * which is placed inside the libc partition.
 */
#define Z_LIBC_PARTITION_EXISTS 1

/* C library globals, except the malloc arena */
extern struct k_mem_partition z_libc_partition;
#endif
#endif /* CONFIG_USERSPACE */

#include <syscalls/libc-hooks.h>

#endif /* ZEPHYR_INCLUDE_SYS_LIBC_HOOKS_H_ */
