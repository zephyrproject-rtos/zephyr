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
	return 0;
}

int sample_target_write_received_cb(struct i2c_target_config *config, uint8_t val)
{
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
	if (t_next_byte < TEST_DATA_LEN) {
		*val = write_data[t_next_byte++];
	}
	return 0;
}

int sample_target_stop_cb(struct i2c_target_config *config)
{
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

uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST_PLUS) | I2C_MODE_CONTROLLER;

ZTEST(i2c_loopback, test_target_mode)
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

#ifdef CONFIG_I2C_CALLBACK
/* Callback function for I2C operations */
static void i2c_callback(const struct device *dev, int status, void *user_data)
{
	/* Signal completion if needed */
	struct k_sem *sem = (struct k_sem *)user_data;

	k_sem_give(sem);
}

ZTEST(i2c_loopback, test_target_mode_cb)
{
	/* Semaphore for signaling completion */
	static struct k_sem async_sem;

	k_sem_init(&async_sem, 0, 1);
	k_timeout_t timeout = K_MSEC(2000);

	struct i2c_target_config target_cfg = {
		.address = 0x54UL,
		.callbacks = &sample_target_callbacks,
	};

	ret = i2c_target_register(i2c_target, &target_cfg);
	zassert_equal(ret, 0, "TARGET registration failed: %d", ret);

	ret = i2c_transfer_cb(i2c_controller, &tx_msg, 1, target_cfg.address, i2c_callback,
			      &async_sem);
	zassert_equal(ret, 0, "Target write failed: %d", ret);

	/* Wait for transfer to complete */
	k_sem_take(&async_sem, timeout);

	ret = i2c_transfer_cb(i2c_controller, &rx_msg, 1, target_cfg.address, i2c_callback,
			      &async_sem);
	zassert_equal(ret, 0, "Target read failed: %d", ret);

	/* Wait for transfer to complete */
	k_sem_take(&async_sem, timeout);

	ret = i2c_target_unregister(i2c_target, &target_cfg);
	zassert_equal(ret, 0, "TARGET unregistration failed: %d", ret);

	for (int i = 0; i < TEST_DATA_LEN; i++) {
		zassert_equal(target_read_data[i], write_data[i],
			      "Data mismatch at index %d: expected 0x%02X, got 0x%02X", i,
			      write_data[i], target_read_data[i]);
	}
}
#endif /* CONFIG_I2C_CALLBACK */

void *i2c_test_setup(void)
{
	zassert_true(device_is_ready(i2c_controller), "I2C controller device is not ready");
	zassert_true(device_is_ready(i2c_target), "I2C target device is not ready");

	uint32_t i2c_cfg_tmp;

	ret = i2c_configure(i2c_controller, i2c_cfg);
	zassert_equal(ret, 0, "I2C configure failed with error: %d", ret);

	ret = i2c_get_config(i2c_controller, &i2c_cfg_tmp);
	zassert_equal(ret, 0, "I2C get_config failed with error: %d", ret);
	zassert_equal(i2c_cfg, i2c_cfg_tmp, "I2C get_config returned incorrect config");
	return NULL;
}

ZTEST_SUITE(i2c_loopback, NULL, i2c_test_setup, NULL, NULL, NULL);
