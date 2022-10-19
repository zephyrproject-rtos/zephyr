/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_POSIX_ARCH_INLINES_H
#define ZEPHYR_INCLUDE_ARCH_POSIX_ARCH_INLINES_H

#include <zephyr/kernel_structs.h>

static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return CONFIG_MP_MAX_NUM_CPUS;
}

#endif /* ZEPHYR_INCLUDE_ARCH_POSIX_ARCH_INLINES_H */
