/*
 * Copyright (c) 2022 Alexandre Mergnat <amergnat@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_

#ifndef _ASMLANGUAGE

/*
 * Fallback on CMSIS when possible
 */
#include <cmsis_compiler.h>

/*
 * Provide a default implementation for un-supported compilers.
 */

#ifndef __ISB
#define __ISB()	__asm__ volatile ("isb 0xF" : : : "memory")
#endif

#ifndef __DSB
#define __DSB()	__asm__ volatile ("dsb 0xF" : : : "memory")
#endif

#ifndef __DMB
#define __DMB()	__asm__ volatile ("dmb 0xF" : : : "memory")
#endif

// #if defined(CONFIG_ARCH_HAS_MEMORY_BARRIER)

#define z_memory_barrier()	__DSB()
#define z_read_mb()	__DSB()
#define z_write_mb()	__DSB()

// #endif /* CONFIG_ARCH_HAS_MEMORY_BARRIER */

// #include <sys/barrier.h>

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_ */
