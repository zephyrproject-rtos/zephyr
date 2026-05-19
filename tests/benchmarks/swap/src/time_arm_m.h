/* Copyright 2025 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */
#include <cmsis_core.h>

static ALWAYS_INLINE void time_setup(void)
{
	/* Disable SysTick interrupts so the timer driver doesn't
	 * interfere, we want the full 24 bit space.
	 */
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
	SysTick->LOAD = 0xffffff;
}

static ALWAYS_INLINE uint32_t time(void)
{
	return SysTick->VAL;
}

static ALWAYS_INLINE uint32_t time_delta(uint32_t t0, uint32_t t1)
{
	/* SysTick counts down, not up! */
	return (t0 - t1) & 0xffffff;
}
