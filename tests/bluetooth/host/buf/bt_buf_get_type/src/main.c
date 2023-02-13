/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

/* Rows count equals number of types */
#define TEST_PARAMETERS_LUT_ROWS_COUNT		7

/* LUT containing testing parameters that will be used
 * during each iteration to cover different scenarios
 */
static const enum bt_buf_type testing_params_lut[] = {
	/** HCI command */
	BT_BUF_CMD,
	/** HCI event */
	BT_BUF_EVT,
	/** Outgoing ACL data */
	BT_BUF_ACL_OUT,
	/** Incoming ACL data */
	BT_BUF_ACL_IN,
	/** Outgoing ISO data */
	BT_BUF_ISO_OUT,
	/** Incoming ISO data */
	BT_BUF_ISO_IN,
	/** H:4 data */
	BT_BUF_H4,
};

BUILD_ASSERT(ARRAY_SIZE(testing_params_lut) == TEST_PARAMETERS_LUT_ROWS_COUNT);

ZTEST_SUITE(test_bt_buf_get_set_retrieve_type, NULL, NULL, NULL, NULL, NULL);

/*
 *  Buffer type is set and retrieved correctly
 *
 *  Constraints:
 *     - Use valid buffer buffer reference
 *     - Use valid buffer type
 *
 *  Expected behaviour:
 *     - Buffer type field inside 'struct net_buf' is set correctly
 *     - Retrieving buffer type through bt_buf_get_type() returns the correct
 *       value
 */
ZTEST(test_bt_buf_get_set_retrieve_type, test_buffer_type_set_get_correctly)
{
	static struct net_buf testing_buffer;
	struct net_buf *buf = &testing_buffer;
	enum bt_buf_type current_test_buffer_type;
	enum bt_buf_type buffer_type_set, returned_buffer_type;

	for (size_t i = 0; i < ARRAY_SIZE(testing_params_lut); i++) {

		current_test_buffer_type = testing_params_lut[i];

		bt_buf_set_type(buf, current_test_buffer_type);

		returned_buffer_type = bt_buf_get_type(buf);
		buffer_type_set = ((struct bt_buf_data *)net_buf_user_data(buf))->type;

		zassert_equal(buffer_type_set, current_test_buffer_type,
			      "Buffer type %u set by bt_buf_set_type() is incorrect, expected %u",
			      buffer_type_set, current_test_buffer_type);

		zassert_equal(
			returned_buffer_type, current_test_buffer_type,
			"Buffer type %u returned by bt_buf_get_type() is incorrect, expected %u",
			returned_buffer_type, current_test_buffer_type);
	}
}
