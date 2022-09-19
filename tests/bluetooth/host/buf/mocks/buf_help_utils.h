/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

struct net_buf_pool *bt_buf_get_evt_pool(void);
struct net_buf_pool *bt_buf_get_acl_in_pool(void);
struct net_buf_pool *bt_buf_get_hci_rx_pool(void);
struct net_buf_pool *bt_buf_get_discardable_pool(void);
struct net_buf_pool *bt_buf_get_num_complete_pool(void);
struct net_buf_pool *bt_buf_get_discardable_pool(void);
struct net_buf_pool *bt_buf_get_num_complete_pool(void);
struct net_buf_pool *bt_buf_get_iso_rx_pool(void);

/* LUT testing parameter item */
struct testing_params {
	uint8_t evt;			/*	Event type */
	bool discardable;		/*	Discardable flag	*/
};

#define TEST_PARAM_PAIR_DEFINE(EVT) {EVT, true}, {EVT, false}

/* Repeat test entries */
#define REGISTER_SETUP_TEARDOWN(i, ...) \
	ztest_unit_test_setup_teardown(__VA_ARGS__, unit_test_setup, unit_test_noop)

#define ztest_unit_test_setup(fn, setup) \
	ztest_unit_test_setup_teardown(fn, setup, unit_test_noop)
