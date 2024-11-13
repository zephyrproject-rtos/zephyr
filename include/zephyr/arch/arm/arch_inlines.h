/*
 * Copyright 2022 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_ARCH_INLINES_H
#define ZEPHYR_INCLUDE_ARCH_ARM_ARCH_INLINES_H

#include <zephyr/kernel_structs.h>
#if defined(CONFIG_CPU_AARCH32_CORTEX_R) || defined(CONFIG_CPU_AARCH32_CORTEX_A)
#include <zephyr/arch/arm/cortex_a_r/lib_helpers.h>
#include <zephyr/arch/arm/cortex_a_r/tpidruro.h>

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
#ifdef CONFIG_VALIDATE_ARCH_CURR_CPU
	__ASSERT_NO_MSG(!z_smp_cpu_mobile());
#endif /* CONFIG_VALIDATE_ARCH_CURR_CPU */

#ifdef CONFIG_SMP
	return (_cpu_t *)(read_tpidruro() & TPIDRURO_CURR_CPU);
#else
	return &_kernel.cpus[0];
#endif /* CONFIG_SMP */
}
#else

#ifndef CONFIG_SMP
static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
	/* Dummy implementation always return the first cpu */
	return &_kernel.cpus[0];
}
#endif
#endif

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

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_ARCH_INLINES_H */
