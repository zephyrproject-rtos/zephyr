/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_

#ifndef _ASMLANGUAGE

/* Fallback on CMSIS */
#if defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#elif defined(CONFIG_CPU_CORTEX_R) || defined(CONFIG_CPU_AARCH32_CORTEX_A)
#include <arch/arm/aarch32/cortex_a_r/cmsis.h>
#endif /* CONFIG_CPU_CORTEX_M | CONFIG_CPU_CORTEX_R | CONFIG_CPU_CORTEX_A */


#if defined(__BUILTIN_ISB)
#define arch_isb() __BUILTIN_ISB()
#else
#define arch_isb() __ISB()
#endif /* __BUILTIN_ISB */

#if defined(__BUILTIN_DSB)
#define arch_mb()		__BUILTIN_DSB()
#define arch_rmb()		__BUILTIN_DSB()
#define arch_wmb()		__BUILTIN_DSB()
#else
#define arch_mb()		__DSB()
#define arch_rmb()		__DSB()
#define arch_wmb()		__DSB()
#endif /* __BUILTIN_DSB */

#if defined(CONFIG_SMP)

#if defined(__BUILTIN_DMB)
#define arch_smp_mb()	__BUILTIN_DMB()
#define arch_smp_rmb()	__BUILTIN_DMB()
#define arch_smp_wmb()	__BUILTIN_DMB()
#else
#define arch_smp_mb()	__DMB()
#define arch_smp_rmb()	__DMB()
#define arch_smp_wmb()	__DMB()
#endif /* __BUILTIN_DMB */


#else /* !CONFIG_SMP */

#define arch_smp_mb()    arch_mb()
#define arch_smp_rmb()   arch_rmb()
#define arch_smp_wmb()   arch_wmb()

#endif /* CONFIG_SMP */


#undef tmp_isb
#undef tmp_dsb
#undef tmp_dmb

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_ */
