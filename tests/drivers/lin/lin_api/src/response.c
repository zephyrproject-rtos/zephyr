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
 * @defgroup t_lin_basic_api test_lin_basic_api
 * @brief Test Purpose: Verify the basic functionality of the LIN API by using a commander node to
 * write and read a frame to and from a responder node, then verify data integrity.
 *
 * This test uses 2 nodes: a LIN commander node and a LIN responder node for loopback testing.
 * @}
 */

static void responder_evt_callback(const struct device *dev, const struct lin_event *event,
				   void *user_data)
{
	struct lin_msg *responder_msg = user_data;
	int ret;

	switch (event->type) {
	case LIN_EVT_RX_HEADER: {
		uint8_t id = lin_get_frame_id(event->header.pid);

		if (!lin_verify_pid(event->header.pid)) {
			TC_PRINT("LIN received invalid ID\n");
			return;
		}

		if (id == LIN_COMMANDER_WRITE_ID) {
			ret = lin_read(lin_responder, responder_msg, K_FOREVER);
			if (ret) {
				TC_PRINT("LIN responder read failed\n");
				return;
			}
		} else if (id == LIN_COMMANDER_READ_ID) {
			ret = lin_response(lin_responder, responder_msg, K_FOREVER);
			if (ret) {
				TC_PRINT("LIN responder send failed\n");
				return;
			}
		} else {
			TC_PRINT("Unexpected LIN ID\n");
			return;
		}

		break;
	}
	case LIN_EVT_TX_DATA: {
		if (event->status != 0) {
			TC_PRINT("LIN responder data transfer error: %d\n", event->status);
			return;
		}

		break;
	}
	case LIN_EVT_RX_DATA: {
		if (event->status != 0) {
			TC_PRINT("LIN responder data transfer error: %d\n", event->status);
			return;
		}

		if (lin_get_frame_id(event->data.pid) == LIN_COMMANDER_WRITE_ID) {
			k_sem_give(&transmission_completed);
		}

		break;
	}
	case LIN_EVT_RX_WAKEUP:
		__fallthrough;
	case LIN_EVT_ERR:
		__fallthrough;
	default:
		break;
	}
}

static void commander_evt_callback(const struct device *dev, const struct lin_event *event,
				   void *user_data)
{
	switch (event->type) {
	case LIN_EVT_TX_HEADER: {
		if (event->status != 0) {
			TC_PRINT("LIN commander header write error: %d\n", event->status);
			return;
		}

		break;
	}
	case LIN_EVT_TX_DATA: {
		if (event->status != 0) {
			TC_PRINT("LIN commander data write error: %d\n", event->status);
			return;
		}

		break;
	}
	case LIN_EVT_RX_DATA: {
		if (event->status != 0) {
			TC_PRINT("LIN commander data transfer error: %d\n", event->status);
			return;
		}

		if (lin_get_frame_id(event->data.pid) == LIN_COMMANDER_READ_ID) {
			k_sem_give(&transmission_completed);
		}

		break;
	}
	case LIN_EVT_RX_HEADER:
		__fallthrough;
	case LIN_EVT_RX_WAKEUP:
		__fallthrough;
	case LIN_EVT_ERR:
		__fallthrough;
	default:
		break;
	}
}

void test_lin_prepare_data(struct lin_msg *tx_msg, struct lin_msg *rx_msg, uint8_t frame_id)
{
	tx_msg->id = frame_id;
	tx_msg->data = (void *)lin_test_data;
	tx_msg->data_len = LIN_TEST_DATA_LEN;
	tx_msg->checksum_type = LIN_CHECKSUM_CLASSIC;

	rx_msg->id = frame_id;
	rx_msg->data = rx_buffer;
	rx_msg->data_len = LIN_TEST_DATA_LEN;
	rx_msg->checksum_type = LIN_CHECKSUM_CLASSIC;
}

