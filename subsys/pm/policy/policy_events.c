/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/time_units.h>

/** Lock to synchronize access to the events list. */
static struct k_spinlock events_lock;
/** List of events. */
static sys_slist_t events_list;
/** Pointer to Next Event. */
struct pm_policy_event *next_event;

static void update_next_event(void)
{
	struct pm_policy_event *evt;

	next_event = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&events_list, evt, node) {
		if (next_event == NULL) {
			next_event = evt;
			continue;
		}

		if (next_event->uptime_ticks <= evt->uptime_ticks) {
			continue;
		}

		next_event = evt;
	}
}

int64_t pm_policy_next_event_ticks(void)
{
	int64_t ticks = -1;

	K_SPINLOCK(&events_lock) {
		if (next_event == NULL) {
			K_SPINLOCK_BREAK;
		}

		ticks = next_event->uptime_ticks - k_uptime_ticks();

		if (ticks < 0) {
			ticks = 0;
		}
	}

	return ticks;
}

void pm_policy_event_register(struct pm_policy_event *evt, int64_t uptime_ticks)
{
	K_SPINLOCK(&events_lock) {
		evt->uptime_ticks = uptime_ticks;
		sys_slist_append(&events_list, &evt->node);
		update_next_event();
	}
}

void pm_policy_event_update(struct pm_policy_event *evt, int64_t uptime_ticks)
{
	K_SPINLOCK(&events_lock) {
		evt->uptime_ticks = uptime_ticks;
		update_next_event();
	}
}

void pm_policy_event_unregister(struct pm_policy_event *evt)
{
	K_SPINLOCK(&events_lock) {
		(void)sys_slist_find_and_remove(&events_list, &evt->node);
		update_next_event();
	}
}
