/* kernel version support */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _kernel_version__h_
#define _kernel_version__h_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The kernel version has been converted from a string to a four-byte
 * quantity that is divided into two parts.
 *
 * Part 1: The three most significant bytes represent the kernel's
 * numeric version, x.y.z. These fields denote:
 *       x -- major release
 *       y -- minor release
 *       z -- patchlevel release
 * Each of these elements must therefore be in the range 0 to 255, inclusive.
 *
 * Part 2: The least significant byte is reserved for future use.
 */
#define SYS_KERNEL_VER_MAJOR(ver) ((ver >> 24) & 0xFF)
#define SYS_KERNEL_VER_MINOR(ver) ((ver >> 16) & 0xFF)
#define SYS_KERNEL_VER_PATCHLEVEL(ver) ((ver >> 8) & 0xFF)

/* kernel version routines */

extern uint32_t sys_kernel_version_get(void);

#ifdef __cplusplus
}
#endif

#endif /* _kernel_version__h_ */
