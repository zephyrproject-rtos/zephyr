/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/bluetooth/buf.h>
#include "kconfig.h"
#include "buf_help_utils.h"
#include "assert.h"

/*
 *  Test passing invalid buffer type to bt_buf_get_rx()
 *
 *  Constraints:
 *   - Use invalid buffer type 'BT_BUF_CMD'
 *
 *  Expected behaviour:
 *   - An assertion should be raised as an invalid parameter was used
 */
void test_invalid_input_type_bt_buf_cmd(void)
{
	expect_assert();
	bt_buf_get_rx(BT_BUF_CMD, Z_TIMEOUT_TICKS(1000));
}

/*
 *  Test passing invalid buffer type to bt_buf_get_rx()
 *
 *  Constraints:
 *   - Use invalid buffer type 'BT_BUF_ACL_OUT'
 *
 *  Expected behaviour:
 *   - An assertion should be raised as an invalid parameter was used
 */
void test_invalid_input_type_bt_buf_acl_out(void)
{
	expect_assert();
	bt_buf_get_rx(BT_BUF_ACL_OUT, Z_TIMEOUT_TICKS(1000));
}

/*
 *  Test passing invalid buffer type to bt_buf_get_rx()
 *
 *  Constraints:
 *   - Use invalid buffer type 'BT_BUF_ISO_OUT'
 *
 *  Expected behaviour:
 *   - An assertion should be raised as an invalid parameter was used
 */
void test_invalid_input_type_bt_buf_iso_out(void)
{
	expect_assert();
	bt_buf_get_rx(BT_BUF_ISO_OUT, Z_TIMEOUT_TICKS(1000));
}

/*
 *  Test passing invalid buffer type to bt_buf_get_rx()
 *
 *  Constraints:
 *   - Use invalid buffer type 'BT_BUF_H4'
 *
 *  Expected behaviour:
 *   - An assertion should be raised as an invalid parameter was used
 */
void test_invalid_input_type_bt_buf_h4(void)
{
	expect_assert();
	bt_buf_get_rx(BT_BUF_H4, Z_TIMEOUT_TICKS(1000));
}

ztest_register_test_suite(test_bt_buf_get_rx_invalid_input, NULL,
			  ztest_unit_test(test_invalid_input_type_bt_buf_cmd),
			  ztest_unit_test(test_invalid_input_type_bt_buf_acl_out),
			  ztest_unit_test(test_invalid_input_type_bt_buf_iso_out),
			  ztest_unit_test(test_invalid_input_type_bt_buf_h4));
