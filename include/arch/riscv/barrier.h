/*
 * Copyright (c) 2022 Alexandre Mergnat <amergnat@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_BARRIER_H_

#ifndef _ASMLANGUAGE

#if defined(CONFIG_ARCH_HAS_MEMORY_BARRIER)

#define arch_mb()	__asm__ volatile ("fence iorw, iorw" : : : "memory")
#define arch_rmb()	__asm__ volatile ("fence ir, ir" : : : "memory")
#define arch_wmb()	__asm__ volatile ("fence ow, ow" : : : "memory")

#endif /* !CONFIG_ARCH_HAS_MEMORY_BARRIER */

#include <sys/barrier.h>

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_BARRIER_H_ */
