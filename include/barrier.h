/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
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

/* Some architectures haven't their own implementation */
#if !defined(CONFIG_ARCH_HAS_MEMORY_BARRIER) && !defined(__HAS_BUILTIN_MEMORY_BARRIER)

/**
 * @defgroup default barrier_apis Barriers API
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief isb - Instruction Synchronization Barrier
 *
 * This is used to guarantee that any subsequent instructions are fetched, so
 * that privilege and access are checked with the current MMU configuration.
 */
#define arch_isb()

/**
 * @brief mb - Memory Barrier
 *
 * A full system memory barrier. All memory operations before the mb() in the
 * instruction stream will be committed before any operations after the mb()
 * are committed.
 */
#define arch_mb()		compiler_barrier()

/**
 * @brief rmb - Read Memory Barrier
 *
 * Like mb(), but only guarantees ordering between read accesses. That is, all
 * read operations before an rmb() will be committed before any read operations
 * after the rmb().
 */
#define arch_rmb()		arch_mb()

/**
 * @brief wmb - Write Memory Barrier
 *
 * Like mb(), but only guarantees ordering between write accesses. That is, all
 * write operations before a wmb() will be committed before any write
 * operations after the wmb().
 */
#define arch_wmb()		arch_mb()

/**
 * @brief smp_mb - SMP Memory Barrier
 *
 * Similar to mb(), but only guarantees ordering between cores/processors
 * within an SMP system. All memory accesses before the smp_mb() will be
 * visible to all cores within the SMP system before any accesses after the
 * smp_mb().
 */
#define arch_smp_mb()	arch_mb()

/**
 * @brief smp_rmb - SMP Read Memory Barrier
 *
 * Like smp_mb(), but only guarantees ordering between read accesses.
 */
#define arch_smp_rmb()	arch_rmb()

/**
 * @brief smp_wmb - SMP Write Memory Barrier
 *
 * Like smp_mb(), but only guarantees ordering between write accesses.
 */
#define arch_smp_wmb()	arch_wmb()

#endif /* !CONFIG_ARCH_HAS_MEMORY_BARRIER */

#if !defined(CONFIG_SMP)
#undef  arch_smp_mb
#undef  arch_smp_rmb
#undef  arch_smp_wmb
#define arch_smp_mb()	arch_mb()
#define arch_smp_rmb()	arch_rmb()
#define arch_smp_wmb()	arch_wmb()
#endif /* !CONFIG_SMP */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_BARRIER_H_ */
