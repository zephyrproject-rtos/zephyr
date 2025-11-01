/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_lin_api
 * @{
 * @defgroup t_lin_api_basic test_lin_api_basic
 * @brief TestPurpose: verify LIN API basic functionality
 *
 * This test uses 2 nodes: a LIN master node and a LIN slave node for loopback testing.
 * @}
 */

#include <zephyr/drivers/lin.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(master))
#define LIN_MASTER DT_ALIAS(master)
#else
#error "Please set the correct LIN master device"
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(slave))
#define LIN_SLAVE DT_ALIAS(slave)
#else
#error "Please set the correct LIN slave device"
#endif

#define LIN_BUS_BAUDRATE            19200U
#define LIN_BUS_BREAK_LEN           13U
#define LIN_BUS_BREAK_DELIMITER_LEN 2U

#define LIN_MASTER_WRITE_ID 0x01
#define LIN_MASTER_READ_ID  0x02

static const struct device *const lin_master = DEVICE_DT_GET(LIN_MASTER);
static const struct device *const lin_slave = DEVICE_DT_GET(LIN_SLAVE);

static const struct lin_config master_cfg = {
	.mode = LIN_MODE_COMMANDER,
	.baudrate = LIN_BUS_BAUDRATE,
	.break_len = LIN_BUS_BREAK_LEN,
	.break_delimiter_len = LIN_BUS_BREAK_DELIMITER_LEN,
	.flags = 0,
};

static const struct lin_config slave_cfg = {
	.mode = LIN_MODE_RESPONDER,
	.baudrate = LIN_BUS_BAUDRATE,
	.break_len = LIN_BUS_BREAK_LEN,
	.break_delimiter_len = LIN_BUS_BREAK_DELIMITER_LEN,
	.flags = 0,
};

static const uint8_t data[LIN_MAX_DLEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
static struct k_sem transmission_completed;
static struct lin_msg slave_msg;

static void slave_evt_callback(const struct device *dev, const struct lin_event *event,
			       void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);
	int ret;

	switch (event->type) {
	case LIN_EVT_RX_HEADER:
		if (event->pid == LIN_MASTER_WRITE_ID) {
			ret = lin_read(lin_slave, &slave_msg, K_FOREVER);
			if (ret) {
				TC_PRINT("LIN slave read failed\n");
				return;
			}
		} else if (event->pid == LIN_MASTER_READ_ID) {
			ret = lin_response(lin_slave, &slave_msg, K_FOREVER);
			if (ret) {
				TC_PRINT("LIN slave send failed\n");
				return;
			}
		} else {
			TC_PRINT("Unexpected LIN ID\n");
			return;
		}
		break;
	case LIN_EVT_TX_DATA:
	case LIN_EVT_RX_DATA:
	case LIN_EVT_RX_WAKEUP:
	case LIN_EVT_ERR:
	default:
		break;
	}
}

static void master_evt_callback(const struct device *dev, const struct lin_event *event,
				void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (event->type) {
	case LIN_EVT_TX_DATA:
	case LIN_EVT_RX_DATA:
		k_sem_give(&transmission_completed);
		break;
	case LIN_EVT_RX_HEADER:
	case LIN_EVT_RX_WAKEUP:
	case LIN_EVT_ERR:
	default:
		break;
	}
}

static int test_lin_basic_api(struct lin_msg *master_msg, struct lin_msg *slave_msg)
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
	ret = lin_start(lin_master);
	if (ret) {
		TC_PRINT("LIN start failed\n");
		return TC_FAIL;
	}

	ret = lin_start(lin_slave);
	if (ret) {
		TC_PRINT("LIN slave start failed\n");
		return TC_FAIL;
	}

	/* 3. Master send header + data frames to slave */
	ret = lin_send(lin_master, master_msg, K_FOREVER);
	if (ret) {
		TC_PRINT("LIN send failed\n");
		return TC_FAIL;
	}

	/* 4. Wait for transmission completed */
	ret = k_sem_take(&transmission_completed, K_MSEC(100));
	if (ret) {
		TC_PRINT("Master write timeout\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

static void *test_lin_setup(void)
{
	k_sem_init(&transmission_completed, 0, 1);

	return NULL;
}

static void test_lin_before(void *f)
{
	ARG_UNUSED(f);

	k_sem_reset(&transmission_completed);
}

static void test_lin_after(void *f)
{
	ARG_UNUSED(f);

	lin_stop(lin_master);
	lin_stop(lin_slave);
}

static void test_lin_teardown(void *f)
{
	ARG_UNUSED(f);
}

ZTEST(lin_api_basic, test_lin_configure)
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

ZTEST(lin_api_basic, test_lin_master_write)
{
	struct lin_msg lin_master_msg = {
		.id = LIN_MASTER_WRITE_ID,
		.data_len = LIN_MAX_DLEN,
		.checksum_type = LIN_CHECKSUM_CLASSIC,
	};
	int ret;

	/* Prepare master message write data */
	memcpy(lin_master_msg.data, data, LIN_MAX_DLEN);

	zassert_true(device_is_ready(lin_master), "LIN master device is not ready");
	zassert_true(device_is_ready(lin_slave), "LIN slave device is not ready");

	/* Set master event handler */
	ret = lin_set_callback(lin_master, master_evt_callback, NULL);
	zassert_ok(ret, "Failed to set master event callback");

	/* Set slave event handler */
	ret = lin_set_callback(lin_slave, slave_evt_callback, NULL);
	zassert_ok(ret, "Failed to set slave event callback");

	/* Run the LIN master write test */
	ret = test_lin_basic_api(&lin_master_msg, &slave_msg);
	zassert_ok(ret, "LIN master write test failed");

	/* Verify LIN received data */
	zassert_equal(slave_msg.id, lin_master_msg.id, "LIN received ID does not match sent ID");
	zassert_mem_equal(slave_msg.data, lin_master_msg.data, lin_master_msg.data_len,
			  "LIN received data does not match sent data");
}

ZTEST(lin_api_basic, test_lin_master_read)
{
	struct lin_msg lin_master_msg = {
		.id = LIN_MASTER_READ_ID,
		.data_len = LIN_MAX_DLEN,
		.checksum_type = LIN_CHECKSUM_CLASSIC,
	};
	int ret;

	/* Prepare slave message response data */
	memcpy(slave_msg.data, data, LIN_MAX_DLEN);

	zassert_true(device_is_ready(lin_master), "LIN master device is not ready");
	zassert_true(device_is_ready(lin_slave), "LIN slave device is not ready");

	/* Set master event handler */
	ret = lin_set_callback(lin_master, master_evt_callback, NULL);
	zassert_ok(ret, "Failed to set master event callback");

	/* Set slave event handler */
	ret = lin_set_callback(lin_slave, slave_evt_callback, NULL);
	zassert_ok(ret, "Failed to set slave event callback");

	/* Run the master read test */
	ret = test_lin_basic_api(&lin_master_msg, &slave_msg);
	zassert_ok(ret, "LIN master read test failed");

	/* Verify LIN received data */
	zassert_equal(lin_master_msg.id, slave_msg.id, "LIN received ID does not match sent ID");
	zassert_mem_equal(lin_master_msg.data, slave_msg.data, lin_master_msg.data_len,
			  "LIN received data does not match sent data");
}

ZTEST_SUITE(lin_api_basic, NULL, test_lin_setup, test_lin_before, test_lin_after,
	    test_lin_teardown);
