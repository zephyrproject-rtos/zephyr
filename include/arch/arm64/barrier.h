/*
 * Copyright (c) 2022 Alexandre Mergnat <amergnat@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_BARRIER_H_

#ifndef _ASMLANGUAGE

#ifndef __ISB
#define __ISB()	__asm__ volatile ("isb")
#endif

#ifndef __DSB
#define __DSB()	__asm__ volatile ("dsb sy" : : : "memory")
#endif

#ifndef __DMB
#define __DMB()	__asm__ volatile ("dmb sy" : : : "memory")
#endif

// #if defined(CONFIG_ARCH_HAS_MEMORY_BARRIER)

// #define arch_mb()	__DSB()
// #define arch_rmb()	__asm__ volatile ("dsb ld" : : : "memory")
// #define arch_wmb()	__asm__ volatile ("dsb st" : : : "memory")

// #endif /* CONFIG_ARCH_HAS_MEMORY_BARRIER */

#define z_memory_barrier()	__DSB()
#define z_read_mb()	__asm__ volatile ("dsb ld" : : : "memory")
#define z_write_mb()	__asm__ volatile ("dsb st" : : : "memory")

// #include <sys/barrier.h>

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_BARRIER_H_ */
