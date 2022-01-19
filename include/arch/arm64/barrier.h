/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_BARRIER_H_

#ifndef _ASMLANGUAGE

#if !defined(isb)
#define isb()		__asm__ volatile ("isb")
#endif

#if !defined(mb)
#define mb()		__asm__ volatile ("dsb sy" : : : "memory")
#endif

#if !defined(rmb)
#define rmb()		__asm__ volatile ("dsb ld" : : : "memory")
#endif

#if !defined(wmb)
#define wmb()		__asm__ volatile ("dsb st" : : : "memory")
#endif

#if !defined(smp_mb)
#define smp_mb()	__asm__ volatile ("dmb ish" : : : "memory")
#endif

#if !defined(smp_rmb)
#define smp_rmb()	__asm__ volatile ("dmb ishld" : : : "memory")
#endif

#if !defined(smp_wmb)
#define smp_wmb()	__asm__ volatile ("dmb ishst" : : : "memory")
#endif

#include <barrier.h>

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_BARRIER_H_ */
