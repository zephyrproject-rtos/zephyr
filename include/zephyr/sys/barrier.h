/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_BARRIER_H_
#define ZEPHYR_INCLUDE_SYS_BARRIER_H_

#include <zephyr/toolchain.h>

#if defined(CONFIG_BARRIER_OPERATIONS_ARCH)
# if defined(CONFIG_ARM)
# include <zephyr/arch/arm/barrier.h>
# elif defined(CONFIG_ARM64)
# include <zephyr/arch/arm64/barrier.h>
# endif
#elif defined(CONFIG_BARRIER_OPERATIONS_BUILTIN)
#include <zephyr/sys/barrier_builtin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup barrier_apis Barrier Services APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Full/sequentially-consistent data memory barrier.
 *
 * This routine acts as a synchronization fence between threads and prevents
 * re-ordering of data accesses instructions across the barrier instruction.
 */
static ALWAYS_INLINE void barrier_dmem_fence_full(void)
{
#if defined(CONFIG_BARRIER_OPERATIONS_ARCH) || defined(CONFIG_BARRIER_OPERATIONS_BUILTIN)
	z_barrier_dmem_fence_full();
#endif
}

/**
 * @brief Full/sequentially-consistent data synchronization barrier.
 *
 * This routine acts as a synchronization fence between threads and prevents
 * re-ordering of data accesses instructions across the barrier instruction
 * like @ref barrier_dmem_fence_full(), but has the additional effect of
 * blocking execution of any further instructions, not just loads or stores, or
 * both, until synchronization is complete.
 *
 * @note When not supported by hardware or architecture, this instruction falls
 * back to a full/sequentially-consistent data memory barrier.
 */
static ALWAYS_INLINE void barrier_dsync_fence_full(void)
{
#if defined(CONFIG_BARRIER_OPERATIONS_ARCH) || defined(CONFIG_BARRIER_OPERATIONS_BUILTIN)
	z_barrier_dsync_fence_full();
#endif
}

/**
 * @brief Full/sequentially-consistent instruction synchronization barrier.
 *
 * This routine is used to guarantee that any subsequent instructions are
 * fetched and to ensure any previously executed context-changing operations,
 * such as writes to system control registers, have completed by the time the
 * routine completes. In hardware terms, this might mean that the instruction
 * pipeline is flushed, for example.
 *
 * @note When not supported by hardware or architecture, this instruction falls
 * back to a compiler barrier.
 */
static ALWAYS_INLINE void barrier_isync_fence_full(void)
{
#if defined(CONFIG_BARRIER_OPERATIONS_ARCH) || defined(CONFIG_BARRIER_OPERATIONS_BUILTIN)
	z_barrier_isync_fence_full();
#endif
}

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ATOMIC_H_ */
