/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_l2_connectivity.h>
#include "test_conn_impl.h"

static void inc_call_count(struct test_conn_data *data, bool a)
{
	if (a) {
		data->call_cnt_a += 1;
	} else {
		data->call_cnt_b += 1;
	}
}

static int test_connect(const struct net_if_conn *if_conn, bool a)
{
	struct test_conn_data *data = if_conn->ctx;

	inc_call_count(data, a);

	if (data->api_err != 0) {
		return data->api_err;
	}

	data->conn_bal += 1;
	/* Mark iface as connected */
	net_if_dormant_off(if_conn->iface);
	return 0;
}

static int test_disconnect(const struct net_if_conn *if_conn, bool a)
{
	struct test_conn_data *data = if_conn->ctx;

	inc_call_count(data, a);

	if (data->api_err != 0) {
		return data->api_err;
	}

	data->conn_bal -= 1;

	/* Mark iface as dormant (disconnected) */
	net_if_dormant_on(if_conn->iface);
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

int test_set_opt_a(const struct net_if_conn *if_conn, int optname,
		   const void *optval, size_t optlen)
{
	struct test_conn_data *data = if_conn->ctx;
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

int test_get_opt_a(const struct net_if_conn *if_conn, int optname,
		   void *optval, size_t *optlen)
{
	struct test_conn_data *data = if_conn->ctx;
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

static void test_init(const struct net_if_conn *if_conn, bool a)
{
	struct test_conn_data *data = if_conn->ctx;

	if (a) {
		data->init_calls_a += 1;
	} else {
		data->init_calls_b += 1;
	}

	/* Mark the iface dormant (disconnected) on initialization */
	net_if_dormant_on(if_conn->iface);
}

static void test_init_a(const struct net_if_conn *if_conn)
{
	test_init(if_conn, true);
}

static void test_init_b(const struct net_if_conn *if_conn)
{
	test_init(if_conn, false);
}

static int test_connect_a(const struct net_if_conn *if_conn)
{
	return test_connect(if_conn, true);
}

static int test_connect_b(const struct net_if_conn *if_conn)
{
	return test_connect(if_conn, false);
}

static int test_disconnect_a(const struct net_if_conn *if_conn)
{
	return test_disconnect(if_conn, true);
}

static int test_disconnect_b(const struct net_if_conn *if_conn)
{
	return test_disconnect(if_conn, false);
}

static struct net_l2_conn_api test_conn_api_a = {
	.connect = test_connect_a,
	.disconnect = test_disconnect_a,
	.init = test_init_a,
	.get_opt = test_get_opt_a,
	.set_opt = test_set_opt_a
};

static struct net_l2_conn_api test_conn_api_b = {
	.connect = test_connect_b,
	.disconnect = test_disconnect_b,
	.init = test_init_b,
};

static struct net_l2_conn_api test_conn_api_ni = {
	.connect = test_connect_a,
	.disconnect = test_disconnect_a,
};

/* Equivalent but distinct implementations */
NET_L2_CONNECTIVITY_DEFINE(TEST_L2_CONN_IMPL_A, &test_conn_api_a);
NET_L2_CONNECTIVITY_DEFINE(TEST_L2_CONN_IMPL_B, &test_conn_api_b);

/* Implementation without init */
NET_L2_CONNECTIVITY_DEFINE(TEST_L2_CONN_IMPL_NI, &test_conn_api_ni);

/* Bad implementation, should be handled gracefully */
NET_L2_CONNECTIVITY_DEFINE(TEST_L2_CONN_IMPL_N, NULL);
