/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
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
	uint32_t id = cr_read(TRICORE_CORE_ID) & 0x7U;

	/* TC1.6.2 devices number CPU5 as CORE_ID 6; CORE_ID 5 is not used */
	if (IS_ENABLED(CONFIG_CPU_TC162) && (id == 6U)) {
		id = 5U;
	}

	return id;
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

/** @endcond */

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_TRICORE_ARCH_INLINES_H_ */
