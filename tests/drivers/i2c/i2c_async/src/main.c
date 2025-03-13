/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/i2c.h>

#define EEPROM_NODE     DT_NODELABEL(eeprom0)
#define EEPROM_I2C_ADDR DT_REG_ADDR(EEPROM_NODE)
#define I2C_NODE        DT_PARENT(EEPROM_NODE)
#define TEST_DATA_LEN   8
#define EEPROM_ADDR_LEN  1

static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
static uint8_t write_data[TEST_DATA_LEN] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
static uint8_t read_data[TEST_DATA_LEN];

/* write data to EEPROM */
uint8_t eeprom_addr = 1;

struct i2c_msg tx_msg[2] = {{.buf = &eeprom_addr, .len = EEPROM_ADDR_LEN, .flags = I2C_MSG_WRITE},
			    {.buf = write_data, .len = sizeof(write_data), .flags = I2C_MSG_WRITE}};

/* Read data back from the EEPROM */
struct i2c_msg rx_msg[2] = {{.buf = &eeprom_addr, .len = EEPROM_ADDR_LEN, .flags = I2C_MSG_WRITE},
			    {.buf = read_data, .len = sizeof(read_data), .flags = I2C_MSG_READ}};

#if defined(CONFIG_I2C_CALLBACK)
/* Callback function for I2C operations */
static void i2c_async_callback(const struct device *dev, int status, void *user_data)
{
	if (status == 0) {
		printk("I2C operation completed successfully\n");
	} else {
		printk("I2C operation failed with error: %d\n", status);
	}

	/* Signal completion if needed */
	struct k_sem *sem = (struct k_sem *)user_data;

	k_sem_give(sem);
}
#endif /* CONFIG_I2C_CALLBACK */

ZTEST(i2c_async, test_i2c_configure)
{
	int err;
	uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST_PLUS);
	uint32_t i2c_cfg_tmp;

	err = i2c_configure(i2c_dev, i2c_cfg);
	printk("conf=%d\n", I2C_SPEED_GET(i2c_cfg));
	zassert_equal(err, 0, "I2C configure failed with error: %d", err);

	err = i2c_get_config(i2c_dev, &i2c_cfg_tmp);
	printk("conf1=%d\n", i2c_cfg_tmp);
	zassert_equal(err, 0, "I2C get_config failed with error: %d", err);
	zassert_equal(I2C_SPEED_GET(i2c_cfg), i2c_cfg_tmp,
		      "I2C get_config returned incorrect config");
}

#if !defined(CONFIG_I2C_CALLBACK)
ZTEST(i2c_async, test_eeprom_int)
{
	int ret;

	ret = i2c_transfer(i2c_dev, tx_msg, 2, EEPROM_I2C_ADDR);
	zassert_equal(ret, 0, "EEPROM write failed: %d", ret);

	k_msleep(10);

	ret = i2c_transfer(i2c_dev, rx_msg, 2, EEPROM_I2C_ADDR);
	zassert_equal(ret, 0, "EEPROM read failed: %d", ret);

	for (int i = 0; i < TEST_DATA_LEN; i++) {
		zassert_equal(read_data[i], write_data[i],
			      "Data mismatch at index %d: expected 0x%02X, got 0x%02X", i,
			      write_data[i], read_data[i]);
	}
}
#endif /* CONFIG_I2C_CALLBACK */

#if defined(CONFIG_I2C_CALLBACK)
ZTEST(i2c_async, test_eeprom_async)
{
	/* Semaphore for signaling completion */
	static struct k_sem async_sem;

	k_sem_init(&async_sem, 0, 1);
	k_timeout_t timeout = K_MSEC(500);

	int ret = i2c_transfer_cb(i2c_dev, tx_msg, 2, EEPROM_I2C_ADDR, i2c_async_callback,
				  &async_sem);
	zassert_equal(ret, 0, "EEPROM write failed: %d", ret);

	/* Wait for transfer to complete */
	k_sem_take(&async_sem, timeout);

	ret = i2c_transfer_cb(i2c_dev, rx_msg, 2, EEPROM_I2C_ADDR, i2c_async_callback, &async_sem);
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

/* Test Setup */
void *i2c_test_setup(void)
{
	zassert_true(device_is_ready(i2c_dev), "I2C device is not ready");
	return NULL;
}

ZTEST_SUITE(i2c_async, NULL, i2c_test_setup, NULL, NULL, NULL);
