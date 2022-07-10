/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/bluetooth/buf.h>
#include <host/hci_core.h>
#include "kconfig.h"
#include "buf_help_utils.h"

/* Rows count equals number of events x 2 */
#define TEST_PARAMETERS_LUT_ROWS_COUNT		4

/* LUT containing testing parameters that will be used
 * during each iteration to cover different scenarios
 */
static const struct testing_params testing_params_lut[] = {
	TEST_PARAM_PAIR_DEFINE(BT_HCI_EVT_CMD_COMPLETE),
	TEST_PARAM_PAIR_DEFINE(BT_HCI_EVT_CMD_STATUS)
};

BUILD_ASSERT(ARRAY_SIZE(testing_params_lut) == TEST_PARAMETERS_LUT_ROWS_COUNT);

/* Global iterator to iterate over the LUT */
static uint8_t testing_params_it;

/* Pointer to the current set of testing parameter */
struct testing_params const *current_test_vector;

/* Return the memory pool used for event memory allocation
 * based on compilation flags
 */
static struct net_buf_pool *get_memory_pool(void)
{
	struct net_buf_pool *memory_pool;

	if ((IS_ENABLED(CONFIG_BT_HCI_ACL_FLOW_CONTROL))) {
		memory_pool = bt_buf_get_evt_pool();
	} else {
		memory_pool = bt_buf_get_hci_rx_pool();
	}

	return memory_pool;
}

/*
 *  Return value from bt_buf_get_evt() should match the value returned
 *  from bt_buf_get_cmd_complete() which is NULL
 *
 *  Constraints:
 *   - Event type 'BT_HCI_EVT_CMD_COMPLETE' or 'BT_HCI_EVT_CMD_STATUS'
 *   -'discardable' flag value doesn't care
 *   - bt_buf_get_cmd_complete() returns NULL
 *   - Timeout value is a positive non-zero value
 *
 *  Expected behaviour:
 *   - net_buf_alloc() to be called with the correct memory allocation pool
 *     and the same timeout value passed to bt_buf_get_evt()
 *   - bt_buf_get_evt() returns NULL
 */
static void test_return_value_matches_bt_buf_get_cmd_complete_null(void)
{
	struct net_buf *returned_buf;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);
	uint8_t evt = current_test_vector->evt;
	bool discardable = current_test_vector->discardable;

	bt_dev.sent_cmd = NULL;

	zassert_true((evt == BT_HCI_EVT_CMD_COMPLETE || evt == BT_HCI_EVT_CMD_STATUS),
		     "Invalid event type %u to this test", evt);

	ztest_expect_value(net_buf_alloc_fixed, pool, get_memory_pool());
	ztest_returns_value(net_buf_alloc_fixed, NULL);

	ztest_expect_value(net_buf_validate_timeout_value_mock, value, timeout.ticks);

	returned_buf = bt_buf_get_evt(evt, discardable, timeout);

	zassert_is_null(returned_buf,
			"bt_buf_get_evt() returned non-NULL value while expecting NULL");
}

/*
 *  Return value from bt_buf_get_evt() should match the value returned
 *  from bt_buf_get_cmd_complete() which isn't NULL
 *
 *  Constraints:
 *   - Event type 'BT_HCI_EVT_CMD_COMPLETE' or 'BT_HCI_EVT_CMD_STATUS'
 *   -'discardable' flag value doesn't care
 *   - bt_buf_get_cmd_complete() returns a valid reference
 *   - Timeout value is a positive non-zero value
 *
 *  Expected behaviour:
 *   - net_buf_alloc() to be called with the correct memory allocation pool
 *     and the same timeout value passed to bt_buf_get_evt()
 *   - bt_buf_get_evt() returns the same reference returned by net_buf_alloc_fixed()
 */
static void test_return_value_matches_bt_buf_get_cmd_complete_not_null(void)
{
	static struct net_buf expected_buf;
	struct net_buf *returned_buf;
	uint8_t returned_buffer_type;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);
	uint8_t evt = current_test_vector->evt;
	bool discardable = current_test_vector->discardable;

	zassert_true((evt == BT_HCI_EVT_CMD_COMPLETE || evt == BT_HCI_EVT_CMD_STATUS),
		     "Invalid event type %u to this test", evt);

	bt_dev.sent_cmd = NULL;

	ztest_expect_value(net_buf_simple_reserve, buf, &expected_buf.b);
	ztest_expect_value(net_buf_simple_reserve, reserve, BT_BUF_RESERVE);

	ztest_expect_value(net_buf_alloc_fixed, pool, get_memory_pool());
	ztest_returns_value(net_buf_alloc_fixed, &expected_buf);

	ztest_expect_value(net_buf_validate_timeout_value_mock, value, timeout.ticks);

	returned_buf = bt_buf_get_evt(evt, discardable, timeout);

	zassert_equal(returned_buf, &expected_buf,
		      "bt_buf_get_evt() returned incorrect buffer pointer value");

	returned_buffer_type = bt_buf_get_type(returned_buf);
	zassert_equal(returned_buffer_type, BT_BUF_EVT,
		      "bt_buf_get_evt() returned incorrect buffer type %u, expected %u (%s)",
		      returned_buffer_type, BT_BUF_EVT, STRINGIFY(BT_BUF_EVT));
}

/* Initialize test variables */
static void test_round_initialization(void)
{
	testing_params_it = 0;
	current_test_vector = NULL;
}

/* Setup test variables */
static void unit_test_setup(void)
{
	zassert_true((testing_params_it < (ARRAY_SIZE(testing_params_lut))),
		     "Invalid testing parameters index %u", testing_params_it);
	current_test_vector = &testing_params_lut[testing_params_it];
	testing_params_it++;
}

/* Each run will use a testing parameters set from LUT
 * This can help in identifying which parameters fails instead of using
 * a single 'void' test function and call the parameterized function.
 */
ztest_register_test_suite(bt_buf_get_evt_cmd_type_returns_null, NULL,
			  ztest_unit_test(test_round_initialization),
			  LISTIFY(TEST_PARAMETERS_LUT_ROWS_COUNT, REGISTER_SETUP_TEARDOWN, (,),
				  test_return_value_matches_bt_buf_get_cmd_complete_null));

ztest_register_test_suite(bt_buf_get_evt_cmd_type_returns_not_null, NULL,
			  ztest_unit_test(test_round_initialization),
			  LISTIFY(TEST_PARAMETERS_LUT_ROWS_COUNT, REGISTER_SETUP_TEARDOWN, (,),
				  test_return_value_matches_bt_buf_get_cmd_complete_not_null));
