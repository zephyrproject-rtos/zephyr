/*
 * Copyright (c) 2013-2016 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions (POSIX)
 *
 */

#ifndef ZEPHYR_ARCH_POSIX_INCLUDE_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_POSIX_INCLUDE_KERNEL_ARCH_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <kernel_internal.h>

/* stacks */
#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)


#ifndef _ASMLANGUAGE

struct _kernel_arch {
	/* empty */
};

typedef struct _kernel_arch _kernel_arch_t;

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_POSIX_INCLUDE_KERNEL_ARCH_DATA_H_ */
