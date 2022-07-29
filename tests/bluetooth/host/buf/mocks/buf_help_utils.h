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

#define ztest_unit_test_setup(fn, setup) \
	ztest_unit_test_setup_teardown(fn, setup, unit_test_noop)

/*
 *  Validate expected behaviour when net_buf_alloc() is called
 *
 *  Expected behaviour:
 *   - net_buf_alloc() to be called once with :
 *       - correct memory allocation pool
 *       - same timeout value passed to bt_buf_get_cmd_complete()
 */
void validate_net_buf_alloc_called_behaviour(struct net_buf_pool *pool, k_timeout_t *timeout);

/*
 *  Validate expected behaviour when net_buf_alloc() isn't called
 *
 *  Expected behaviour:
 *   - net_buf_alloc() not to called at all
 */
void validate_net_buf_alloc_not_called_behaviour(void);

/*
 *  Validate expected behaviour when net_buf_reserve() is called
 *
 *  Expected behaviour:
 *   - net_buf_reserve() to be called once with :
 *       - correct reference value
 *       - 'reserve' argument set to 'BT_BUF_RESERVE' value
 */
void validate_net_buf_reserve_called_behaviour(struct net_buf *buf);

/*
 *  Validate expected behaviour when net_buf_reserve() isn't called
 *
 *  Expected behaviour:
 *   - net_buf_reserve() not to called at all
 */
void validate_net_buf_reserve_not_called_behaviour(void);

/*
 *  Validate expected behaviour when net_buf_ref() is called
 *
 *  Expected behaviour:
 *   - net_buf_ref() to be called once with :
 *       - correct reference value
 */
void validate_net_buf_ref_called_behaviour(struct net_buf *buf);

/*
 *  Validate expected behaviour when net_buf_ref() isn't called
 *
 *  Expected behaviour:
 *   - net_buf_ref() not to called at all
 */
void validate_net_buf_ref_not_called_behaviour(void);
