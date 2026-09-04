/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/precision_timing/precision_pi.h>

void precision_pi_init(struct precision_pi *pi, double kp, double ki)
{
	pi->kp = kp;
	pi->ki = ki;
	pi->integral = 0.0;
}

void precision_pi_reset(struct precision_pi *pi)
{
	pi->integral = 0.0;
}

double precision_pi_update(struct precision_pi *pi, double error)
{
	pi->integral += pi->ki * error;

	return pi->kp * error + pi->integral;
}
