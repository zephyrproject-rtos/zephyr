/**
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BARRIER_ARM_H_
#define ZEPHYR_INCLUDE_BARRIER_ARM_H_

#ifndef ZEPHYR_INCLUDE_SYS_BARRIER_H_
#error Please include <zephyr/sys/barrier.h>
#endif

#include <cmsis_core.h>

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE void z_barrier_dmem_fence_full(void)
{
	__DMB();
}

static ALWAYS_INLINE void z_barrier_dsync_fence_full(void)
{
	__DSB();
}

static ALWAYS_INLINE void z_barrier_isync_fence_full(void)
{
	__ISB();
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BARRIER_ARM_H_ */
