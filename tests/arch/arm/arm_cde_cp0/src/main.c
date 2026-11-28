/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <arm_cde.h>

/*
 * Build-only check that CONFIG_ARMV8_M_CDE_CP0 makes the compiler accept CDE
 * instructions on coprocessor 0. The code is never run, since whether CP0 is
 * implemented and enabled depends on the SoC.
 */

#ifndef __ARM_FEATURE_CDE
#error "CDE is not enabled by the compiler flags"
#endif

#if !(__ARM_FEATURE_CDE_COPROC & BIT(0))
#error "CDE coprocessor 0 is not enabled by the compiler flags"
#endif

int main(void)
{
	uint32_t val = __arm_cx1(0, 0);

	val = __arm_cx2(0, val, 0);

	return (int)val;
}
