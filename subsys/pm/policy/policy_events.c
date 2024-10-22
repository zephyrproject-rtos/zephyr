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

/** @brief Update next event. */
static void update_next_event(uint32_t cyc)
{
	int64_t new_next_event_cyc = -1;
	struct pm_policy_event *evt;

	/* unset the next event pointer */
	next_event = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&events_list, evt, node) {
		uint64_t cyc_evt = evt->value_cyc;

		/*
		 * cyc value is a 32-bit rolling counter:
		 *
		 * |---------------->-----------------------|
		 * 0               cyc                  UINT32_MAX
		 *
		 * Values from [0, cyc) are events happening later than
		 * [cyc, UINT32_MAX], so pad [0, cyc) with UINT32_MAX + 1 to do
		 * the comparison.
		 */
		if (cyc_evt < cyc) {
			cyc_evt += (uint64_t)UINT32_MAX + 1U;
		}

		if ((new_next_event_cyc < 0) || (cyc_evt < new_next_event_cyc)) {
			new_next_event_cyc = cyc_evt;
			next_event = evt;
		}
	}
}

int32_t pm_policy_next_event_ticks(void)
{
	int32_t cyc_evt = -1;

	if ((next_event) && (next_event->value_cyc > 0)) {
		cyc_evt = next_event->value_cyc - k_cycle_get_32();
		cyc_evt = MAX(0, cyc_evt);
		BUILD_ASSERT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC >= CONFIG_SYS_CLOCK_TICKS_PER_SEC,
			     "HW Cycles per sec should be greater that ticks per sec");
		return k_cyc_to_ticks_floor32(cyc_evt);
	}

	return -1;
}

void pm_policy_event_register(struct pm_policy_event *evt, uint32_t time_us)
{
	k_spinlock_key_t key = k_spin_lock(&events_lock);
	uint32_t cyc = k_cycle_get_32();

	evt->value_cyc = cyc + k_us_to_cyc_ceil32(time_us);
	sys_slist_append(&events_list, &evt->node);
	update_next_event(cyc);

	k_spin_unlock(&events_lock, key);
}

void pm_policy_event_update(struct pm_policy_event *evt, uint32_t cycle)
{
	k_spinlock_key_t key = k_spin_lock(&events_lock);

	evt->value_cyc = cycle;
	update_next_event(k_cycle_get_32());

	k_spin_unlock(&events_lock, key);
}

void pm_policy_event_unregister(struct pm_policy_event *evt)
{
	k_spinlock_key_t key = k_spin_lock(&events_lock);

	(void)sys_slist_find_and_remove(&events_list, &evt->node);
	update_next_event(k_cycle_get_32());

	k_spin_unlock(&events_lock, key);
}
