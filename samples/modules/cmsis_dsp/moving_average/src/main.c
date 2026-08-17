/*
 * Copyright (c) 2024 Benjamin Cabé <benjamin@zephyrproject.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

#include "arm_math.h"

#define NUM_TAPS   10U
#define BLOCK_SIZE 32U

#define NUM_TAPS_PADDED   ROUND_UP(NUM_TAPS, 4)
#define BLOCK_SIZE_PADDED ROUND_UP(BLOCK_SIZE, 4)

/*
 * The CMSIS-DSP MVE Q31 FIR implementation offsets pState by
 * 2 * round_up(blockSize, 4), then uses a normal FIR state area after that.
 */
#define FIR_STATE_SIZE \
	((2U * BLOCK_SIZE_PADDED) + BLOCK_SIZE_PADDED + NUM_TAPS - 1U)

/*
 * Filter coefficients are all equal for a moving average filter.
 * 1 / NUM_TAPS = 0.1 in Q31 format.
 *
 * The MVE implementation for 9-12 taps loads coefficients in 4-wide vectors,
 * so the coefficient array must be padded to a multiple of 4.
 */
static q31_t firCoeffs[NUM_TAPS_PADDED] __aligned(16) = {
	0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD,
	0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD,
	0x0CCCCCCD, 0x0CCCCCCD,
};

static arm_fir_instance_q31 sFIR;

static q31_t firState[FIR_STATE_SIZE] __aligned(16);
static q31_t input[BLOCK_SIZE_PADDED] __aligned(16);
static q31_t output[BLOCK_SIZE_PADDED] __aligned(16);

int main(void)
{
	uint32_t start, end;

	for (int i = 0; i < FIR_STATE_SIZE; i++) {
		firState[i] = 0;
	}

	for (int i = 0; i < BLOCK_SIZE_PADDED; i++) {
		input[i] = 0;
		output[i] = 0;
	}

	/* Initialize input data with a ramp from 0 to 31 */
	for (int i = 0; i < BLOCK_SIZE; i++) {
		input[i] = i << 24; /* Convert to Q31 format */
	}

	/* Initialize the FIR filter */
	arm_fir_init_q31(&sFIR, NUM_TAPS, firCoeffs, firState, BLOCK_SIZE);

	/* Apply the FIR filter to the input data and measure cycles */
	start = k_cycle_get_32();
	arm_fir_q31(&sFIR, input, output, BLOCK_SIZE);
	end = k_cycle_get_32();

	printf("Time: %u us (%u cycles)\n",
	       k_cyc_to_us_floor32(end - start), end - start);

	for (int i = 0; i < BLOCK_SIZE; i++) {
		printf("Input[%02d]: ", i);

		for (int j = NUM_TAPS - 1; j >= 0; j--) {
			if (j <= i) {
				printf("%2d ", (int)(input[i - j] >> 24));
			} else {
				printf("%2d ", 0);
			}
		}

		printf("| Output[%02d]: %6.2f\n",
		       i, (double)output[i] / (1 << 24));
	}

	return 0;
}
