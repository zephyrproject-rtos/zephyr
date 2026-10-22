/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon architecture inline function implementations
 */

#ifndef ZEPHYR_INCLUDE_ARCH_HEXAGON_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_HEXAGON_ARCH_INLINES_H_

#include <zephyr/kernel_structs.h>

/**
 * @brief Return the number of CPUs.
 *
 * @return Number of CPUs on the system.
 */
static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return CONFIG_MP_MAX_NUM_CPUS;
}

#endif /* ZEPHYR_INCLUDE_ARCH_HEXAGON_ARCH_INLINES_H_ */
