/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/buf.h>
#include <host/hci_core.h>
#include "mocks/net_buf.h"
#include "mocks/net_buf_expects.h"
#include "mocks/buf_help_utils.h"

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

ZTEST_SUITE(bt_buf_get_evt_cmd_type_returns_null, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(bt_buf_get_evt_cmd_type_returns_not_null, NULL, NULL, NULL, NULL, NULL);

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
ZTEST(bt_buf_get_evt_cmd_type_returns_null, test_return_value_matches_bt_buf_get_cmd_complete_null)
{
	struct net_buf *returned_buf;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);
	struct testing_params const *params_vector;
	uint8_t evt;
	bool discardable;

	for (size_t i = 0; i < (ARRAY_SIZE(testing_params_lut)); i++) {

		/* Register resets */
		NET_BUF_FFF_FAKES_LIST(RESET_FAKE);

		params_vector = &testing_params_lut[i];
		evt = params_vector->evt;
		discardable = params_vector->discardable;

		zassert_true((evt == BT_HCI_EVT_CMD_COMPLETE || evt == BT_HCI_EVT_CMD_STATUS),
			     "Invalid event type %u to this test", evt);

		bt_dev.sent_cmd = NULL;
		net_buf_alloc_fixed_fake.return_val = NULL;

		returned_buf = bt_buf_get_evt(evt, discardable, timeout);

		expect_single_call_net_buf_alloc(get_memory_pool(), &timeout);
		expect_not_called_net_buf_reserve();
		expect_not_called_net_buf_ref();

		zassert_is_null(returned_buf,
				"bt_buf_get_evt() returned non-NULL value while expecting NULL");
	}
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
ZTEST(bt_buf_get_evt_cmd_type_returns_not_null,
	test_return_value_matches_bt_buf_get_cmd_complete_not_null)
{
	static struct net_buf expected_buf;
	struct net_buf *returned_buf;
	uint8_t returned_buffer_type;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);
	struct testing_params const *params_vector;
	uint8_t evt;
	bool discardable;

	for (size_t i = 0; i < (ARRAY_SIZE(testing_params_lut)); i++) {

		/* Register resets */
		NET_BUF_FFF_FAKES_LIST(RESET_FAKE);

		params_vector = &testing_params_lut[i];
		evt = params_vector->evt;
		discardable = params_vector->discardable;

		zassert_true((evt == BT_HCI_EVT_CMD_COMPLETE || evt == BT_HCI_EVT_CMD_STATUS),
			     "Invalid event type %u to this test", evt);

		bt_dev.sent_cmd = NULL;
		net_buf_alloc_fixed_fake.return_val = &expected_buf;

		returned_buf = bt_buf_get_evt(evt, discardable, timeout);

		expect_single_call_net_buf_alloc(get_memory_pool(), &timeout);
		expect_single_call_net_buf_reserve(&expected_buf);
		expect_not_called_net_buf_ref();

		zassert_equal(returned_buf, &expected_buf,
			      "bt_buf_get_evt() returned incorrect buffer pointer value");

		returned_buffer_type = bt_buf_get_type(returned_buf);
		zassert_equal(
			returned_buffer_type, BT_BUF_EVT,
			"bt_buf_get_evt() returned incorrect buffer type %u, expected %u (%s)",
			returned_buffer_type, BT_BUF_EVT, STRINGIFY(BT_BUF_EVT));
	}
}
