/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/lin.h>
#include <zephyr/ztest.h>

#include "common.h"

/**
 * @addtogroup t_lin_driver
 * @{
 * @defgroup t_lin_errors test_lin_errors
 * @brief TestPurpose: verify LIN error handling functionality
 *
 * This test uses 2 nodes: a LIN commander node and a LIN responder node for testing parameter
 * handling in each mode.
 * @}
 */

ZTEST(lin_errors, test_lin_commander_errors)
{
	/** Dummy message for testing */
	struct lin_msg msg = {
		.id = LIN_COMMANDER_WRITE_ID,
		.data = (uint8_t *)lin_test_data,
		.data_len = LIN_TEST_DATA_LEN,
		.checksum_type = LIN_CHECKSUM_CLASSIC,
	};
	struct lin_filter filter;
	int ret;

	ret = lin_configure(lin_commander, &commander_cfg);
	zassert_ok(ret, "Failed to configure LIN commander node");

	ret = lin_start(lin_commander);
	zassert_ok(ret, "Failed to start LIN commander node");

	ret = lin_start(lin_commander);
	zassert_equal(ret, -EALREADY,
		      "lin_start() should fail when starting an already started node");

	ret = lin_response(lin_commander, &msg, K_MSEC(100));
	zassert_equal(ret, -EPERM, "lin_response() should fail when called in commander mode");

	ret = lin_read(lin_commander, &msg, K_MSEC(100));
	zassert_equal(ret, -EPERM, "lin_read() should fail when called in commander mode");

	ret = lin_set_rx_filter(lin_commander, &filter);
	zassert_equal(ret, -EPERM, "lin_set_rx_filter() should fail when called in commander mode");

	ret = lin_stop(lin_commander);
	zassert_ok(ret, "Failed to stop LIN commander node");

	ret = lin_stop(lin_commander);
	zassert_equal(ret, -EALREADY,
		      "lin_stop() should fail when stopping an already stopped node");
}

ZTEST(lin_errors, test_lin_responder_errors)
{
	/** Dummy message for testing */
	struct lin_msg msg = {
		.id = LIN_COMMANDER_WRITE_ID,
		.data = (uint8_t *)lin_test_data,
		.data_len = LIN_TEST_DATA_LEN,
		.checksum_type = LIN_CHECKSUM_CLASSIC,
	};
	int ret;

	ret = lin_configure(lin_responder, &responder_cfg);
	zassert_ok(ret, "Failed to configure LIN responder node");

	ret = lin_start(lin_responder);
	zassert_ok(ret, "Failed to start LIN responder node");

	ret = lin_start(lin_responder);
	zassert_equal(ret, -EALREADY,
		      "lin_start() should fail when starting an already started node");

	ret = lin_send(lin_responder, &msg, K_MSEC(100));
	zassert_equal(ret, -EPERM, "lin_send() should fail when called in responder mode");

	ret = lin_receive(lin_responder, &msg, K_MSEC(100));
	zassert_equal(ret, -EPERM, "lin_receive() should fail when called in responder mode");

	ret = lin_response(lin_responder, &msg, K_MSEC(100));
	zassert_equal(ret, -EFAULT, "lin_response() should fail when no header received");

	ret = lin_read(lin_responder, &msg, K_MSEC(100));
	zassert_equal(ret, -EFAULT, "lin_read() should fail when no header received");

	ret = lin_stop(lin_responder);
	zassert_ok(ret, "Failed to stop LIN responder node");

	ret = lin_stop(lin_responder);
	zassert_equal(ret, -EALREADY,
		      "lin_stop() should fail when stopping an already stopped node");
}

ZTEST_SUITE(lin_errors, NULL, test_lin_setup, test_lin_before, test_lin_after, NULL);
