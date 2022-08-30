/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include "mocks/buf_help_utils.h"

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

/* Global iterator to iterate over the LUT */
static uint8_t testing_params_it;

/* Buffer type to be used during current test run */
enum bt_buf_type current_test_buffer_type;

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
void test_buffer_type_set_get_correctly(void)
{
	static struct net_buf testing_buffer;
	struct net_buf *buf = &testing_buffer;
	enum bt_buf_type buffer_type_set, returned_buffer_type;

	bt_buf_set_type(buf, current_test_buffer_type);

	returned_buffer_type = bt_buf_get_type(buf);
	buffer_type_set = ((struct bt_buf_data *)net_buf_user_data(buf))->type;

	zassert_equal(buffer_type_set, current_test_buffer_type,
		      "Buffer type %u set by bt_buf_set_type() is incorrect, expected %u",
		      buffer_type_set, current_test_buffer_type);

	zassert_equal(returned_buffer_type, current_test_buffer_type,
		      "Buffer type %u returned by bt_buf_get_type() is incorrect, expected %u",
		      returned_buffer_type, current_test_buffer_type);
}

/* Setup test variables */
static void unit_test_setup(void)
{
	zassert_true((testing_params_it < (ARRAY_SIZE(testing_params_lut))),
		     "Invalid testing parameters index %u", testing_params_it);
	current_test_buffer_type = testing_params_lut[testing_params_it];
	testing_params_it++;
}

void test_main(void)
{
	/* Each run will use a testing parameters set from LUT
	 * This can help in identifying which parameters fails instead of using
	 * a single 'void' test function and call the parameterized function.
	 */
	ztest_test_suite(test_bt_buf_get_set_get_type,
			 LISTIFY(TEST_PARAMETERS_LUT_ROWS_COUNT, REGISTER_SETUP_TEARDOWN, (,),
				 test_buffer_type_set_get_correctly));

	ztest_run_test_suite(test_bt_buf_get_set_get_type);
}
