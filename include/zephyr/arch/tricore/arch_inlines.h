/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TriCore inline arch helpers
 */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_ARCH_INLINES_H_

#include <zephyr/arch/tricore/cr.h>

#ifndef _ASMLANGUAGE

#include <zephyr/kernel_structs.h>

/** @cond INTERNAL_HIDDEN */

static ALWAYS_INLINE uint32_t arch_proc_id(void)
{
	return 0;
}

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
#if defined(CONFIG_SMP)
	return &_kernel.cpus[arch_proc_id()];
#else
	return &_kernel.cpus[0];
#endif
}

static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return CONFIG_MP_MAX_NUM_CPUS;
}

static ALWAYS_INLINE uint32_t timestamp_serialize(void)
{
	return 0;
}

/** @endcond */

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_TRICORE_ARCH_INLINES_H_ */
