/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_BARRIER_H_

#ifndef _ASMLANGUAGE

#define arch_isb()		__asm__ volatile ("fence.i")
#define arch_mb()		__asm__ volatile ("fence iorw, iorw" : : : "memory")
#define arch_rmb()		__asm__ volatile ("fence ir, ir" : : : "memory")
#define arch_wmb()		__asm__ volatile ("fence ow, ow" : : : "memory")
#define arch_smp_mb()	__asm__ volatile ("fence rw, rw" : : : "memory")
#define arch_smp_rmb()	__asm__ volatile ("fence r, r" : : : "memory")
#define arch_smp_wmb()	__asm__ volatile ("fence w, w" : : : "memory")

#include <barrier.h>

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_BARRIER_H_ */
