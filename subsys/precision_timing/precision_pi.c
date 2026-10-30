/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/precision_timing/precision_pi.h>
#include <zephyr/sys/__assert.h>

void precision_pi_init(struct precision_pi *pi, double kp, double ki)
{
	pi->kp = kp;
	pi->ki = ki;
	pi->integral = 0.0;
	pi->integral_limit = 0.0;
	pi->output_limit = 0.0;
}

void precision_pi_set_limits(struct precision_pi *pi, double integral_limit, double output_limit)
{
	pi->integral_limit = integral_limit;
	pi->output_limit = output_limit;
}

void precision_pi_reset(struct precision_pi *pi)
{
	pi->integral = 0.0;
}

static double pi_clamp(double value, double limit)
{
	if (limit <= 0.0) {
		return value;
	}

	if (value > limit) {
		return limit;
	}

	if (value < -limit) {
		return -limit;
	}

	return value;
}

double precision_pi_update(struct precision_pi *pi, double error)
{
	return precision_pi_update_interval(pi, error, 1.0);
}

double precision_pi_update_interval(struct precision_pi *pi, double error, double interval)
{
	double integral;
	double output;
	double limited_output;
	bool integral_moves_outward;

	__ASSERT(interval > 0.0, "PI interval must be positive");

	integral = pi_clamp(pi->integral + (pi->ki / interval) * error, pi->integral_limit);
	output = (pi->kp / interval) * error + integral;
	limited_output = pi_clamp(output, pi->output_limit);

	if (limited_output != output) {
		/*
		 * Conditional integration: while the output is at its limit, the
		 * integral term keeps its value instead of moving towards the
		 * limit. It may still move back. Recomputing the integral from the
		 * limited output instead (back-calculation) would push it to the
		 * opposite sign when the proportional term alone exceeds the limit,
		 * and lose the estimate of the steady-state output.
		 */
		integral_moves_outward = ((integral - pi->integral) * limited_output) > 0.0;
		if (integral_moves_outward) {
			integral = pi->integral;
		}
	}

	pi->integral = integral;

	return limited_output;
}
