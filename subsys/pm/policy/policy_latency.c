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
static uint32_t max_latency_us = (uint32_t)SYS_FOREVER_US;
/** Maximum CPU latency in cycles */
int32_t max_latency_cyc = -1;
/** Optional manager for applying immediate changes based on latency requests. */
struct pm_policy_latency_immediate_ctrl immediate_mgr;

/** @brief Update maximum allowed latency. */
static void update_max_latency(uint32_t prev, uint32_t value)
{
	uint32_t new_max_latency_us = SYS_FOREVER_US;
	struct pm_policy_latency_request *req;

	if (value < max_latency_us) {
		/* If new value is smaller than previous max it becomes the new max latency. */
		max_latency_us = value;
		max_latency_cyc = max_latency_us != SYS_FOREVER_US ?
				(int32_t)k_us_to_cyc_ceil32(max_latency_us) : -1;
		return;
	} else if (prev > max_latency_us) {
		/* If previous and new value is bigger than max there is no max latency change. */
		return;
	}

	/* Need to iterate over all existing requests to get the new max latency. */
	SYS_SLIST_FOR_EACH_CONTAINER(&latency_reqs, req, node) {
		if ((new_max_latency_us == SYS_FOREVER_US) ||
		    (req->value_us < new_max_latency_us)) {
			new_max_latency_us = req->value_us;
		}
	}

	if (max_latency_us != new_max_latency_us) {
		int32_t new_max_latency_cyc = -1;

		if (new_max_latency_us != SYS_FOREVER_US) {
			new_max_latency_cyc = (int32_t)k_us_to_cyc_ceil32(new_max_latency_us);
		}

		max_latency_us = new_max_latency_us;
		max_latency_cyc = new_max_latency_cyc;
	}
}

static void onoff_cb(struct onoff_manager *mgr, struct onoff_client *cli, uint32_t state, int res)
{
	struct pm_policy_latency_request *req =
		CONTAINER_OF(cli, struct pm_policy_latency_request, cli);
	pm_policy_latency_changed_cb_t cb = req->internal;

	cb(req, res);
}

static int onoff_req(struct pm_policy_latency_request *req, uint32_t prev)
{
	uint32_t thr = immediate_mgr.bin_mgr->thr;
	uint32_t notify_method = sys_notify_get_method(&req->cli.notify);

	if (req->value_us <= thr && prev > thr) {
		/* Replace user callback with internal callback because onoff manager
		 * callback signature does not match latency changed callback signature.
		 */
		if (notify_method == SYS_NOTIFY_METHOD_CALLBACK) {
			req->internal = req->cli.notify.method.callback;
			req->cli.notify.method.callback = onoff_cb;
		}

		return onoff_request(immediate_mgr.bin_mgr->mgr, &req->cli) == ONOFF_STATE_ON ?
			0 : 1;
	} else if (req->value_us > thr && prev <= thr) {
		return onoff_cancel_or_release(immediate_mgr.bin_mgr->mgr, &req->cli) ==
			ONOFF_STATE_ON ? 0 : 1;
	}

	if (notify_method != SYS_NOTIFY_METHOD_COMPLETED) {
		pm_policy_latency_changed_cb_t cb = sys_notify_finalize(&req->cli.notify, 0);

		if (cb) {
			cb(req, 0);
		}
	}

	return 0;
}

static int finalize(struct pm_policy_latency_request *req, uint32_t prev)
{
	if (IS_ENABLED(CONFIG_PM_POLICY_LATENCY_IMMEDIATE_BIN_ACTION) && immediate_mgr.mgr) {
		if (immediate_mgr.onoff) {
			return onoff_req(req, prev);
		} else {
			return -ENOTSUP;
		}
	}

	return 0;
}

int pm_policy_latency_request_add(struct pm_policy_latency_request *req, uint32_t value_us)
{
	sys_snode_t *node;
	k_spinlock_key_t key;

	key = k_spin_lock(&latency_lock);
	if (sys_slist_find(&latency_reqs, &req->node, &node)) {
		k_spin_unlock(&latency_lock, key);
		return -EALREADY;
	}
	req->value_us = value_us;
	sys_slist_append(&latency_reqs, &req->node);
	update_max_latency((uint32_t)SYS_FOREVER_US, value_us);

	k_spin_unlock(&latency_lock, key);

	return finalize(req, (uint32_t)SYS_FOREVER_US);
}

typedef int (*latency_func_t)(struct pm_policy_latency_request *req, uint32_t value_us);

static int sync_req(latency_func_t func, struct pm_policy_latency_request *req, uint32_t value_us)
{
	if (!IS_ENABLED(CONFIG_PM_POLICY_LATENCY_IMMEDIATE_BIN_ACTION) ||
	    immediate_mgr.mgr == NULL) {
		return func(req, value_us);
	}

	struct k_poll_signal sig;
	struct k_poll_event evt = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
							    K_POLL_MODE_NOTIFY_ONLY,
							    &sig);
	int rv;

	k_poll_signal_init(&sig);
	sys_notify_init_signal(&req->cli.notify, &sig);

	rv = func(req, value_us);
	if (rv >= 0) {
		rv = k_poll(&evt, 1, K_FOREVER);
	}

	return rv;
}

int pm_policy_latency_request_add_sync(struct pm_policy_latency_request *req, uint32_t value_us)
{
	return sync_req(pm_policy_latency_request_add, req, value_us);
}

int pm_policy_latency_request_update(struct pm_policy_latency_request *req, uint32_t value_us)
{
	sys_snode_t *node;
	uint32_t prev = req->value_us;

	k_spinlock_key_t key = k_spin_lock(&latency_lock);

	if (!sys_slist_find(&latency_reqs, &req->node, &node)) {
		k_spin_unlock(&latency_lock, key);
		return -EAGAIN;
	}

	req->value_us = value_us;
	update_max_latency(prev, value_us);

	k_spin_unlock(&latency_lock, key);

	return finalize(req, prev);
}

int pm_policy_latency_request_update_sync(struct pm_policy_latency_request *req, uint32_t value_us)
{
	return sync_req(pm_policy_latency_request_update, req, value_us);
}

int pm_policy_latency_request_remove(struct pm_policy_latency_request *req)
{
	sys_snode_t *node;
	uint32_t prev = req->value_us;

	k_spinlock_key_t key = k_spin_lock(&latency_lock);

	if (!sys_slist_find(&latency_reqs, &req->node, &node)) {
		k_spin_unlock(&latency_lock, key);
		return -EALREADY;
	}
	req->value_us = (uint32_t)SYS_FOREVER_US;
	(void)sys_slist_find_and_remove(&latency_reqs, &req->node);
	update_max_latency(prev, req->value_us);

	k_spin_unlock(&latency_lock, key);

	return finalize(req, prev);
}

int pm_policy_latency_immediate_ctrl_add(struct pm_policy_latency_immediate_ctrl *mgr)
{
	if (mgr && !mgr->onoff) {
		return -ENOTSUP;
	} else if (mgr == NULL) {
		immediate_mgr.mgr = NULL;
	} else {
		immediate_mgr = *mgr;
	}

	return 0;
}
