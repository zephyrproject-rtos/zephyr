/*
 * Copyright (c) 2014-2016 Wind River Systems, Inc.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

#include <zephyr/kernel_structs.h>

#include <zephyr/arch/arc/v2/aux_regs.h>

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
#ifdef CONFIG_VALIDATE_ARCH_CURR_CPU
	__ASSERT_NO_MSG(!z_smp_cpu_mobile());
#endif /* CONFIG_VALIDATE_ARCH_CURR_CPU */

#ifdef CONFIG_SMP
	uint32_t core;

	core = z_arc_v2_core_id();

	return &_kernel.cpus[core];
#else
	return &_kernel.cpus[0];
#endif /* CONFIG_SMP */
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

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_ */
