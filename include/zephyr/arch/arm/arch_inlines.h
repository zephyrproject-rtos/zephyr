/*
 * Copyright 2022 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_ARCH_INLINES_H
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_ARCH_INLINES_H

#include <zephyr/kernel_structs.h>
#include <zephyr/arch/arm/cortex_a_r/lib_helpers.h>
#include <zephyr/arch/arm/cortex_a_r/tpidruro.h>

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
	/* Dummy implementation always return the first cpu */
	return (_cpu_t *)(read_tpidruro() & TPIDRURO_CURR_CPU);
}

static ALWAYS_INLINE uint32_t arch_proc_id(void)
{
	/*
	 * Placeholder implementation to be replaced with an architecture
	 * specific call to get processor ID
	 */
	return arch_curr_cpu()->id;
}

static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return CONFIG_MP_MAX_NUM_CPUS;
}

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_ARCH_INLINES_H */
