/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ERRNO_PRIVATE_H_
#define ZEPHYR_INCLUDE_SYS_ERRNO_PRIVATE_H_

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NOTE: located here to avoid include dependency loops between errno.h
 * and kernel.h
 */

#ifdef CONFIG_LIBC_ERRNO
#include <errno.h>

static inline int *z_errno(void)
{
	return &errno;
}

#elif defined(CONFIG_ERRNO_IN_TLS)
extern __thread int z_errno_var;

static inline int *z_errno(void)
{
	return &z_errno_var;
}
#else
/**
 * return a pointer to a memory location containing errno
 *
 * errno is thread-specific, and can't just be a global. This pointer
 * is guaranteed to be read/writable from user mode.
 *
 * @return Memory location of errno data for current thread
 */
__syscall int *z_errno(void);

#endif /* CONFIG_ERRNO_IN_TLS */

#ifdef __cplusplus
}
#endif

#if !defined(CONFIG_ERRNO_IN_TLS) && !defined(CONFIG_LIBC_ERRNO)
#include <zephyr/syscalls/errno_private.h>
#endif /* CONFIG_ERRNO_IN_TLS */

#endif /* ZEPHYR_INCLUDE_SYS_ERRNO_PRIVATE_H_ */
