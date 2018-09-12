/*
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <logging/sys_log.h>

#include <zephyr.h>
#include <device.h>
#include <stdio.h>
#include <misc/util.h>

#include <i2c.h>
#include <drivers/i2c/slave/eeprom.h>

#include <ztest.h>

#define TEST_DATA_SIZE	20

static u8_t eeprom_0_data[TEST_DATA_SIZE] = "0123456789abcdefghij";
static u8_t eeprom_1_data[TEST_DATA_SIZE] = "jihgfedcba9876543210";
static u8_t i2c_buffer[TEST_DATA_SIZE];

/*
 * We need 5x(buffer size) + 1 to print a comma-separated list of each
 * byte in hex, plus a null.
 */
u8_t buffer_print_eeprom[TEST_DATA_SIZE * 5 + 1];
u8_t buffer_print_i2c[TEST_DATA_SIZE * 5 + 1];

static void to_display_format(const u8_t *src, size_t size, char *dst)
{
	size_t i;

	for (i = 0; i < size; i++) {
		sprintf(dst + 5 * i, "0x%02x,", src[i]);
	}
}

static void run_full_read(struct device *i2c, u8_t addr, u8_t *comp_buffer)
{
	int ret;

	SYS_LOG_INF("Start full read. Master: %s, address: 0x%x",
		    i2c->config->name, addr);

	/* Read EEPROM from I2C Master requests, then compare */
	ret = i2c_burst_read(i2c, addr,
			     0, i2c_buffer, TEST_DATA_SIZE);
	zassert_equal(ret, 0, "Failed to read EEPROM");

	if (memcmp(i2c_buffer, comp_buffer, TEST_DATA_SIZE)) {
		to_display_format(i2c_buffer, TEST_DATA_SIZE,
				  buffer_print_i2c);
		to_display_format(comp_buffer, TEST_DATA_SIZE,
				  buffer_print_eeprom);
		SYS_LOG_ERR("Buffer contents are different: %s",
			    buffer_print_i2c);
		SYS_LOG_ERR("                           vs: %s",
			    buffer_print_eeprom);

		ztest_test_fail();
	}
}

static void run_partial_read(struct device *i2c, u8_t addr, u8_t *comp_buffer,
			     unsigned int offset)
{
	int ret;

	SYS_LOG_INF("Start partial read. Master: %s, address: 0x%x, off=%d",
		    i2c->config->name, addr, offset);

	ret = i2c_burst_read(i2c, addr,
			     offset, i2c_buffer, TEST_DATA_SIZE-offset);
	zassert_equal(ret, 0, "Failed to read EEPROM");

	if (memcmp(i2c_buffer, &comp_buffer[offset], TEST_DATA_SIZE-offset)) {
		to_display_format(i2c_buffer, TEST_DATA_SIZE-offset,
				  buffer_print_i2c);
		to_display_format(&comp_buffer[offset], TEST_DATA_SIZE-offset,
				  buffer_print_eeprom);
		SYS_LOG_ERR("Buffer contents are different: %s",
			    buffer_print_i2c);
		SYS_LOG_ERR("                           vs: %s",
			    buffer_print_eeprom);

		ztest_test_fail();
	}
}

static void run_program_read(struct device *i2c, u8_t addr, unsigned int offset)
{
	int ret, i;

	SYS_LOG_INF("Start program. Master: %s, address: 0x%x, off=%d",
		    i2c->config->name, addr, offset);

	for (i = 0 ; i < TEST_DATA_SIZE-offset ; ++i) {
		i2c_buffer[i] = i;
	}

	ret = i2c_burst_write(i2c, addr,
			      offset, i2c_buffer, TEST_DATA_SIZE-offset);
	zassert_equal(ret, 0, "Failed to write EEPROM");

	(void)memset(i2c_buffer, 0xFF, TEST_DATA_SIZE);

	/* Read back EEPROM from I2C Master requests, then compare */
	ret = i2c_burst_read(i2c, addr,
			     offset, i2c_buffer, TEST_DATA_SIZE-offset);
	zassert_equal(ret, 0, "Failed to read EEPROM");

	for (i = 0 ; i < TEST_DATA_SIZE-offset ; ++i) {
		if (i2c_buffer[i] != i) {
			to_display_format(i2c_buffer, TEST_DATA_SIZE-offset,
					  buffer_print_i2c);
			SYS_LOG_ERR("Invalid Buffer content: %s",
				    buffer_print_i2c);

			ztest_test_fail();
		}
	}
}

