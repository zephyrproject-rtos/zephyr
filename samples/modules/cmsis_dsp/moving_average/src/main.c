#include "arm_math.h"

#include <stdio.h>
#include <zephyr/kernel.h>


#define NUM_TAPS 10   /* Number of taps in the FIR filter (moving average window length) */
#define BLOCK_SIZE 32 /* Number of samples processed per block */

/*
 * Filter coefficients are all equal for a moving average filter.
 * Here, 1/NUM_TAPS = 0.1f.
 */
q31_t firCoeffs[NUM_TAPS] = {0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD,
			     0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD, 0x0CCCCCCD};

arm_fir_instance_q31 sFIR;

// State buffer provided to the FIR module
q31_t firState[NUM_TAPS + BLOCK_SIZE - 1];

int main(void) {
	// Input and output data arrays
	q31_t input[BLOCK_SIZE];
	q31_t output[BLOCK_SIZE];

	// Initialize input data with some values (example)
	for (int i = 0; i < BLOCK_SIZE; i++) {
		input[i] = i << 24; // Convert to Q31 format
	}

	// Initialize the FIR filter
	arm_fir_init_q31(&sFIR, NUM_TAPS, firCoeffs, firState, BLOCK_SIZE);

	// Apply the FIR filter to the input data
	// measure how many cycles this takes
	uint64_t start = k_cycle_get_64();
	arm_fir_q31(&sFIR, input, output, BLOCK_SIZE);
	uint64_t end = k_cycle_get_64();
	// print the number of cycles it took to run the FIR filter, as well as actual time in us
	printf("Cycles: %llu\n", end - start);
	printf("Time: %llu us\n", (end - start) / (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / USEC_PER_SEC));

	// Output data contains the moving average of the input
	for (int i = 0; i < BLOCK_SIZE; i++) {
		// print the input "slice" and the output
		// needs to be padded with zeros at the beginning
		printf("Input[%02d]: ", i);
		for (int j = 0; j < NUM_TAPS; j++) {
			if (i - j >= 0) {
				printf("%2d ", (int)(input[i - j] >> 24)); // Convert back to integer format
			} else {
				printf("%2d ", 0);
			}
		}

		printf("Output[%02d]: %6.2f\n", i, (float32_t)output[i] / (1 << 24)); // Convert back to float format
	}

	return 0;
}
