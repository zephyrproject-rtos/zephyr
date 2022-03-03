/*
 * Copyright (c) 2022 Alexandre Mergnat <amergnat@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public interface for barriers.
 */
#ifndef ZEPHYR_INCLUDE_BARRIER_H_
#define ZEPHYR_INCLUDE_BARRIER_H_

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

//  Some architectures haven't their own implementation 
// #if defined(CONFIG_ARCH_HAS_MEMORY_BARRIER)

// #define arch(name)		arch_##name

#if defined(CONFIG_ARM)
#include <arch/arm/aarch32/barrier.h>
#elif defined(CONFIG_ARM64)
#include <arch/arm64/barrier.h>
#elif defined(CONFIG_RISCV)
#include <arch/riscv/barrier.h>
#elif defined(__GNUC__)

/**
 * @defgroup barrier_apis Barriers API
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief z_full_mb - Memory Barrier
 *
 * A full system memory barrier. All memory operations before the mb() in the
 * instruction stream will be committed before any operations after the mb()
 * are committed.
 */
#define z_full_mb()	__atomic_thread_fence(__ATOMIC_ACQ_REL)

/**
 * @brief z_read_mb - Read Memory Barrier
 *
 * Like z_full_mb(), but only guarantees ordering between read accesses. That is,
 * all read operations before an rmb() will be committed before any read
 * operations after the rmb().
 */
#define z_read_mb()	__atomic_thread_fence(__ATOMIC_ACQUIRE)

/**
 * @brief z_write_mb - Write Memory Barrier
 *
 * Like z_full_mb(), but only guarantees ordering between write accesses. That is,
 * all write operations before a wmb() will be committed before any write
 * operations after the wmb().
 */
#define z_write_mb()	__atomic_thread_fence(__ATOMIC_RELEASE)

#else

#error "Missing memory barrier definitions"

#endif /* CONFIG_ARM */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_BARRIER_H_ */
