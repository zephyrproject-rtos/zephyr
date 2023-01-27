/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_ARCH_MICROBLAZE_ARCH_INLINES_H
#define ZEPHYR_INCLUDE_ARCH_MICROBLAZE_ARCH_INLINES_H

#include <zephyr/kernel_structs.h>
#include <zephyr/devicetree.h>

#define _CPU DT_PATH(cpus, cpu_0)

static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return CONFIG_MP_MAX_NUM_CPUS;
}

static inline uint32_t arch_get_cpu_clock_frequency(void)
{
	return DT_PROP_OR(_CPU, clock_frequency, 0);
}

#endif /* ZEPHYR_INCLUDE_ARCH_MICROBLAZE_ARCH_INLINES_H */
