/*
 * Copyright (c) 2026 Contributors to the Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/pid.h>
#include <zephyr/sys/util.h>

void pid_init(struct pid_state *state)
{
	state->integral = 0;
	state->prev_error = 0;
}

void pid_reset(struct pid_state *state)
{
	pid_init(state);
}

int32_t pid_update(const struct pid_cfg *cfg, struct pid_state *state,
		   int32_t setpoint, int32_t measured)
{
	int32_t error = setpoint - measured;

	/* Proportional term */
	int64_t p_term = (int64_t)cfg->kp * error;

	/* Integral term with anti-windup clamping */
	state->integral += (int64_t)cfg->ki * error;
	state->integral = CLAMP(state->integral,
				(int64_t)cfg->integral_min << PID_Q16_SHIFT,
				(int64_t)cfg->integral_max << PID_Q16_SHIFT);

	/* Derivative term */
	int64_t d_term = (int64_t)cfg->kd * (error - state->prev_error);

	state->prev_error = error;

	/* Sum and convert from Q16.16 to integer */
	int64_t output = (p_term + state->integral + d_term) >> PID_Q16_SHIFT;

	return CLAMP((int32_t)output, cfg->output_min, cfg->output_max);
}
