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

static uint8_t eeprom_0_data[TEST_DATA_SIZE] = "0123456789abcdefghij";
static uint8_t eeprom_1_data[TEST_DATA_SIZE] = "jihgfedcba9876543210";
static uint8_t i2c_buffer[TEST_DATA_SIZE];

/*
 * We need 5x(buffer size) + 1 to print a comma-separated list of each
 * byte in hex, plus a null.
 */
uint8_t buffer_print_eeprom[TEST_DATA_SIZE * 5 + 1];
uint8_t buffer_print_i2c[TEST_DATA_SIZE * 5 + 1];

static void to_display_format(const uint8_t *src, size_t size, char *dst)
{
	size_t i;

	for (i = 0; i < size; i++) {
		sprintf(dst + 5 * i, "0x%02x,", src[i]);
	}
}

static int run_full_read(struct device *i2c, uint8_t addr, uint8_t *comp_buffer)
{
	int ret;

	LOG_INF("Start full read. Master: %s, address: 0x%x",
		    i2c->name, addr);

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
		LOG_ERR("                 vs expected: %s",
			    buffer_print_eeprom);
		return -EINVAL;
	}

	return 0;
}

static int run_partial_read(struct device *i2c, uint8_t addr,
			    uint8_t *comp_buffer, unsigned int offset)
{
	int ret;

	LOG_INF("Start partial read. Master: %s, address: 0x%x, off=%d",
		    i2c->name, addr, offset);

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
		LOG_ERR("                  vs expected: %s",
			    buffer_print_eeprom);
		return -EINVAL;
	}

	return 0;
}

static int run_program_read(struct device *i2c, uint8_t addr, unsigned int offset)
{
	int ret, i;

	LOG_INF("Start program. Master: %s, address: 0x%x, off=%d",
		    i2c->name, addr, offset);

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
			LOG_ERR("Invalid Buffer content: %s", buffer_print_i2c);
			return -EINVAL;
		}
	}

	return 0;
}

void test_eeprom_slave(void)
{
	struct device *eeprom_0;
	struct device *eeprom_1;
	struct device *i2c_0;
	struct device *i2c_1;
	int ret, offset;
	int reg_addr0 = DT_REG_ADDR(DT_INST(0, atmel_at24));
	int reg_addr1 = DT_REG_ADDR(DT_INST(1, atmel_at24));
	const char *label_eeprom0 = DT_LABEL(DT_INST(0, atmel_at24));
	const char *label_eeprom1 = DT_LABEL(DT_INST(1, atmel_at24));

	i2c_0 = device_get_binding(DT_BUS_LABEL(DT_INST(0, atmel_at24)));
	zassert_not_null(i2c_0, "I2C device %s not found",
			 DT_BUS_LABEL(DT_INST(0, atmel_at24)));

	LOG_INF("Found I2C Master device %s",
		    DT_BUS_LABEL(DT_INST(0, atmel_at24)));

	i2c_1 = device_get_binding(DT_BUS_LABEL(DT_INST(1, atmel_at24)));
	zassert_not_null(i2c_1, "I2C device %s not found",
			 DT_BUS_LABEL(DT_INST(1, atmel_at24)));

	LOG_INF("Found I2C Master device %s",
		    DT_BUS_LABEL(DT_INST(1, atmel_at24)));

	/*
	 * Normal applications would interact with an EEPROM
	 * identified by the string literal used in the binding node
	 * label property ("EEPROM_SLAVE_0") rather than the generated
	 * macro DT_LABEL(DT_INST(0, atmel_at24)).  There is no guarantee that
	 * the index for the compatible is persistent across builds;
	 * for example DT_LABEL(DT_INST(0, atmel_at24)) might refer to
	 * "EEPROM_SLAVE_1" * if the order of the node declarations were changed
	 * in the overlay file.
	 *
	 * The label string cannot be directly used to determine the
	 * correct parent bus and device index for whitebox testing in
	 * this application.  So for this application only, where the
	 * devices are interchangeable, the device is selected the
	 * using the generated macro.
	 */

	eeprom_0 = device_get_binding(label_eeprom0);
	zassert_not_null(eeprom_0, "EEPROM device %s not found", label_eeprom0);

	LOG_INF("Found EEPROM device %s", label_eeprom0);

	eeprom_1 = device_get_binding(label_eeprom1);
	zassert_not_null(eeprom_1, "EEPROM device %s not found", label_eeprom1);

	LOG_INF("Found EEPROM device %s", label_eeprom1);

	/* Program dummy bytes */
	ret = eeprom_slave_program(eeprom_0, eeprom_0_data, TEST_DATA_SIZE);
	zassert_equal(ret, 0, "Failed to program EEPROM %s", label_eeprom0);

	ret = eeprom_slave_program(eeprom_1, eeprom_1_data, TEST_DATA_SIZE);
	zassert_equal(ret, 0, "Failed to program EEPROM %s", label_eeprom1);

	/* Attach EEPROM */
	ret = i2c_slave_driver_register(eeprom_0);
	zassert_equal(ret, 0, "Failed to register EEPROM %s", label_eeprom0);

	LOG_INF("EEPROM %s Attached !", label_eeprom0);

	ret = i2c_slave_driver_register(eeprom_1);
	zassert_equal(ret, 0, "Failed to register EEPROM %s", label_eeprom1);

	LOG_INF("EEPROM %s Attached !", label_eeprom1);

	/* Run Tests without bus access conflicts */
	zassert_equal(0, run_full_read(i2c_0, reg_addr1, eeprom_1_data),
		     "Full read from #1 failed");
	zassert_equal(0, run_full_read(i2c_1, reg_addr0, eeprom_0_data),
		     "Full read from #2 failed");

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		zassert_equal(0, run_partial_read(i2c_0, reg_addr1,
			      eeprom_1_data, offset),
			      "Partial read i2c0 inst1 failed");
	}

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		zassert_equal(0, run_partial_read(i2c_1, reg_addr0,
			      eeprom_0_data, offset),
			      "Partial read i2c1 inst0 failed");
	}

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		zassert_equal(0, run_program_read(i2c_0, reg_addr1, offset),
			      "Program read i2c0 inst1 failed");
	}

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		zassert_equal(0, run_program_read(i2c_1, reg_addr0, offset),
			      "Program read i2c1 inst0 failed");
	}

	LOG_INF("Success !");

	/* Detach EEPROM */
	ret = i2c_slave_driver_unregister(eeprom_0);
	zassert_equal(ret, 0, "Failed to unregister EEPROM %s", label_eeprom0);

	LOG_INF("EEPROM %s Detached !", label_eeprom0);

	ret = i2c_slave_driver_unregister(eeprom_1);
	zassert_equal(ret, 0, "Failed to unregister EEPROM %s", label_eeprom1);

	LOG_INF("EEPROM %s Detached !", label_eeprom1);
}

void test_main(void)
{
	ztest_test_suite(test_eeprom_slave, ztest_unit_test(test_eeprom_slave));
	ztest_run_test_suite(test_eeprom_slave);
}
