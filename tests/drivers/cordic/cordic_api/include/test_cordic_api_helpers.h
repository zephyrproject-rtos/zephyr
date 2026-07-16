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

#define PI_F32 (3.14159265358979323846f)  // TODO: shall be replaced by that one from math library

#define F32_TO_Q31(v) zdsp_f32_to_q31_shift(v, 0)
#define Q31_TO_F32(v) zdsp_q31_to_f32_shift(v, 0)

#define F32_TO_Q15(v) zdsp_f32_to_q15_shift(v, 0)

#define F32_TO_Q15(v) zdsp_f32_to_q15_shift(v, 0)
#define Q15_TO_F32(v) zdsp_q15_to_f32_shift(v, 0)

#define cordic_float_close(v1, v2, err) \
	fabs((double)v1 - (double)v2) <= ((double)err)
