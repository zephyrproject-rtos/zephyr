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

#if defined(ARCH_HAS_MEMORY_BARRIER)

#error "Missing memory barrier definitions"

#elif defined(__GNUC__)

/**
 * @defgroup barrier_apis Barriers API
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief z_full_mb - Memory Barrier
 *
 * A full system memory barrier. All memory operations before the z_full_mb()
 * in the instruction stream will be committed before any operations after the
 * z_full_mb() are committed. This ordering will be visible to all bus masters
 * in the system. It will also ensure the order in which accesses from
 * a single processor reaches slave devices.
 */
#define z_full_mb()	__atomic_thread_fence(__ATOMIC_SEQ_CST)

/**
 * @brief z_read_mb - Read Memory Barrier
 *
 * Like z_full_mb(), but only guarantees ordering between read accesses. That
 * is, all read operations before an z_read_mb() will be committed before any
 * read operations after the z_read_mb().
 */
#define z_read_mb()	__atomic_thread_fence(__ATOMIC_SEQ_CST)

/**
 * @brief z_write_mb - Write Memory Barrier
 *
 * Like z_full_mb(), but only guarantees ordering between write accesses. That
 * is, all write operations before a z_write_mb() will be committed before any
 * write operations after the z_write_mb().
 */
#define z_write_mb()	__atomic_thread_fence(__ATOMIC_SEQ_CST)

/**
 * @}
 */

#else

#error "Missing memory barrier definitions"

#endif /* ARCH_HAS_MEMORY_BARRIER */

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_BARRIER_H_ */
