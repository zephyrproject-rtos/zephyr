/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RH850 inline arch helpers
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

#include <zephyr/kernel_structs.h>

/** @cond INTERNAL_HIDDEN */

static ALWAYS_INLINE uint32_t arch_proc_id(void)
{
	return 0U;
}

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
	return &_kernel.cpus[0];
}

static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return CONFIG_MP_MAX_NUM_CPUS;
}

/** @endcond */

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_RH850_ARCH_INLINES_H_ */
