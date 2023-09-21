/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */



/* test_conn_impl is separated into its own file specifically in order to test that
 * CONN_MGR_CONN_DECLARE_PUBLIC functions as expected.
 */


#ifndef ZEPHYR_INCLUDE_TEST_CONN_IMPL_H_
#define ZEPHYR_INCLUDE_TEST_CONN_IMPL_H_

#include <zephyr/net/conn_mgr_connectivity_impl.h>
#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TEST_CONN_OPT_X 0
#define TEST_CONN_OPT_Y 1
#define TEST_CONN_DATA_LEN 50

struct test_conn_data {
	/* The number of times an A-implementation API func has been called (other than init) */
	int call_cnt_a;

	/* The number of times a B-implementation API func has been called (other than init) */
	int call_cnt_b;

	/* Increases on each connect call, decreases on each disconnect call */
	int conn_bal;

	/* The number of times A-implementation init was called (should always be 1) */
	int init_calls_a;

	/* The number of times B-implementation init was called (should always be 1) */
	int init_calls_b;

	/* If nonzero, an error code the APIs should return. */
	int api_err;

	/* If true, the implementation should time out on connect. */
	bool timeout;

	/* If nonzero, the implementation should fail to connect and raise this fatal error. */
	int fatal_error;

	/* Places to store data from set_opt calls */
	char data_x[TEST_CONN_DATA_LEN + 1];
	char data_y[TEST_CONN_DATA_LEN + 1];
};

/* Create test L2 connectivity implementations A and B
 *
 * A and B share generic connect/disconnect implementations that differ only in which call counter
 * they increment.
 *
 * Additionally, A has conn_opt callbacks, whereas B does not.
 */
#define TEST_L2_CONN_IMPL_A_CTX_TYPE struct test_conn_data
CONN_MGR_CONN_DECLARE_PUBLIC(TEST_L2_CONN_IMPL_A);

#define TEST_L2_CONN_IMPL_B_CTX_TYPE struct test_conn_data
CONN_MGR_CONN_DECLARE_PUBLIC(TEST_L2_CONN_IMPL_B);

/* Create an invalid L2 connectivity implementation with NULL API */
#define TEST_L2_CONN_IMPL_N_CTX_TYPE struct test_conn_data
CONN_MGR_CONN_DECLARE_PUBLIC(TEST_L2_CONN_IMPL_N);

/* Create an L2 connectivity implementation without the optional init */
#define TEST_L2_CONN_IMPL_NI_CTX_TYPE struct test_conn_data
CONN_MGR_CONN_DECLARE_PUBLIC(TEST_L2_CONN_IMPL_NI);

#define SIMULATED_EVENT_DELAY_MS 100
#define SIMULATED_EVENT_DELAY_TIME K_MSEC(SIMULATED_EVENT_DELAY_MS)
#define SIMULATED_EVENT_WAIT_TIME K_MSEC(SIMULATED_EVENT_DELAY_MS + 10)

/* Helper API for controlling implementations from tests */

/**
 * @brief Simulate a connection loss on the target iface.
 *
 * @param target - iface to simulate connection loss on.
 */
void simulate_connection_loss(struct net_if *target);

/**
 * @brief Simulate a fatal error on the target iface
 *
 * Please do not simulate events on more than one iface at a time.
 *
 * @param target - iface to simulate fatal error on.
 * @param reason - Reason to be given for the fatal error.
 */
void simulate_fatal_error(struct net_if *target, int reason);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_TEST_CONN_IMPL_H_ */
