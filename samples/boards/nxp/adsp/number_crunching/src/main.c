/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Number crunching sample:
 * calling functions from an out of tree static library
 */

#include <zephyr/kernel.h>
#include <stdio.h>

#include "math_ops.h"

int main(void)
{
	printk("\r\nNumber crunching example!\r\n\n");

	test_vec_sum_int16_op();

	test_power_int16_op();

	test_power_int32_op();

	test_fft_op();

	test_iir_op();

	test_fir_blms_op();

	return 0;
}
