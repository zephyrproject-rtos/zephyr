/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/timing/precision_timing.h>

void precision_deadline_cancel(struct precision_deadline *deadline)
{
	if (deadline == NULL) {
		return;
	}

	deadline->expiry_ms = 0;
	deadline->scheduled = false;
}

void precision_deadline_schedule(struct precision_deadline *deadline, precision_time_t delay_ns)
{
	uint64_t delay_ms;
	int64_t now_ms;

	if (deadline == NULL) {
		return;
	}

	if (delay_ns <= 0) {
		precision_deadline_cancel(deadline);
		return;
	}

	/* Round up so that the deadline never expires before the requested delay. */
	delay_ms = (uint64_t)delay_ns / NSEC_PER_MSEC;
	if ((uint64_t)delay_ns % NSEC_PER_MSEC != 0U) {
		delay_ms++;
	}

	now_ms = k_uptime_get();
	if (delay_ms > (uint64_t)(INT64_MAX - now_ms)) {
		deadline->expiry_ms = INT64_MAX;
	} else {
		deadline->expiry_ms = now_ms + (int64_t)delay_ms;
	}

	deadline->scheduled = true;
}

bool precision_deadline_due(struct precision_deadline *deadline)
{
	if (deadline == NULL || !deadline->scheduled || k_uptime_get() < deadline->expiry_ms) {
		return false;
	}

	precision_deadline_cancel(deadline);

	return true;
}
