/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Memory barrier operations.
 * @ingroup barrier_apis
 */

#ifndef ZEPHYR_INCLUDE_SYS_BARRIER_H_
#define ZEPHYR_INCLUDE_SYS_BARRIER_H_

#include <zephyr/toolchain.h>

#if defined(CONFIG_BARRIER_OPERATIONS_ARCH)
# if defined(CONFIG_ARM)
# include <zephyr/arch/arm/barrier.h>
# elif defined(CONFIG_ARM64)
# include <zephyr/arch/arm64/barrier.h>
# elif defined(CONFIG_ARC)
# include <zephyr/arch/arc/barrier.h>
# endif
#elif defined(CONFIG_BARRIER_OPERATIONS_BUILTIN)
#include <zephyr/sys/barrier_builtin.h>
#else
/* No backend is available for this architecture and toolchain combination, so
 * fall back to a compiler barrier. That keeps the compiler from moving
 * accesses across the barrier, but it emits no instruction, so nothing
 * constrains the hardware. Architectures where the ordering matters must
 * select BARRIER_OPERATIONS_BUILTIN or BARRIER_OPERATIONS_ARCH; the barrier
 * API must never expand to nothing at all.
 */

static ALWAYS_INLINE void z_barrier_sync_synchronize(void)
{
	compiler_barrier();
}

static ALWAYS_INLINE void z_barrier_dmem_fence_full(void)
{
	compiler_barrier();
}

static ALWAYS_INLINE void z_barrier_dsync_fence_full(void)
{
	compiler_barrier();
}

static ALWAYS_INLINE void z_barrier_isync_fence_full(void)
{
	compiler_barrier();
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup barrier_apis Barrier Services APIs
 * @since 3.4
 * @version 0.1.0
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Full/sequentially-consistent data memory barrier across cores.
 *
 * This routine acts as a synchronization fence between threads/cores and
 * prevents re-ordering of data accesses instructions across the barrier
 * instruction.
 *
 * @note When no barrier backend is available for the architecture and
 * toolchain in use, this falls back to a compiler barrier, which orders the
 * accesses the compiler emits but places no constraint on the hardware.
 */
static ALWAYS_INLINE void barrier_sync_synchronize(void)
{
	z_barrier_sync_synchronize();
}

/**
 * @brief Full/sequentially-consistent data memory barrier.
 *
 * This routine acts as a synchronization fence between threads and prevents
 * re-ordering of data accesses instructions across the barrier instruction.
 *
 * @note When no barrier backend is available for the architecture and
 * toolchain in use, this falls back to a compiler barrier, which orders the
 * accesses the compiler emits but places no constraint on the hardware.
 */
static ALWAYS_INLINE void barrier_dmem_fence_full(void)
{
	z_barrier_dmem_fence_full();
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
 * back to a full/sequentially-consistent data memory barrier, and to a
 * compiler barrier when no backend is available at all.
 */
static ALWAYS_INLINE void barrier_dsync_fence_full(void)
{
	z_barrier_dsync_fence_full();
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
	z_barrier_isync_fence_full();
}

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ATOMIC_H_ */
