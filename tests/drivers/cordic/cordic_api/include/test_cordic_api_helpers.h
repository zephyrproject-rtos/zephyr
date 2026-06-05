/*
 * Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <zephyr/drivers/cordic.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <math.h>

#include <zephyr/dsp/types.h>
#include <zephyr/dsp/utils.h>

/* The tolerance is choosen intentionally low to keep all possible platforms pass.
 * If lower tolerance is needed a separate platform specific test shall be created.
 */
#define TOLERANCE_F32 (1e-3f)

#define PI_F32 (3.14159265358979323846f)

#define PI_Q31 zdsp_f32_to_q31_shift(PI_F32, 0)
#define PI_Q15 zdsp_f32_to_q15_shift(PI_F32, 0)

#define F32_TO_Q31(v) zdsp_f32_to_q31_shift(v, 0)
#define Q31_TO_F32(v) zdsp_q31_to_f32_shift(v, 0)

#define F16_TO_Q15(v) zdsp_f32_to_q15_shift(v, 0)
#define Q15_TO_F16(v) zdsp_q15_to_f32_shift(v, 0)

#define cordic_float_close(value, expected, tol) \
	((value) >= (expected) - (tol) && (value) <= (expected) + (tol))
