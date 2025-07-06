/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/pm/policy.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/time_units.h>

/** Lock to synchronize access to the latency request list. */
static struct k_spinlock latency_lock;
/** List of maximum latency requests. */
static sys_slist_t latency_reqs;
/** Maximum CPU latency in us */
static int32_t max_latency_us = SYS_FOREVER_US;
/** Maximum CPU latency in cycles */
int32_t max_latency_cyc = -1;
/** List of latency change subscribers. */
static sys_slist_t latency_subs;

/** @brief Update maximum allowed latency. */
static void update_max_latency(void)
{
	int32_t new_max_latency_us = SYS_FOREVER_US;
	struct pm_policy_latency_request *req;

	SYS_SLIST_FOR_EACH_CONTAINER(&latency_reqs, req, node) {
		if ((new_max_latency_us == SYS_FOREVER_US) ||
		    ((int32_t)req->value_us < new_max_latency_us)) {
			new_max_latency_us = (int32_t)req->value_us;
		}
	}

	if (max_latency_us != new_max_latency_us) {
		struct pm_policy_latency_subscription *sreq;
		int32_t new_max_latency_cyc = -1;

		SYS_SLIST_FOR_EACH_CONTAINER(&latency_subs, sreq, node) {
			sreq->cb(new_max_latency_us);
		}

		if (new_max_latency_us != SYS_FOREVER_US) {
			new_max_latency_cyc = (int32_t)k_us_to_cyc_ceil32(new_max_latency_us);
		}

		max_latency_us = new_max_latency_us;
		max_latency_cyc = new_max_latency_cyc;
	}
}

void pm_policy_latency_request_add(struct pm_policy_latency_request *req,
				   uint32_t value_us)
{
	req->value_us = value_us;

	k_spinlock_key_t key = k_spin_lock(&latency_lock);

	sys_slist_append(&latency_reqs, &req->node);
	update_max_latency();

	k_spin_unlock(&latency_lock, key);
}

void pm_policy_latency_request_update(struct pm_policy_latency_request *req,
				      uint32_t value_us)
{
	k_spinlock_key_t key = k_spin_lock(&latency_lock);

	req->value_us = value_us;
	update_max_latency();

	k_spin_unlock(&latency_lock, key);
}

void pm_policy_latency_request_remove(struct pm_policy_latency_request *req)
{
	k_spinlock_key_t key = k_spin_lock(&latency_lock);

	(void)sys_slist_find_and_remove(&latency_reqs, &req->node);
	update_max_latency();

	k_spin_unlock(&latency_lock, key);
}

void pm_policy_latency_changed_subscribe(struct pm_policy_latency_subscription *req,
					 pm_policy_latency_changed_cb_t cb)
{
	k_spinlock_key_t key = k_spin_lock(&latency_lock);

	req->cb = cb;
	sys_slist_append(&latency_subs, &req->node);

	k_spin_unlock(&latency_lock, key);
}

void pm_policy_latency_changed_unsubscribe(struct pm_policy_latency_subscription *req)
{
	k_spinlock_key_t key = k_spin_lock(&latency_lock);

	(void)sys_slist_find_and_remove(&latency_subs, &req->node);

	k_spin_unlock(&latency_lock, key);
}