void test_eeprom_slave(void)
{
	struct device *eeprom_0;
	struct device *eeprom_1;
	struct device *i2c_0;
	struct device *i2c_1;
	int ret, offset;

	i2c_0 = device_get_binding(
			CONFIG_I2C_EEPROM_SLAVE_0_CONTROLLER_DEV_NAME);
	zassert_not_null(i2c_0, "I2C device %s not found",
			 CONFIG_I2C_EEPROM_SLAVE_0_CONTROLLER_DEV_NAME);

	SYS_LOG_INF("Found I2C Master device %s",
		    CONFIG_I2C_EEPROM_SLAVE_0_CONTROLLER_DEV_NAME);

	i2c_1 = device_get_binding(
			CONFIG_I2C_EEPROM_SLAVE_1_CONTROLLER_DEV_NAME);
	zassert_not_null(i2c_1, "I2C device %s not found",
			 CONFIG_I2C_EEPROM_SLAVE_1_CONTROLLER_DEV_NAME);

	SYS_LOG_INF("Found I2C Master device %s",
		    CONFIG_I2C_EEPROM_SLAVE_1_CONTROLLER_DEV_NAME);

	eeprom_0 = device_get_binding(CONFIG_I2C_EEPROM_SLAVE_0_NAME);
	zassert_not_null(eeprom_0, "EEPROM device %s not found",
			 CONFIG_I2C_EEPROM_SLAVE_0_NAME);

	SYS_LOG_INF("Found EEPROM device %s", CONFIG_I2C_EEPROM_SLAVE_0_NAME);

	eeprom_1 = device_get_binding(CONFIG_I2C_EEPROM_SLAVE_1_NAME);
	zassert_not_null(eeprom_1, "EEPROM device %s not found",
			 CONFIG_I2C_EEPROM_SLAVE_1_NAME);

	SYS_LOG_INF("Found EEPROM device %s", CONFIG_I2C_EEPROM_SLAVE_1_NAME);

	/* Program dummy bytes */
	ret = eeprom_slave_program(eeprom_0, eeprom_0_data, TEST_DATA_SIZE);
	zassert_equal(ret, 0, "Failed to program EEPROM %s",
		      CONFIG_I2C_EEPROM_SLAVE_0_NAME);

	ret = eeprom_slave_program(eeprom_1, eeprom_1_data, TEST_DATA_SIZE);
	zassert_equal(ret, 0, "Failed to program EEPROM %s",
		      CONFIG_I2C_EEPROM_SLAVE_1_NAME);

	/* Attach EEPROM */
	ret = i2c_slave_driver_register(eeprom_0);
	zassert_equal(ret, 0, "Failed to register EEPROM %s",
		      CONFIG_I2C_EEPROM_SLAVE_0_NAME);

	SYS_LOG_INF("EEPROM %s Attached !", CONFIG_I2C_EEPROM_SLAVE_0_NAME);

	ret = i2c_slave_driver_register(eeprom_1);
	zassert_equal(ret, 0, "Failed to register EEPROM %s",
		      CONFIG_I2C_EEPROM_SLAVE_1_NAME);

	SYS_LOG_INF("EEPROM %s Attached !", CONFIG_I2C_EEPROM_SLAVE_1_NAME);

	/* Run Tests without bus access conflicts */
	run_full_read(i2c_0, CONFIG_I2C_EEPROM_SLAVE_1_ADDRESS, eeprom_1_data);
	run_full_read(i2c_1, CONFIG_I2C_EEPROM_SLAVE_0_ADDRESS, eeprom_0_data);

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		run_partial_read(i2c_0, CONFIG_I2C_EEPROM_SLAVE_1_ADDRESS,
				 eeprom_1_data, offset);
	}

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		run_partial_read(i2c_1, CONFIG_I2C_EEPROM_SLAVE_0_ADDRESS,
				 eeprom_0_data, offset);
	}

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		run_program_read(i2c_0, CONFIG_I2C_EEPROM_SLAVE_1_ADDRESS,
				 offset);
	}

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		run_program_read(i2c_1, CONFIG_I2C_EEPROM_SLAVE_0_ADDRESS,
				 offset);
	}

	SYS_LOG_INF("Success !");

	/* Detach EEPROM */
	ret = i2c_slave_driver_unregister(eeprom_0);
	zassert_equal(ret, 0, "Failed to unregister EEPROM %s",
		      CONFIG_I2C_EEPROM_SLAVE_0_NAME);

	SYS_LOG_INF("EEPROM %s Detached !",
		    CONFIG_I2C_EEPROM_SLAVE_0_NAME);

	ret = i2c_slave_driver_unregister(eeprom_1);
	zassert_equal(ret, 0, "Failed to unregister EEPROM %s",
		      CONFIG_I2C_EEPROM_SLAVE_1_NAME);

	SYS_LOG_INF("EEPROM %s Detached !",
		    CONFIG_I2C_EEPROM_SLAVE_1_NAME);
}

void test_main(void)
{
	ztest_test_suite(test_eeprom_slave, ztest_unit_test(test_eeprom_slave));
	ztest_run_test_suite(test_eeprom_slave);
}
