/*
 * Copyright (c) 2023 Florian Grandel, Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief nanosecond resolution network time implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_time.h>

void net_time_timer_expiry_fn(struct k_timer *timer)
{
	struct net_time_timer *net_time_timer = (struct net_time_timer *)timer;
	const struct net_time_reference_api *time_reference_api;

	if (net_time_timer->period_ns == 0) {
		return;
	}

	time_reference_api = net_time_timer->time_reference_api;

	K_SPINLOCK(&timer->timeout_api->state->timer_lock)
	{
		k_timepoint_t next_expiry_tp;

		net_time_timer->current_expiry_ns += net_time_timer->period_ns;

		time_reference_api->get_timepoint_from_time(
			time_reference_api, net_time_timer->rounding,
			net_time_timer->current_expiry_ns, &next_expiry_tp);

		timer->period = K_TICKS(next_expiry_tp.tick - timer->timeout_api->elapsed());
	}

	if (net_time_timer->expiry_fn) {
		net_time_timer->expiry_fn(net_time_timer);
	}
}
