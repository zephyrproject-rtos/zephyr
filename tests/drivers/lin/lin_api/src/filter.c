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
 * @defgroup t_lin_filter test_lin_filter
 * @brief TestPurpose: verify LIN filter functionality
 *
 * This test uses 2 nodes: a LIN commander node and a LIN responder node for loopback testing.
 * @}
 */

static volatile uint32_t header_count;
static volatile uint8_t last_pid;

static void lin_filter_test_responder_callback(const struct device *dev,
					       const struct lin_event *event, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (event->type) {
	case LIN_EVT_RX_HEADER:
		header_count++;
		last_pid = event->header.pid;
		break;
	case LIN_EVT_TX_DATA:
		__fallthrough;
	case LIN_EVT_RX_DATA:
		__fallthrough;
	case LIN_EVT_RX_WAKEUP:
		__fallthrough;
	case LIN_EVT_ERR:
		__fallthrough;
	default:
		break;
	}
}

static void lin_filter_test_commander_callback(const struct device *dev,
					       const struct lin_event *event, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (event->type) {
	case LIN_EVT_TX_HEADER:
		k_sem_give(&transmission_completed);
		break;
	case LIN_EVT_TX_DATA:
		__fallthrough;
	case LIN_EVT_RX_DATA:
		__fallthrough;
	case LIN_EVT_RX_HEADER:
		__fallthrough;
	case LIN_EVT_TX_WAKEUP:
		__fallthrough;
	case LIN_EVT_RX_WAKEUP:
		__fallthrough;
	default:
		break;
	}
}

static int test_lin_filter_test_startup(void)
{
	int ret;

	/* Set commander event handler */
	ret = lin_set_callback(lin_commander, lin_filter_test_commander_callback, NULL);
	if (ret != 0) {
		return ret;
	}

	/* Set responder event handler */
	ret = lin_set_callback(lin_responder, lin_filter_test_responder_callback, NULL);
	if (ret != 0) {
		return ret;
	}

	/* Configure LIN commander and responder nodes */
	ret = lin_configure(lin_commander, &commander_cfg);
	if (ret != 0) {
		return ret;
	}

	ret = lin_configure(lin_responder, &responder_cfg);
	if (ret != 0) {
		return ret;
	}

	/* Start LIN communication */
	ret = lin_start(lin_responder);
	if (ret != 0) {
		return ret;
	}

	ret = lin_start(lin_commander);
	if (ret != 0) {
		return ret;
	}

	return ret;
}

static int test_lin_filter_set(uint8_t frame_id, uint8_t mask)
{
	struct lin_filter filter;

	filter.primary_pid = frame_id;
	filter.secondary_pid = 0;
	filter.mask = mask;

	return lin_set_rx_filter(lin_responder, &filter);
}

static int test_lin_header_send(uint8_t frame_id)
{
	struct lin_msg msg;

	msg.id = frame_id;
	msg.data = NULL;
	msg.data_len = 0;
	msg.checksum_type = LIN_CHECKSUM_CLASSIC;

	return lin_send(lin_commander, &msg, K_FOREVER);
}

ZTEST(lin_filter, test_accept_ids)
{
	uint8_t test_pids[] = {0xA6, 0xA3, 0xA8, 0xAD};
	int ret;

	header_count = 0;
	last_pid = 0;

	ret = test_lin_filter_test_startup();
	zassert_ok(ret, "Failed to start LIN filter test");

	/* Set filter to accept IDs 0xA0 - 0xAF */
	ret = test_lin_filter_set(0xA0, 0xF0);
	zassert_ok(ret, "Failed to set LIN filter");

	/* Send IDs that match the filter */
	for (size_t i = 0; i < ARRAY_SIZE(test_pids); i++) {
		ret = test_lin_header_send(lin_get_frame_id(test_pids[i]));
		zassert_ok(ret, "Failed to send LIN header with ID 0x%02X: ret=%d", test_pids[i],
			   ret);

		ret = k_sem_take(&transmission_completed, K_MSEC(1000));
		zassert_ok(ret, "Last transmission is not completed: Frame ID 0x%02X",
			   test_pids[i]);

		/* Verify that only IDs matching the filter were received */
		zassert_equal(header_count, (i + 1),
			      "Unexpected number of headers received: %d != %d", header_count,
			      (i + 1));
		zassert_equal(last_pid, test_pids[i],
			      "Last PID received (0x%02X) does not match filter", last_pid);
	}
}

ZTEST(lin_filter, test_reject_ids)
{
	uint8_t test_pids[] = {0x80, 0xC1, 0x42, 0x03};
	int ret;

	header_count = 0;
	last_pid = 0;

	ret = test_lin_filter_test_startup();
	zassert_ok(ret, "Failed to start LIN filter test");

	/* Set filter to accept ID 0x20 */
	ret = test_lin_filter_set(0x20, 0xFF);
	zassert_ok(ret, "Failed to set LIN filter");

	/* Send IDs that do not match the filter */
	for (size_t i = 0; i < ARRAY_SIZE(test_pids); i++) {
		ret = test_lin_header_send(lin_get_frame_id(test_pids[i]));
		zassert_ok(ret, "Failed to send LIN header with ID 0x%02X", test_pids[i]);

		ret = k_sem_take(&transmission_completed, K_MSEC(1000));
		zassert_ok(ret, "Last transmission is not completed: Frame ID 0x%02X",
			   test_pids[i]);

		/* Verify that no IDs matching the filter were received */
		zassert_equal(header_count, 0, "Unexpected headers received");
		zassert_equal(last_pid, 0, "Last PID received (0x%02X) should be zero", last_pid);
	}
}

ZTEST_SUITE(lin_filter, NULL, test_lin_setup, test_lin_before, test_lin_after, NULL);
