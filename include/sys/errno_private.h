/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ERRNO_PRIVATE_H_
#define ZEPHYR_INCLUDE_SYS_ERRNO_PRIVATE_H_

#include <toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NOTE: located here to avoid include dependency loops between errno.h
 * and kernel.h
 */

/**
 * return a pointer to a memory location containing errno
 *
 * errno is thread-specific, and can't just be a global. This pointer
 * is guaranteed to be read/writable from user mode.
 *
 * @return Memory location of errno data for current thread
 */
__syscall int *z_errno(void);

#ifdef __cplusplus
}
#endif

#include <syscalls/errno_private.h>

#endif /* ZEPHYR_INCLUDE_SYS_ERRNO_PRIVATE_H_ */
