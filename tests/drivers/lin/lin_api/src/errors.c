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
 * This test uses 2 nodes: a LIN master node and a LIN slave node for testing parameter handling in
 * each mode.
 * @}
 */

ZTEST(lin_errors, test_lin_master_errors)
{
	/** Dummy message for testing */
	struct lin_msg msg = {
		.id = LIN_MASTER_WRITE_ID,
		.data = (uint8_t *)lin_test_data,
		.data_len = LIN_TEST_DATA_LEN,
		.checksum_type = LIN_CHECKSUM_CLASSIC,
	};
	struct lin_filter filter;
	int ret;

	ret = lin_configure(lin_master, &master_cfg);
	zassert_ok(ret, "Failed to configure LIN master node");

	ret = lin_start(lin_master);
	zassert_ok(ret, "Failed to start LIN master node");

	ret = lin_start(lin_master);
	zassert_equal(ret, -EALREADY,
		      "lin_start() should fail when starting an already started node");

	ret = lin_response(lin_master, &msg, K_MSEC(100));
	zassert_equal(ret, -EPERM, "lin_response() should fail when called in master mode");

	ret = lin_read(lin_master, &msg, K_MSEC(100));
	zassert_equal(ret, -EPERM, "lin_read() should fail when called in master mode");

	ret = lin_set_rx_filter(lin_master, &filter);
	zassert_equal(ret, -EPERM, "lin_set_rx_filter() should fail when called in master mode");

	ret = lin_stop(lin_master);
	zassert_ok(ret, "Failed to stop LIN master node");

	ret = lin_stop(lin_master);
	zassert_equal(ret, -EALREADY,
		      "lin_stop() should fail when stopping an already stopped node");
}

ZTEST(lin_errors, test_lin_slave_errors)
{
	/** Dummy message for testing */
	struct lin_msg msg = {
		.id = LIN_MASTER_WRITE_ID,
		.data = (uint8_t *)lin_test_data,
		.data_len = LIN_TEST_DATA_LEN,
		.checksum_type = LIN_CHECKSUM_CLASSIC,
	};
	int ret;

	ret = lin_configure(lin_slave, &slave_cfg);
	zassert_ok(ret, "Failed to configure LIN slave node");

	ret = lin_start(lin_slave);
	zassert_ok(ret, "Failed to start LIN slave node");

	ret = lin_start(lin_slave);
	zassert_equal(ret, -EALREADY,
		      "lin_start() should fail when starting an already started node");

	ret = lin_send(lin_slave, &msg, K_MSEC(100));
	zassert_equal(ret, -EPERM, "lin_send() should fail when called in slave mode");

	ret = lin_receive(lin_slave, &msg, K_MSEC(100));
	zassert_equal(ret, -EPERM, "lin_receive() should fail when called in slave mode");

	ret = lin_response(lin_slave, &msg, K_MSEC(100));
	zassert_equal(ret, -EFAULT, "lin_response() should fail when no header received");

	ret = lin_read(lin_slave, &msg, K_MSEC(100));
	zassert_equal(ret, -EFAULT, "lin_read() should fail when no header received");

	ret = lin_stop(lin_slave);
	zassert_ok(ret, "Failed to stop LIN slave node");

	ret = lin_stop(lin_slave);
	zassert_equal(ret, -EALREADY,
		      "lin_stop() should fail when stopping an already stopped node");
}

ZTEST_SUITE(lin_errors, NULL, test_lin_setup, test_lin_before, test_lin_after, NULL);
