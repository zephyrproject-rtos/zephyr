/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_BARRIER_H_

#ifndef _ASMLANGUAGE

#define arch_isb()		__asm__ volatile ("isb")

#define arch_mb()		__asm__ volatile ("dsb sy" : : : "memory")

#define arch_rmb()		__asm__ volatile ("dsb ld" : : : "memory")

#define arch_wmb()		__asm__ volatile ("dsb st" : : : "memory")

#if defined(CONFIG_SMP)

#define arch_smp_mb()	__asm__ volatile ("dmb ish" : : : "memory")

#define arch_smp_rmb()	__asm__ volatile ("dmb ishld" : : : "memory")

#define arch_smp_wmb()	__asm__ volatile ("dmb ishst" : : : "memory")

#else /* !CONFIG_SMP */

#define arch_smp_mb()    arch_mb()

#define arch_smp_rmb()   arch_rmb()

#define arch_smp_wmb()   arch_wmb()

#endif /* CONFIG_SMP */

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_BARRIER_H_ */
