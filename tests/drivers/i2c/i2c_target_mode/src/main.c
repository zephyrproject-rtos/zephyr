/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/i2c.h>

#define TEST_DATA_LEN 8

static const struct device *i2c_target = DEVICE_DT_GET(DT_NODELABEL(dut));
static const struct device *i2c_controller = DEVICE_DT_GET(DT_NODELABEL(dut_aux));

static uint8_t write_data[TEST_DATA_LEN] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
static uint8_t read_data[TEST_DATA_LEN];

int ret;

/* Target mode callbacks */
static uint8_t target_read_data[TEST_DATA_LEN];
static uint8_t next_byte;
static uint8_t t_next_byte;

/* Callback functions */
int sample_target_write_requested_cb(struct i2c_target_config *config)
{
	printk("Sample target write requested\n");
	return 0;
}

int sample_target_write_received_cb(struct i2c_target_config *config, uint8_t val)
{
	printk("Sample target write received: 0x%02x\n", val);
	if (next_byte < TEST_DATA_LEN) {
		target_read_data[next_byte++] = val;
	}
	return 0;
}

int sample_target_read_requested_cb(struct i2c_target_config *config, uint8_t *val)
{
	*val = write_data[t_next_byte++];
	return 0;
}

int sample_target_read_processed_cb(struct i2c_target_config *config, uint8_t *val)
{
	printk("Sample target read processed: 0x%02x\n", *val);
	if (t_next_byte < TEST_DATA_LEN) {
		*val = write_data[t_next_byte++];
	}
	return 0;
}

int sample_target_stop_cb(struct i2c_target_config *config)
{
	printk("Sample target stop callback\n");
	return 0;
}

static struct i2c_target_callbacks sample_target_callbacks = {
	.write_requested = sample_target_write_requested_cb,
	.write_received = sample_target_write_received_cb,
	.read_requested = sample_target_read_requested_cb,
	.read_processed = sample_target_read_processed_cb,
	.stop = sample_target_stop_cb,
};

struct i2c_msg tx_msg = {.buf = write_data, .len = TEST_DATA_LEN, .flags = I2C_MSG_WRITE};

struct i2c_msg rx_msg = {.buf = read_data, .len = TEST_DATA_LEN, .flags = I2C_MSG_READ};

ZTEST(i2c_target_mode, test_target_mode)
{
	struct i2c_target_config target_cfg = {
		.address = 0x54UL,
		.callbacks = &sample_target_callbacks,
	};

	ret = i2c_target_register(i2c_target, &target_cfg);
	zassert_equal(ret, 0, "TARGET registration failed: %d", ret);

	ret = i2c_transfer(i2c_controller, &tx_msg, 1, target_cfg.address);
	zassert_equal(ret, 0, "Target write failed: %d", ret);

	k_msleep(10);

	ret = i2c_transfer(i2c_controller, &rx_msg, 1, target_cfg.address);
	zassert_equal(ret, 0, "Target read failed: %d", ret);

	ret = i2c_target_unregister(i2c_target, &target_cfg);
	zassert_equal(ret, 0, "TARGET unregistration failed: %d", ret);

	for (int i = 0; i < TEST_DATA_LEN; i++) {
		zassert_equal(target_read_data[i], write_data[i],
			      "Data mismatch at index %d: expected 0x%02X, got 0x%02X", i,
			      write_data[i], target_read_data[i]);
	}
}

void *i2c_test_setup(void)
{
	zassert_true(device_is_ready(i2c_controller), "I2C controller device is not ready");
	zassert_true(device_is_ready(i2c_target), "I2C target device is not ready");
	return NULL;
}

ZTEST_SUITE(i2c_target_mode, NULL, i2c_test_setup, NULL, NULL, NULL);
