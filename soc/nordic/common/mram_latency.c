/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mram_latency.h>
#include <zephyr/kernel.h>
#include <nrfs_mram.h>
#include <nrfs_backend_ipc_service.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mram_latency, CONFIG_MRAM_LATENCY_LOG_LEVEL);

enum mram_latency_state {
	MRAM_LATENCY_OFF = 0,
	MRAM_LATENCY_OFF_PENDING,
	MRAM_LATENCY_ON,
};

static struct k_work work;
static bool no_latency;
static enum mram_latency_state state;

static onoff_notify_fn onoff_notify;
struct onoff_manager mram_latency_mgr;

struct sync_latency_req {
	struct onoff_client cli;
	struct k_sem sem;
	int res;
};

static void latency_change_req(bool latency_not_allowed)
{
	nrfs_err_t err;

	if (latency_not_allowed) {
		err = nrfs_mram_set_latency(MRAM_LATENCY_NOT_ALLOWED, NULL);
		if (err != NRFS_SUCCESS) {
			onoff_notify(&mram_latency_mgr, -EIO);
		}
	} else {
		/* There is no event for that setting so we can notify onoff manager
		 * immediately.
		 */
		err = nrfs_mram_set_latency(MRAM_LATENCY_ALLOWED, NULL);
		onoff_notify(&mram_latency_mgr, err != NRFS_SUCCESS ? -EIO : 0);
	}
}

static void latency_change(bool latency_not_allowed)
{
	LOG_DBG("Request: latency %s allowed", latency_not_allowed ? "not " : "");
	if (state == MRAM_LATENCY_OFF) {
		state = MRAM_LATENCY_OFF_PENDING;
	} else if (k_is_in_isr()) {
		/* nrfs cannot be called from interrupt context so defer to the work
		 * queue context and execute from there.
		 */
		no_latency = latency_not_allowed;
		k_work_submit(&work);
	} else {
		latency_change_req(latency_not_allowed);
	}
}

static void no_latency_start(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	onoff_notify = notify;
	latency_change(true);
}

static void no_latency_stop(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	latency_change(false);
}

static void evt_handler(nrfs_mram_latency_evt_t const *p_evt, void *context)
{
	int res = p_evt->type == NRFS_MRAM_LATENCY_REQ_APPLIED ? 0 : -EIO;

	LOG_DBG("Latency not allowed - applied");
	onoff_notify(&mram_latency_mgr, res);
}

static void work_handler(struct k_work *work)
{
	latency_change_req(no_latency);
}

int mram_no_latency_cancel_or_release(struct onoff_client *cli)
{
	return onoff_cancel_or_release(&mram_latency_mgr, cli);
}

int mram_no_latency_request(struct onoff_client *cli)
{
	return onoff_request(&mram_latency_mgr, cli);
}

static void sync_req_cb(struct onoff_manager *mgr, struct onoff_client *cli, uint32_t state,
			int res)
{
	struct sync_latency_req *req = CONTAINER_OF(cli, struct sync_latency_req, cli);

	req->res = res;
	k_sem_give(&req->sem);
}

int mram_no_latency_sync_request(void)
{
	struct sync_latency_req req;
	int rv;

	if (k_is_in_isr() || (state != MRAM_LATENCY_ON)) {
		return -ENOTSUP;
	}

	k_sem_init(&req.sem, 0, 1);
	sys_notify_init_callback(&req.cli.notify, sync_req_cb);
	rv = onoff_request(&mram_latency_mgr, &req.cli);
	if (rv < 0) {
		return rv;
	}

	rv = k_sem_take(&req.sem, K_MSEC(CONFIG_MRAM_LATENCY_SYNC_TIMEOUT));
	if (rv < 0) {
		return rv;
	}

	return req.res;
}

int mram_no_latency_sync_release(void)
{
	return onoff_release(&mram_latency_mgr) >= 0 ? 0 : -EIO;
}

/* First initialize onoff manager to be able to accept requests. */
static int init_manager(void)
{
	static const struct onoff_transitions transitions =
		ONOFF_TRANSITIONS_INITIALIZER(no_latency_start, no_latency_stop, NULL);

	return onoff_manager_init(&mram_latency_mgr, &transitions);
}

/* When kernel and IPC is running initialize nrfs. Optionally, execute pending request. */
static int init_nrfs(void)
{
	nrfs_err_t err;
	int rv;

	err = nrfs_backend_wait_for_connection(K_FOREVER);
	if (err != NRFS_SUCCESS) {
		return -EIO;
	}

	err = nrfs_mram_init(evt_handler);
	if (err != NRFS_SUCCESS) {
		return -EIO;
	}

	k_work_init(&work, work_handler);

	if (state == MRAM_LATENCY_OFF_PENDING) {
		latency_change(true);
	}

	state = MRAM_LATENCY_ON;

	return rv;
}

SYS_INIT(init_manager, PRE_KERNEL_1, 0);
SYS_INIT(init_nrfs, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
