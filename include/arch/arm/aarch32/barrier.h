/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_

#ifndef _ASMLANGUAGE

/*
 * Fallback on CMSIS
 */
#if defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#elif defined(CONFIG_CPU_CORTEX_R) || defined(CONFIG_CPU_AARCH32_CORTEX_A)
#include <arch/arm/aarch32/cortex_a_r/cmsis.h>
#endif

#if !defined(isb)
#define isb()		__ISB()
#endif

#if !defined(mb)
#define mb()		__DSB()
#endif

#if !defined(rmb)
#define rmb()		__DSB()
#endif

#if !defined(wmb)
#define wmb()		__DSB()
#endif

#if !defined(smp_mb)
#define smp_mb()	__DMB()
#endif

#if !defined(smp_rmb)
#define smp_rmb()	__DMB()
#endif

#if !defined(smp_wmb)
#define smp_wmb()	__DMB()
#endif

#include <barrier.h>

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_ */
