/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#include <mram_latency.h>

#define TIMEOUT_MS 10

static volatile uint32_t current_state;
static struct onoff_monitor monitor;
static struct onoff_client early_client;
static int early_rv;
static int early_result;

struct test_req {
	struct onoff_client cli;
	struct k_sem sem;
	int res;
	uint32_t state;
};

static void basic_cb(struct onoff_manager *mgr, struct onoff_client *cli, uint32_t state, int res)
{
	struct test_req *req = CONTAINER_OF(cli, struct test_req, cli);

	req->res = res;
	req->state = state;
	k_sem_give(&req->sem);
}

static void monitor_cb(struct onoff_manager *mgr, struct onoff_monitor *mon, uint32_t state,
		       int res)
{
	current_state = state;
}

ZTEST(mram_latency, test_basic_requests)
{
	struct test_req req1, req2;
	uint32_t exp_state;
	int rv;

	k_sem_init(&req1.sem, 0, 1);
	k_sem_init(&req2.sem, 0, 1);

	sys_notify_init_callback(&req1.cli.notify, basic_cb);
	exp_state = ONOFF_STATE_OFF;
	/* Req: 0->1 trigger to on */
	rv = mram_no_latency_request(&req1.cli);
	zassert_equal(rv, exp_state, "Unexpected rv:%d (exp:%d)", rv, exp_state);

	sys_notify_init_callback(&req2.cli.notify, basic_cb);
	exp_state = ONOFF_STATE_TO_ON;
	/* Req: 1->2 */
	rv = mram_no_latency_request(&req2.cli);
	zassert_equal(rv, exp_state, "Unexpected rv:%d (exp:%d)", rv, exp_state);

	rv = k_sem_take(&req1.sem, K_MSEC(TIMEOUT_MS));
	zassert_equal(rv, 0, "Unexpected rv:%d", rv);
	zassert_equal(req1.res, 0, "Unexpected res:%d", req1.res);
	zassert_equal(req1.state, ONOFF_STATE_ON, "Unexpected state:%08x", req1.state);

	rv = k_sem_take(&req2.sem, K_MSEC(TIMEOUT_MS));
	zassert_equal(rv, 0, "Unexpected rv:%d", rv);
	zassert_equal(req2.res, 0, "Unexpected res:%d", req2.res);
	zassert_equal(req2.state, ONOFF_STATE_ON);

	exp_state = ONOFF_STATE_ON;
	rv = mram_no_latency_cancel_or_release(&req2.cli);
	/* Req: 2->1 */
	zassert_equal(rv, exp_state, "Unexpected rv:%d (exp:%d)", rv, exp_state);

	/* Req: 1->0 going to off triggered*/
	rv = mram_no_latency_cancel_or_release(&req1.cli);
	zassert_equal(rv, exp_state, "Unexpected rv:%d (exp:%d)", rv, exp_state);

	sys_notify_init_callback(&req1.cli.notify, basic_cb);
	exp_state = ONOFF_STATE_OFF;

	/* Req: 0->1 triggered to on while in to off. */
	rv = mram_no_latency_request(&req1.cli);
	zassert_equal(rv, exp_state, "Unexpected rv:%d (exp:%d)", rv, exp_state);

	/* Req: 1->0 releases which to off. */
	exp_state = ONOFF_STATE_TO_ON;
	rv = mram_no_latency_cancel_or_release(&req1.cli);
	zassert_equal(rv, exp_state, "Unexpected rv:%d (exp:%d)", rv, exp_state);

	k_msleep(10);
}

static void timeout(struct k_timer *timer)
{
	struct test_req *req = k_timer_user_data_get(timer);
	uint32_t exp_state;
	int rv;

	sys_notify_init_callback(&req->cli.notify, basic_cb);
	exp_state = ONOFF_STATE_OFF;
	rv = mram_no_latency_request(&req->cli);
	zassert_equal(rv, exp_state, "Unexpected rv:%d (exp:%d)", rv, exp_state);
}

ZTEST(mram_latency, test_req_from_irq)
{
	struct test_req req;
	struct k_timer timer;
	uint32_t exp_state;
	int rv;

	k_sem_init(&req.sem, 0, 1);
	k_timer_init(&timer, timeout, NULL);
	k_timer_user_data_set(&timer, &req);
	/* Start k_timer and from that context request MRAM latency. */
	k_timer_start(&timer, K_MSEC(1), K_NO_WAIT);

	exp_state = ONOFF_STATE_ON;
	rv = k_sem_take(&req.sem, K_MSEC(TIMEOUT_MS));
	zassert_equal(rv, 0, "Unexpected rv:%d", rv);
	zassert_equal(req.res, 0, "Unexpected res:%d", req.res);
	zassert_equal(req.state, exp_state);

	rv = mram_no_latency_cancel_or_release(&req.cli);
	zassert_equal(rv, exp_state, "Unexpected rv:%d (exp:%d)", rv, exp_state);
}

ZTEST(mram_latency, test_sync_req)
{
	zassert_equal(current_state, ONOFF_STATE_OFF);
	mram_no_latency_sync_request();
	zassert_equal(current_state, ONOFF_STATE_ON);
	mram_no_latency_sync_release();
	zassert_equal(current_state, ONOFF_STATE_OFF);
}

ZTEST(mram_latency, test_early_req)
{
	zassert_true(early_rv >= 0);
	zassert_true(early_result >= 0);
}

static void *setup(void)
{
	int rv;

	monitor.callback = monitor_cb;
	rv = onoff_monitor_register(&mram_latency_mgr, &monitor);
	zassert_equal(rv, 0);

	if (early_rv >= 0) {
		sys_notify_fetch_result(&early_client.notify, &early_result);
	}

	mram_no_latency_cancel_or_release(&early_client);

	return NULL;
}

static void before(void *arg)
{
	zassert_equal(current_state, ONOFF_STATE_OFF);
}

static void after(void *arg)
{
	zassert_equal(current_state, ONOFF_STATE_OFF);
}

static int early_mram_client(void)
{
	sys_notify_init_spinwait(&early_client.notify);
	early_rv = mram_no_latency_request(&early_client);

	return 0;
}

SYS_INIT(early_mram_client, PRE_KERNEL_2, 0);

ZTEST_SUITE(mram_latency, NULL, setup, before, after, NULL);
