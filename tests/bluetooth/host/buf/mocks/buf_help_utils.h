/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include declarations and defines related to hooks */

#include <zephyr/zephyr.h>

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

/*  Validate the timeout integer value
 *
 *  A validation function that will be called with
 *  the timeout integer value as an input parameter.
 *
 *  It will check the passed input parameter to check its value if it
 *  matches the expected value using ztest_check_expected_value().
 *
 *  A typical use should call ztest_expect_value() to set the expected
 *  value before calling any function that will be required to validate
 *  a timeout value
 *
 *  @param value Input integer value to be checked
 */
void net_buf_validate_timeout_value_mock(uint32_t value);
