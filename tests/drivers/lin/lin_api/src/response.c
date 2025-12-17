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
 * @brief Test Purpose: Verify the basic functionality of the LIN API by using a master node to
 * write and read a frame to and from a slave node, then verify data integrity.
 *
 * This test uses 2 nodes: a LIN master node and a LIN slave node for loopback testing.
 * @}
 */

static void slave_evt_callback(const struct device *dev, const struct lin_event *event,
			       void *user_data)
{
	struct lin_msg *slave_msg = user_data;
	int ret;

	switch (event->type) {
	case LIN_EVT_RX_HEADER: {
		uint8_t id = lin_get_frame_id(event->header.pid);

		if (!lin_verify_pid(event->header.pid)) {
			TC_PRINT("LIN received invalid ID\n");
			return;
		}

		if (id == LIN_MASTER_WRITE_ID) {
			ret = lin_read(lin_slave, slave_msg, K_FOREVER);
			if (ret) {
				TC_PRINT("LIN slave read failed\n");
				return;
			}
		} else if (id == LIN_MASTER_READ_ID) {
			ret = lin_response(lin_slave, slave_msg, K_FOREVER);
			if (ret) {
				TC_PRINT("LIN slave send failed\n");
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
			TC_PRINT("LIN slave data transfer error: %d\n", event->status);
			return;
		}

		break;
	}
	case LIN_EVT_RX_DATA: {
		if (event->status != 0) {
			TC_PRINT("LIN slave data transfer error: %d\n", event->status);
			return;
		}

		if (lin_get_frame_id(event->data.pid) == LIN_MASTER_WRITE_ID) {
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

static void master_evt_callback(const struct device *dev, const struct lin_event *event,
				void *user_data)
{
	switch (event->type) {
	case LIN_EVT_TX_HEADER: {
		if (event->status != 0) {
			TC_PRINT("LIN master header write error: %d\n", event->status);
			return;
		}

		break;
	}
	case LIN_EVT_TX_DATA: {
		if (event->status != 0) {
			TC_PRINT("LIN master data write error: %d\n", event->status);
			return;
		}

		break;
	}
	case LIN_EVT_RX_DATA: {
		if (event->status != 0) {
			TC_PRINT("LIN master data transfer error: %d\n", event->status);
			return;
		}

		if (lin_get_frame_id(event->data.pid) == LIN_MASTER_READ_ID) {
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

	/* 1. Configure LIN master and slave nodes */
	ret = lin_configure(lin_master, &master_cfg);
	if (ret) {
		TC_PRINT("LIN configure failed\n");
		return TC_FAIL;
	}

	ret = lin_configure(lin_slave, &slave_cfg);
	if (ret) {
		TC_PRINT("LIN slave configure failed\n");
		return TC_FAIL;
	}

	/* 2. Allow LIN communication */
	ret = lin_start(lin_slave);
	if (ret) {
		TC_PRINT("LIN slave start failed\n");
		return TC_FAIL;
	}

	ret = lin_start(lin_master);
	if (ret) {
		TC_PRINT("LIN start failed\n");
		return TC_FAIL;
	}

	if (!is_read) {
		/* 3. Master send header + data frames (if any) to slave */
		ret = lin_send(lin_master, &master_msg, K_FOREVER);
		if (ret) {
			TC_PRINT("LIN send failed\n");
			return TC_FAIL;
		}
	} else {
		/* 3. Master send header frame to slave to read data */
		ret = lin_receive(lin_master, &master_msg, K_FOREVER);
		if (ret) {
			TC_PRINT("LIN master read failed\n");
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

	zassert_true(device_is_ready(lin_master), "LIN master device is not ready");
	zassert_true(device_is_ready(lin_slave), "LIN slave device is not ready");

	/* Configure LIN master */
	ret = lin_configure(lin_master, &master_cfg);
	zassert_ok(ret, "Failed to configure LIN master");

	/* Verify LIN master configuration */
	ret = lin_get_config(lin_master, &config_tmp);
	zassert_ok(ret, "Failed to get LIN master config");
	zassert_mem_equal(&config_tmp, &master_cfg, sizeof(master_cfg),
			  "LIN master config mismatch");

	/* Configure LIN slave */
	ret = lin_configure(lin_slave, &slave_cfg);
	zassert_ok(ret, "Failed to configure LIN slave");

	/* Verify LIN slave configuration */
	ret = lin_get_config(lin_slave, &config_tmp);
	zassert_ok(ret, "Failed to get LIN slave config");
	zassert_mem_equal(&config_tmp, &slave_cfg, sizeof(slave_cfg), "LIN slave config mismatch");
}

ZTEST(lin_basic_api, test_master_write)
{
	int ret;

	test_lin_prepare_data(&master_msg, &slave_msg, LIN_MASTER_WRITE_ID);

	/* Set master event handler */
	ret = lin_set_callback(lin_master, master_evt_callback, &master_msg);
	zassert_ok(ret, "Failed to set master event callback");

	/* Set slave event handler */
	ret = lin_set_callback(lin_slave, slave_evt_callback, &slave_msg);
	zassert_ok(ret, "Failed to set slave event callback");

	/* Run the LIN master write test */
	ret = test_lin_basic_api(false);
	zassert_ok(ret, "LIN master write test failed");

	/* Verify LIN received data */
	zassert_equal(master_msg.id, slave_msg.id,
		      "LIN received ID [%x] does not match sent ID [%x]", slave_msg.id,
		      master_msg.id);
	zassert_mem_equal(rx_buffer, lin_test_data, LIN_TEST_DATA_LEN,
			  "LIN received data does not match sent data");
}

ZTEST(lin_basic_api, test_master_read)
{
	int ret;

	test_lin_prepare_data(&slave_msg, &master_msg, LIN_MASTER_READ_ID);

	/* Set master event handler */
	ret = lin_set_callback(lin_master, master_evt_callback, &master_msg);
	zassert_ok(ret, "Failed to set master event callback");

	/* Set slave event handler */
	ret = lin_set_callback(lin_slave, slave_evt_callback, &slave_msg);
	zassert_ok(ret, "Failed to set slave event callback");

	/* Run the master read test */
	ret = test_lin_basic_api(true);
	zassert_ok(ret, "LIN master read test failed");

	/* Verify LIN received data */
	zassert_equal(master_msg.id, slave_msg.id,
		      "LIN received ID [%x] does not match sent ID [%x]", slave_msg.id,
		      master_msg.id);
	zassert_mem_equal(rx_buffer, lin_test_data, LIN_TEST_DATA_LEN,
			  "LIN received data does not match sent data");
}

ZTEST_SUITE(lin_basic_api, NULL, test_lin_setup, test_lin_before, test_lin_after, NULL);
