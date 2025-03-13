/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#define EEPROM_NODE     DT_NODELABEL(eeprom0)
#define EEPROM_I2C_ADDR DT_REG_ADDR(EEPROM_NODE)
#define I2C_NODE        DT_PARENT(EEPROM_NODE)
#define TEST_DATA_LEN   8

static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST_PLUS) | I2C_MODE_CONTROLLER;
static uint8_t write_data[TEST_DATA_LEN] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
static uint8_t read_data[TEST_DATA_LEN];

/* write data to EEPROM */
uint8_t eeprom_addr = 1;
struct i2c_msg msgs[2];

/* Test Setup */
void *i2c_eeprom_setup(void)
{
	int ret;
	uint32_t i2c_cfg_tmp;

	zassert_true(device_is_ready(i2c_dev), "I2C device is not ready");

	ret = i2c_configure(i2c_dev, i2c_cfg);
	zassert_equal(ret, 0, "I2C configure failed with error: %d", ret);

	ret = i2c_get_config(i2c_dev, &i2c_cfg_tmp);
	zassert_equal(ret, 0, "I2C get_config failed with error: %d", ret);
	zassert_equal(i2c_cfg, i2c_cfg_tmp, "I2C get_config returned incorrect config");
	return NULL;
}

ZTEST(i2c_eeprom, test_eeprom_transfer)
{

	/* Write to EEPROM: address + data */
	msgs[0].buf = &eeprom_addr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = write_data;
	msgs[1].len = TEST_DATA_LEN;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	int ret = i2c_transfer(i2c_dev, msgs, 2, EEPROM_I2C_ADDR);

	zassert_equal(ret, 0, "EEPROM write failed: %d", ret);

	k_msleep(10);

	/* Read back: send address then read data */
	msgs[0].buf = &eeprom_addr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = read_data;
	msgs[1].len = TEST_DATA_LEN;
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	ret = i2c_transfer(i2c_dev, msgs, 2, EEPROM_I2C_ADDR);
	zassert_equal(ret, 0, "EEPROM read failed: %d", ret);

	for (int i = 0; i < TEST_DATA_LEN; i++) {
		zassert_equal(read_data[i], write_data[i],
			      "Data mismatch at index %d: expected 0x%02X, got 0x%02X", i,
			      write_data[i], read_data[i]);
	}
}

#ifdef CONFIG_I2C_CALLBACK
/* Callback function for I2C operations */
static void i2c_eeprom_callback(const struct device *dev, int status, void *user_data)
{
	/* Signal completion if needed */
	struct k_sem *sem = (struct k_sem *)user_data;

	k_sem_give(sem);
}

ZTEST(i2c_eeprom, test_eeprom_transfer_cb)
{
	/* Semaphore for signaling completion */
	static struct k_sem async_sem;

	k_sem_init(&async_sem, 0, 1);
	k_timeout_t timeout = K_MSEC(2000);

	/* Write to EEPROM: address + data */
	msgs[0].buf = &eeprom_addr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = write_data;
	msgs[1].len = TEST_DATA_LEN;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	int ret =
		i2c_transfer_cb(i2c_dev, msgs, 2, EEPROM_I2C_ADDR, i2c_eeprom_callback, &async_sem);
	zassert_equal(ret, 0, "EEPROM write failed: %d", ret);

	/* Wait for transfer to complete */
	k_sem_take(&async_sem, timeout);

	k_msleep(5);

	/* Read back: send address then read data */
	msgs[0].buf = &eeprom_addr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = read_data;
	msgs[1].len = TEST_DATA_LEN;
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	ret = i2c_transfer_cb(i2c_dev, msgs, 2, EEPROM_I2C_ADDR, i2c_eeprom_callback, &async_sem);
	zassert_equal(ret, 0, "EEPROM read failed: %d", ret);

	/* Wait for transfer to complete */
	k_sem_take(&async_sem, timeout);

	for (int i = 0; i < TEST_DATA_LEN; i++) {
		zassert_equal(read_data[i], write_data[i],
			      "Data mismatch at index %d: expected 0x%02X, got 0x%02X", i,
			      write_data[i], read_data[i]);
	}
}
#endif /* CONFIG_I2C_CALLBACK */

ZTEST_SUITE(i2c_eeprom, NULL, i2c_eeprom_setup, NULL, NULL, NULL);
