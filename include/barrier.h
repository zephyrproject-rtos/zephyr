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

/**
 * @defgroup barrier_apis Barriers API
 * @ingroup kernel_apis
 * @{
 */

#if !defined(isb) || defined(__DOXYGEN__)
/**
 * @brief isb - Instruction Synchronization Barrier
 *
 * This is used to guarantee that any subsequent instructions are fetched, so
 * that privilege and access are checked with the current MMU configuration.
 */
#define isb()
#endif /* !isb */

#if !defined(mb) || defined(__DOXYGEN__)
/**
 * @brief mb - Memory Barrier
 *
 * A full system memory barrier. All memory operations before the mb() in the
 * instruction stream will be committed before any operations after the mb()
 * are committed.
 */
#define mb()		compiler_barrier()
#endif /* !mb */

#if !defined(rmb) || defined(__DOXYGEN__)
/**
 * @brief rmb - Read Memory Barrier
 *
 * Like mb(), but only guarantees ordering between read accesses. That is, all
 * read operations before an rmb() will be committed before any read operations
 * after the rmb().
 */
#define rmb()		mb()
#endif /* !rmb */

#if !defined(wmb) || defined(__DOXYGEN__)
/**
 * @brief wmb - Write Memory Barrier
 *
 * Like mb(), but only guarantees ordering between write accesses. That is, all
 * write operations before a wmb() will be committed before any write
 * operations after the wmb().
 */
#define wmb()		mb()
#endif /* !wmb */

#if defined(CONFIG_SMP) || defined(__DOXYGEN__)

#if !defined(smp_mb) || defined(__DOXYGEN__)
/**
 * @brief smp_mb - SMP Memory Barrier
 *
 * Similar to mb(), but only guarantees ordering between cores/processors
 * within an SMP system. All memory accesses before the smp_mb() will be
 * visible to all cores within the SMP system before any accesses after the
 * smp_mb().
 */
#define smp_mb()	mb()
#endif /* !smp_mb */

#if !defined(smp_rmb) || defined(__DOXYGEN__)
/**
 * @brief smp_rmb - SMP Read Memory Barrier
 *
 * Like smp_mb(), but only guarantees ordering between read accesses.
 */
#define smp_rmb()	rmb()
#endif /* !smp_rmb */

#if !defined(smp_wmb) || defined(__DOXYGEN__)
/**
 * @brief smp_wmb - SMP Write Memory Barrier
 *
 * Like smp_mb(), but only guarantees ordering between write accesses.
 */
#define smp_wmb()	wmb()
#endif /* !smp_wmb */

#else /* !CONFIG_SMP */

#if !defined(smp_mb)
#define smp_mb()	compiler_barrier()
#endif /* !smp_mb */

#if !defined(smp_rmb)
#define smp_rmb()	compiler_barrier()
#endif /* !smp_rmb */

#if !defined(smp_wmb)
#define smp_wmb()	compiler_barrier()
#endif /* !smp_wmb */

#endif /* CONFIG_SMP */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_BARRIER_H_ */
