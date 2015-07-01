/* pi.c - pi computation portion of FPU sharing test */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module is used for the microkernel version of the FPU sharing test,
and supplements the basic load/store test by incorporating two additional
contexts that utilize the floating point unit.

Testing utilizes a pair of tasks that independently compute pi. The lower
priority task is regularly preempted by the higher priority task, thereby
testing whether floating point context information is properly preserved.

The following formula is used to compute pi:

    pi = 4 * (1 - 1/3 + 1/5 - 1/7 + 1/9 - ... )

This series converges to pi very slowly. For example, performing 50,000
iterations results in an accuracy of 3 decimal places.

A reference value of pi is computed once at the start of the test. All
subsequent computations must produce the same value, otherwise an error
has occurred.
 */

#ifdef CONFIG_MICROKERNEL
#include <microkernel.h>
#include <stdio.h>
#include <tc_util.h>

#include <float_context.h>

#define PI_NUM_ITERATIONS 700000

static double reference_pi = 0.0f;

/*
 * Test counters are "volatile" because GCC wasn't properly updating
 * calc_pi_low_count properly when calculate_pi_low() contained a "return"
 * in its error handling logic -- the value was incremented in a register,
 * but never written back to memory. (Seems to be a compiler bug!)
 */

static volatile unsigned int calc_pi_low_count = 0;
static volatile unsigned int calc_pi_high_count = 0;

/**
 *
 * calculate_pi_low - entry point for the low priority pi compute task
 *
 * RETURNS: N/A
 */

void calculate_pi_low(void)
{
	volatile double pi; /* volatile to avoid optimizing out of loop */
	double divisor = 3.0;
	double sign = -1.0;
	unsigned int ix;

	/* loop forever, unless an error is detected */

	while (1) {

		sign = -1.0;
		pi = 1.0;
		divisor = 3.0;

		for (ix = 0; ix < PI_NUM_ITERATIONS; ix++) {
			pi += sign / divisor;
			divisor += 2.0;
			sign *= -1.0;
		}

		pi *= 4;

		if (reference_pi == 0.0f) {
			reference_pi = pi;
		} else if (reference_pi != pi) {
			TC_ERROR("Computed pi %1.6f, reference pi %1.6f\n",
					 pi, reference_pi);
			fpu_sharing_error = 1;
			return;
		}

		++calc_pi_low_count;
	}
}

/**
 *
 * calculate_pi_high - entry point for the high priority pi compute task
 *
 * RETURNS: N/A
 */

void calculate_pi_high(void)
{
	volatile double pi; /* volatile to avoid optimizing out of loop */
	double divisor = 3.0;
	double sign = -1.0;
	unsigned int ix;

	/* loop forever, unless an error is detected */

	while (1) {

		sign = -1.0;
		pi = 1.0;
		divisor = 3.0;

		for (ix = 0; ix < PI_NUM_ITERATIONS; ix++) {
			pi += sign / divisor;
			divisor += 2.0;
			sign *= -1.0;
		}

		/*
		 * Relinquish the processor for the remainder of the current system
		 * clock tick, so that lower priority contexts get a chance to run.
		 *
		 * This exercises the ability of the nanokernel to restore the FPU
		 * state of a low priority context _and_ the ability of the nanokernel
		 * to provide a "clean" FPU state to this context once the sleep ends.
		 */

		task_sleep(1);

		pi *= 4;

		if (reference_pi == 0.0f) {
			reference_pi = pi;
		} else if (reference_pi != pi) {
			TC_ERROR("Computed pi %1.6f, reference pi %1.6f\n",
					 pi, reference_pi);
			fpu_sharing_error = 1;
			return;
		}

		/* periodically issue progress report */

		if ((++calc_pi_high_count % 100) == 50) {
			printf("Pi calculation OK after %u (high) + %u (low) tests "
				   "(computed %1.6f)\n",
				   calc_pi_high_count, calc_pi_low_count, pi);
		}
	}
}

#endif /* CONFIG_MICROKERNEL */
