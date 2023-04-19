/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/net_if.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include "test_conn_impl.h"

/* Event simulation */
static int simulated_event;
static struct net_if *simulated_event_iface;
static K_MUTEX_DEFINE(simulated_event_mutex);

/* Static storage for fatal error info */
static int fatal_error;

void simulate_event_handler(struct k_work *work)
{
	ARG_UNUSED(*work);

	k_mutex_lock(&simulated_event_mutex, K_FOREVER);

	if (simulated_event == 0) {
		net_mgmt_event_notify(
			NET_EVENT_CONN_IF_TIMEOUT,
			simulated_event_iface
		);
	} else {
		fatal_error = simulated_event;
		net_mgmt_event_notify_with_info(
			NET_EVENT_CONN_IF_FATAL_ERROR,
			simulated_event_iface, &fatal_error, sizeof(fatal_error)
		);
	}

	k_mutex_unlock(&simulated_event_mutex);
}

static K_WORK_DELAYABLE_DEFINE(simulate_event_work, simulate_event_handler);

/**
 * @brief Simulates an event on the target iface.
 *
 * Do not attempt to simulate multiple events simultaneously -- only the last event requested
 * will be fired.
 *
 * @param target - iface to simulate the event on.
 * @param event - Event to simulate.
 *		  If 0, simulate a timeout.
 *		  Otherwise, simulate a fatal error with this value as the reason/info.
 */
static void simulate_event(struct net_if *target, int event)
{
	k_mutex_lock(&simulated_event_mutex, K_FOREVER);

	simulated_event = event;
	simulated_event_iface = target;
	k_work_reschedule(&simulate_event_work, K_SECONDS(SIMULATED_EVENT_DELAY_SECONDS));

	k_mutex_unlock(&simulated_event_mutex);
}

static void simulate_timeout(struct net_if *target)
{
	simulate_event(target, 0);
}

static void simulate_fatal_error(struct net_if *target, int reason)
{
	simulate_event(target, reason);
}

/* Connectivity implementations */

static void inc_call_count(struct test_conn_data *data, bool a)
{
	if (a) {
		data->call_cnt_a += 1;
	} else {
		data->call_cnt_b += 1;
	}
}

static int test_connect(struct conn_mgr_conn_binding *const binding, bool a)
{
	struct test_conn_data *data = binding->ctx;

	inc_call_count(data, a);

	/* Fail immediately if requested */
	if (data->api_err != 0) {
		return data->api_err;
	}

	/* Fail after a delay if requested */
	if (data->fatal_error) {
		simulate_fatal_error(binding->iface, data->fatal_error);
		return 0;
	}

	if (data->timeout) {
		simulate_timeout(binding->iface);
		return 0;
	}

	/* Succeed otherwise */

	data->conn_bal += 1;

	/* Mark iface as connected */
	net_if_dormant_off(binding->iface);
	return 0;
}

static int test_disconnect(struct conn_mgr_conn_binding *const binding, bool a)
{
	struct test_conn_data *data = binding->ctx;

	inc_call_count(data, a);

	if (data->api_err != 0) {
		return data->api_err;
	}

	data->conn_bal -= 1;

	/* Mark iface as dormant (disconnected) */
	net_if_dormant_on(binding->iface);
	return 0;
}

char *opt_pointer(struct test_conn_data *data, int optname)
{
	switch (optname) {
	case TEST_CONN_OPT_X:
		return data->data_x;
	case TEST_CONN_OPT_Y:
		return data->data_y;
	}
	return NULL;
}

int test_set_opt_a(struct conn_mgr_conn_binding *const binding, int optname,
		   const void *optval, size_t optlen)
{
	struct test_conn_data *data = binding->ctx;
	char *target = opt_pointer(data, optname);
	int len = MIN(optlen, TEST_CONN_DATA_LEN);

	/* get/set opt are only implemented for implementation A */
	inc_call_count(data, true);

	if (target == NULL) {
		return -ENOPROTOOPT;
	}

	if (data->api_err) {
		return data->api_err;
	}

	(void)memset(target, 0, TEST_CONN_DATA_LEN);
	(void)memcpy(target, optval, len);

	return 0;
}

int test_get_opt_a(struct conn_mgr_conn_binding *const binding, int optname,
		   void *optval, size_t *optlen)
{
	struct test_conn_data *data = binding->ctx;
	char *target = opt_pointer(data, optname);
	int len;

	/* get/set opt are only implemented for implementation A */
	inc_call_count(data, true);

	if (target == NULL) {
		*optlen = 0;
		return -ENOPROTOOPT;
	}

	len = MIN(strlen(target) + 1, *optlen);

	if (data->api_err) {
		*optlen = 0;
		return data->api_err;
	}
	*optlen = len;
	(void)memset(optval, 0, len);
	(void)memcpy(optval, target, len-1);

	return 0;
}

static void test_init(struct conn_mgr_conn_binding *const binding, bool a)
{
	struct test_conn_data *data = binding->ctx;

	if (a) {
		data->init_calls_a += 1;
	} else {
		data->init_calls_b += 1;
	}

	/* Mark the iface dormant (disconnected) on initialization */
	net_if_dormant_on(binding->iface);
}

static void test_init_a(struct conn_mgr_conn_binding *const binding)
{
	test_init(binding, true);
}

static void test_init_b(struct conn_mgr_conn_binding *const binding)
{
	test_init(binding, false);
}

static int test_connect_a(struct conn_mgr_conn_binding *const binding)
{
	return test_connect(binding, true);
}

static int test_connect_b(struct conn_mgr_conn_binding *const binding)
{
	return test_connect(binding, false);
}

static int test_disconnect_a(struct conn_mgr_conn_binding *const binding)
{
	return test_disconnect(binding, true);
}

static int test_disconnect_b(struct conn_mgr_conn_binding *const binding)
{
	return test_disconnect(binding, false);
}

static struct conn_mgr_conn_api test_conn_api_a = {
	.connect = test_connect_a,
	.disconnect = test_disconnect_a,
	.init = test_init_a,
	.get_opt = test_get_opt_a,
	.set_opt = test_set_opt_a
};

static struct conn_mgr_conn_api test_conn_api_b = {
	.connect = test_connect_b,
	.disconnect = test_disconnect_b,
	.init = test_init_b,
};

static struct conn_mgr_conn_api test_conn_api_ni = {
	.connect = test_connect_a,
	.disconnect = test_disconnect_a,
};

/* Equivalent but distinct implementations */
CONN_MGR_CONN_DEFINE(TEST_L2_CONN_IMPL_A, &test_conn_api_a);
CONN_MGR_CONN_DEFINE(TEST_L2_CONN_IMPL_B, &test_conn_api_b);

/* Implementation without init */
CONN_MGR_CONN_DEFINE(TEST_L2_CONN_IMPL_NI, &test_conn_api_ni);

/* Bad implementation, should be handled gracefully */
CONN_MGR_CONN_DEFINE(TEST_L2_CONN_IMPL_N, NULL);
