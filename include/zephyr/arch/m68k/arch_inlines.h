/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_M68K_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_M68K_ARCH_INLINES_H_

#include <zephyr/kernel_structs.h>

static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return CONFIG_MP_MAX_NUM_CPUS;
}

#endif /* ZEPHYR_INCLUDE_ARCH_M68K_ARCH_INLINES_H_ */
