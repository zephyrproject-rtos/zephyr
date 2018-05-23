/*
 * Copyright (c) 2018, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	linux/sleep.h
 * @brief	linux sleep primitives for libmetal.
 */

#ifndef __METAL_SLEEP__H__
#error "Include metal/sleep.h instead of metal/linux/sleep.h"
#endif

#ifndef __METAL_LINUX_SLEEP__H__
#define __METAL_LINUX_SLEEP__H__

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int __metal_sleep_usec(unsigned int usec)
{
	return usleep(usec);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_LINUX_SLEEP__H__ */
