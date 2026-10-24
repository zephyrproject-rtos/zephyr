/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/i2c.h>

#define EEPROM_NODE     DT_ALIAS(eeprom_0)
#define EEPROM_I2C_ADDR DT_REG_ADDR(EEPROM_NODE)
#define I2C_NODE        DT_PARENT(EEPROM_NODE)
#define TEST_DATA_LEN   8

/* Address length in bytes derived from the overlay's address-width property */
#define EEPROM_ADDR_LEN (DT_PROP_OR(EEPROM_NODE, address_width, 8) / 8)

static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
static uint8_t write_data[TEST_DATA_LEN] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
static uint8_t read_data[TEST_DATA_LEN];

static void i2c_async_callback(const struct device *dev, int status, void *user_data)
{
	if (status == 0) {
		printk("I2C operation completed successfully\n");
	} else {
		printk("I2C operation failed with error: %d\n", status);
	}

	struct k_sem *sem = (struct k_sem *)user_data;

	k_sem_give(sem);
}

/*
 * Internal address bytes for the EEPROM. For 16-bit addressing: [0]=high byte, [1]=low byte.
 * For 8-bit addressing: only [0] is used (len=1).
 */
#if EEPROM_ADDR_LEN == 2
static uint8_t eeprom_addr[2] = {0x00, 0x01};
#else
static uint8_t eeprom_addr[1] = {0x01};
#endif

ZTEST(i2c_async, test_eeprom_async)
{
	static struct k_sem async_sem;
	int ret;

	k_sem_init(&async_sem, 0, 1);

	k_timeout_t timeout = K_MSEC(500);

	/* Write: address bytes followed by data payload */
	struct i2c_msg tx_msg[2] = {
		{.buf = eeprom_addr, .len = EEPROM_ADDR_LEN, .flags = I2C_MSG_WRITE},
		{.buf = write_data, .len = sizeof(write_data),
		 .flags = I2C_MSG_WRITE | I2C_MSG_STOP}
	};

	ret = i2c_transfer_cb(i2c_dev, tx_msg, 2, EEPROM_I2C_ADDR, i2c_async_callback,
			      &async_sem);
	zassert_equal(ret, 0, "EEPROM write failed: %d", ret);
	k_sem_take(&async_sem, timeout);

	/* Allow EEPROM internal write cycle to complete (AT24CM02: max 10 ms) */
	k_msleep(20);

	/* Read: write address bytes with repeated START, then read data */
	struct i2c_msg rx_msg[2] = {
		{.buf = eeprom_addr, .len = EEPROM_ADDR_LEN, .flags = I2C_MSG_WRITE},
		{.buf = read_data, .len = sizeof(read_data),
		 .flags = I2C_MSG_READ | I2C_MSG_RESTART | I2C_MSG_STOP}
	};

	ret = i2c_transfer_cb(i2c_dev, rx_msg, 2, EEPROM_I2C_ADDR, i2c_async_callback,
			      &async_sem);
	zassert_equal(ret, 0, "EEPROM read failed: %d", ret);
	k_sem_take(&async_sem, timeout);

	for (int i = 0; i < TEST_DATA_LEN; i++) {
		zassert_equal(read_data[i], write_data[i],
			      "Data mismatch at index %d: expected 0x%02X, got 0x%02X", i,
			      write_data[i], read_data[i]);
	}
}

void *i2c_test_setup(void)
{
	zassert_true(device_is_ready(i2c_dev), "I2C device is not ready");
	return NULL;
}

ZTEST_SUITE(i2c_async, NULL, i2c_test_setup, NULL, NULL, NULL);
