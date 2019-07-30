/*
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr.h>
#include <device.h>
#include <stdio.h>
#include <sys/util.h>

#include <drivers/i2c.h>
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

	LOG_INF("Start full read. Master: %s, address: 0x%x",
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
		LOG_ERR("Buffer contents are different: %s",
			    buffer_print_i2c);
		LOG_ERR("                           vs: %s",
			    buffer_print_eeprom);

		ztest_test_fail();
	}
}

static void run_partial_read(struct device *i2c, u8_t addr, u8_t *comp_buffer,
			     unsigned int offset)
{
	int ret;

	LOG_INF("Start partial read. Master: %s, address: 0x%x, off=%d",
		    i2c->config->name, addr, offset);

	ret = i2c_burst_read(i2c, addr,
			     offset, i2c_buffer, TEST_DATA_SIZE-offset);
	zassert_equal(ret, 0, "Failed to read EEPROM");

	if (memcmp(i2c_buffer, &comp_buffer[offset], TEST_DATA_SIZE-offset)) {
		to_display_format(i2c_buffer, TEST_DATA_SIZE-offset,
				  buffer_print_i2c);
		to_display_format(&comp_buffer[offset], TEST_DATA_SIZE-offset,
				  buffer_print_eeprom);
		LOG_ERR("Buffer contents are different: %s",
			    buffer_print_i2c);
		LOG_ERR("                           vs: %s",
			    buffer_print_eeprom);

		ztest_test_fail();
	}
}

static void run_program_read(struct device *i2c, u8_t addr, unsigned int offset)
{
	int ret, i;

	LOG_INF("Start program. Master: %s, address: 0x%x, off=%d",
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
			LOG_ERR("Invalid Buffer content: %s",
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
			DT_INST_0_ATMEL_AT24_BUS_NAME);
	zassert_not_null(i2c_0, "I2C device %s not found",
			 DT_INST_0_ATMEL_AT24_BUS_NAME);

	LOG_INF("Found I2C Master device %s",
		    DT_INST_0_ATMEL_AT24_BUS_NAME);

	i2c_1 = device_get_binding(
			DT_INST_1_ATMEL_AT24_BUS_NAME);
	zassert_not_null(i2c_1, "I2C device %s not found",
			 DT_INST_1_ATMEL_AT24_BUS_NAME);

	LOG_INF("Found I2C Master device %s",
		    DT_INST_1_ATMEL_AT24_BUS_NAME);

	/*
	 * Normal applications would interact with an EEPROM
	 * identified by the string literal used in the binding node
	 * label property ("EEPROM_SLAVE_0") rather than the generated
	 * macro DT_INST_0_ATMEL_AT24_LABEL.  There is no guarantee that
	 * the index for the compatible is persistent across builds;
	 * for example DT_INST_0_ATMEL_AT24 might refer to "EEPROM_SLAVE_1"
	 * if the order of the node declarations were changed in the
	 * overlay file.
	 *
	 * The label string cannot be directly used to determine the
	 * correct parent bus and device index for whitebox testing in
	 * this application.  So for this application only, where the
	 * devices are interchangeable, the device is selected the
	 * using the generated macro.
	 */

	eeprom_0 = device_get_binding(DT_INST_0_ATMEL_AT24_LABEL);
	zassert_not_null(eeprom_0, "EEPROM device %s not found",
			 DT_INST_0_ATMEL_AT24_LABEL);

	LOG_INF("Found EEPROM device %s", DT_INST_0_ATMEL_AT24_LABEL);

	eeprom_1 = device_get_binding(DT_INST_1_ATMEL_AT24_LABEL);
	zassert_not_null(eeprom_1, "EEPROM device %s not found",
			 DT_INST_1_ATMEL_AT24_LABEL);

	LOG_INF("Found EEPROM device %s", DT_INST_1_ATMEL_AT24_LABEL);

	/* Program dummy bytes */
	ret = eeprom_slave_program(eeprom_0, eeprom_0_data, TEST_DATA_SIZE);
	zassert_equal(ret, 0, "Failed to program EEPROM %s",
		      DT_INST_0_ATMEL_AT24_LABEL);

	ret = eeprom_slave_program(eeprom_1, eeprom_1_data, TEST_DATA_SIZE);
	zassert_equal(ret, 0, "Failed to program EEPROM %s",
		      DT_INST_1_ATMEL_AT24_LABEL);

	/* Attach EEPROM */
	ret = i2c_slave_driver_register(eeprom_0);
	zassert_equal(ret, 0, "Failed to register EEPROM %s",
		      DT_INST_0_ATMEL_AT24_LABEL);

	LOG_INF("EEPROM %s Attached !", DT_INST_0_ATMEL_AT24_LABEL);

	ret = i2c_slave_driver_register(eeprom_1);
	zassert_equal(ret, 0, "Failed to register EEPROM %s",
		      DT_INST_1_ATMEL_AT24_LABEL);

	LOG_INF("EEPROM %s Attached !", DT_INST_1_ATMEL_AT24_LABEL);

	/* Run Tests without bus access conflicts */
	run_full_read(i2c_0, DT_INST_1_ATMEL_AT24_BASE_ADDRESS, eeprom_1_data);
	run_full_read(i2c_1, DT_INST_0_ATMEL_AT24_BASE_ADDRESS, eeprom_0_data);

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		run_partial_read(i2c_0, DT_INST_1_ATMEL_AT24_BASE_ADDRESS,
				 eeprom_1_data, offset);
	}

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		run_partial_read(i2c_1, DT_INST_0_ATMEL_AT24_BASE_ADDRESS,
				 eeprom_0_data, offset);
	}

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		run_program_read(i2c_0, DT_INST_1_ATMEL_AT24_BASE_ADDRESS,
				 offset);
	}

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		run_program_read(i2c_1, DT_INST_0_ATMEL_AT24_BASE_ADDRESS,
				 offset);
	}

	LOG_INF("Success !");

	/* Detach EEPROM */
	ret = i2c_slave_driver_unregister(eeprom_0);
	zassert_equal(ret, 0, "Failed to unregister EEPROM %s",
		      DT_INST_0_ATMEL_AT24_LABEL);

	LOG_INF("EEPROM %s Detached !",
		    DT_INST_0_ATMEL_AT24_LABEL);

	ret = i2c_slave_driver_unregister(eeprom_1);
	zassert_equal(ret, 0, "Failed to unregister EEPROM %s",
		      DT_INST_1_ATMEL_AT24_LABEL);

	LOG_INF("EEPROM %s Detached !",
		    DT_INST_1_ATMEL_AT24_LABEL);
}

void test_main(void)
{
	ztest_test_suite(test_eeprom_slave, ztest_unit_test(test_eeprom_slave));
	ztest_run_test_suite(test_eeprom_slave);
}