static int test_lin_basic_api(bool is_read)
{
	int ret;

	/* 1. Configure LIN commander and responder nodes */
	ret = lin_configure(lin_commander, &commander_cfg);
	if (ret) {
		TC_PRINT("LIN configure failed\n");
		return TC_FAIL;
	}

	ret = lin_configure(lin_responder, &responder_cfg);
	if (ret) {
		TC_PRINT("LIN responder configure failed\n");
		return TC_FAIL;
	}

	/* 2. Allow LIN communication */
	ret = lin_start(lin_responder);
	if (ret) {
		TC_PRINT("LIN responder start failed\n");
		return TC_FAIL;
	}

	ret = lin_start(lin_commander);
	if (ret) {
		TC_PRINT("LIN start failed\n");
		return TC_FAIL;
	}

	if (!is_read) {
		/* 3. Commander send header + data frames (if any) to responder */
		ret = lin_send(lin_commander, &commander_msg, K_FOREVER);
		if (ret) {
			TC_PRINT("LIN send failed\n");
			return TC_FAIL;
		}
	} else {
		/* 3. Commander send header frame to responder to read data */
		ret = lin_receive(lin_commander, &commander_msg, K_FOREVER);
		if (ret) {
			TC_PRINT("LIN commander read failed\n");
			return TC_FAIL;
		}
	}

	/* 4. Wait for transmission completed */
	ret = k_sem_take(&transmission_completed, K_MSEC(1000));
	if (ret) {
		TC_PRINT("Transmission timeout\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

ZTEST(lin_basic_api, test_lin_configure)
{
	struct lin_config config_tmp;
	int ret;

	zassert_true(device_is_ready(lin_commander), "LIN commander device is not ready");
	zassert_true(device_is_ready(lin_responder), "LIN responder device is not ready");

	/* Configure LIN commander */
	ret = lin_configure(lin_commander, &commander_cfg);
	zassert_ok(ret, "Failed to configure LIN commander");

	/* Verify LIN commander configuration */
	ret = lin_get_config(lin_commander, &config_tmp);
	zassert_ok(ret, "Failed to get LIN commander config");
	zassert_mem_equal(&config_tmp, &commander_cfg, sizeof(commander_cfg),
			  "LIN commander config mismatch");

	/* Configure LIN responder */
	ret = lin_configure(lin_responder, &responder_cfg);
	zassert_ok(ret, "Failed to configure LIN responder");

	/* Verify LIN responder configuration */
	ret = lin_get_config(lin_responder, &config_tmp);
	zassert_ok(ret, "Failed to get LIN responder config");
	zassert_mem_equal(&config_tmp, &responder_cfg, sizeof(responder_cfg),
			  "LIN responder config mismatch");
}

ZTEST(lin_basic_api, test_commander_write)
{
	int ret;

	test_lin_prepare_data(&commander_msg, &responder_msg, LIN_COMMANDER_WRITE_ID);

	/* Set commander event handler */
	ret = lin_set_callback(lin_commander, commander_evt_callback, &commander_msg);
	zassert_ok(ret, "Failed to set commander event callback");

	/* Set responder event handler */
	ret = lin_set_callback(lin_responder, responder_evt_callback, &responder_msg);
	zassert_ok(ret, "Failed to set responder event callback");

	/* Run the LIN commander write test */
	ret = test_lin_basic_api(false);
	zassert_ok(ret, "LIN commander write test failed");

	/* Verify LIN received data */
	zassert_equal(commander_msg.id, responder_msg.id,
		      "LIN received ID [%x] does not match sent ID [%x]", responder_msg.id,
		      commander_msg.id);
	zassert_mem_equal(rx_buffer, lin_test_data, LIN_TEST_DATA_LEN,
			  "LIN received data does not match sent data");
}

ZTEST(lin_basic_api, test_commander_read)
{
	int ret;

	test_lin_prepare_data(&responder_msg, &commander_msg, LIN_COMMANDER_READ_ID);

	/* Set commander event handler */
	ret = lin_set_callback(lin_commander, commander_evt_callback, &commander_msg);
	zassert_ok(ret, "Failed to set commander event callback");

	/* Set responder event handler */
	ret = lin_set_callback(lin_responder, responder_evt_callback, &responder_msg);
	zassert_ok(ret, "Failed to set responder event callback");

	/* Run the commander read test */
	ret = test_lin_basic_api(true);
	zassert_ok(ret, "LIN commander read test failed");

	/* Verify LIN received data */
	zassert_equal(commander_msg.id, responder_msg.id,
		      "LIN received ID [%x] does not match sent ID [%x]", responder_msg.id,
		      commander_msg.id);
	zassert_mem_equal(rx_buffer, lin_test_data, LIN_TEST_DATA_LEN,
			  "LIN received data does not match sent data");
}

ZTEST_SUITE(lin_basic_api, NULL, test_lin_setup, test_lin_before, test_lin_after, NULL);
