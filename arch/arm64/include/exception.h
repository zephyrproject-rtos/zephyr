/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Exception/interrupt context helpers for Cortex-A CPUs
 *
 * Exception/interrupt context helpers.
 */

#ifndef ZEPHYR_ARCH_ARM64_INCLUDE_EXCEPTION_H_
#define ZEPHYR_ARCH_ARM64_INCLUDE_EXCEPTION_H_

#include <zephyr/arch/cpu.h>

#ifdef _ASMLANGUAGE

/* nothing */

#else

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE bool arch_is_in_isr(void)
{
	uint32_t nested;
#ifdef CONFIG_SMP
	unsigned int key;

	key = arch_irq_lock();
#endif
	nested = arch_curr_cpu()->nested;
#ifdef CONFIG_SMP
	arch_irq_unlock(key);
#endif
	return nested != 0U;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARM64_INCLUDE_EXCEPTION_H_ */
