/*
 * Copyright (c) 2024 Benjamin Cabé <benjamin@zephyrproject.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>

#include "arm_math.h"

#define NUM_TAPS   10 /* Number of taps in the FIR filter (length of the moving average window) */
#define BLOCK_SIZE 32 /* Number of samples processed per block */
#define ROUND_UP_4(x) (((x) + 3) & ~3)

#if defined(ARM_MATH_MVEI) && !defined(ARM_MATH_AUTOVECTORIZE)
#define NUM_TAPS_ARRAY ROUND_UP_4(NUM_TAPS)
#define FIR_STATE_SIZE (NUM_TAPS + BLOCK_SIZE - 1 + (2 * ROUND_UP_4(BLOCK_SIZE)))
#else
#define NUM_TAPS_ARRAY NUM_TAPS
#define FIR_STATE_SIZE (NUM_TAPS + BLOCK_SIZE - 1)
#endif

/*
 * Filter coefficients are all equal for a moving average filter. Here, 1/NUM_TAPS = 0.1f.
 * The Helium version requires the coefficient array to be padded to a multiple of 4.
 */
q31_t firCoeffs[NUM_TAPS_ARRAY] = {0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD,
				   0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD};

arm_fir_instance_q31 sFIR;
q31_t firState[FIR_STATE_SIZE];

int main(void)
{
	q31_t input[BLOCK_SIZE];
	q31_t output[BLOCK_SIZE];
	uint32_t start, end;

	/* Initialize input data with a ramp from 0 to 31 */
	for (int i = 0; i < BLOCK_SIZE; i++) {
		input[i] = i << 24; /* Convert to Q31 format */
	}

	/* Initialize the FIR filter */
	arm_fir_init_q31(&sFIR, NUM_TAPS, firCoeffs, firState, BLOCK_SIZE);

	/* Apply the FIR filter to the input data and measure how many cycles this takes */
	start = k_cycle_get_32();
	arm_fir_q31(&sFIR, input, output, BLOCK_SIZE);
	end = k_cycle_get_32();

	printf("Time: %u us (%u cycles)\n", k_cyc_to_us_floor32(end - start), end - start);

	for (int i = 0; i < BLOCK_SIZE; i++) {
		printf("Input[%02d]: ", i);
		for (int j = NUM_TAPS - 1; j >= 0; j--) {
			if (j <= i) {
				printf("%2d ", (int)(input[i - j] >> 24));
			} else {
				printf("%2d ", 0);
			}
		}
		printf("| Output[%02d]: %6.2f\n", i, (double)output[i] / (1 << 24));
	}

	return 0;
}
