/*
 * Copyright (c) 2017 BayLibre, SAS
 * Copyright (c) 2020 Nordic Semiconductor ASA
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
#include "eeprom.h"

#include <ztest.h>

#define NODE_EP0 DT_NODELABEL(eeprom0)
#define NODE_EP1 DT_NODELABEL(eeprom1)

#define TEST_DATA_SIZE	20
static const uint8_t eeprom_0_data[TEST_DATA_SIZE] = "0123456789abcdefghij";
static const uint8_t eeprom_1_data[TEST_DATA_SIZE] = "jihgfedcba9876543210";
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

static int run_full_read(struct device *i2c, uint8_t addr,
			 const uint8_t *comp_buffer)
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
		return -EIO;
	}

	return 0;
}

static int run_partial_read(struct device *i2c, uint8_t addr,
			    const uint8_t *comp_buffer, unsigned int offset)
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
		return -EIO;
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
			return -EIO;
		}
	}

	return 0;
}

void test_eeprom_slave(void)
{
	const char *label_0 = DT_LABEL(NODE_EP0);
	struct device *eeprom_0 = device_get_binding(label_0);
	struct device *i2c_0 = device_get_binding(DT_BUS_LABEL(NODE_EP0));
	int addr_0 = DT_REG_ADDR(NODE_EP0);
	const char *label_1 = DT_LABEL(NODE_EP1);
	struct device *eeprom_1 = device_get_binding(label_1);
	struct device *i2c_1 = device_get_binding(DT_BUS_LABEL(NODE_EP1));
	int addr_1 = DT_REG_ADDR(NODE_EP1);
	int ret, offset;

	zassert_not_null(i2c_0, "EP0 I2C device %s not found",
			 DT_BUS_LABEL(NODE_EP0));
	zassert_not_null(eeprom_0, "EEPROM device %s not found", label_0);

	LOG_INF("Found EP0 %s on I2C Master device %s at addr %02x",
		label_0, DT_BUS_LABEL(NODE_EP0), addr_0);

	zassert_not_null(i2c_1, "I2C device %s not found",
			 DT_BUS_LABEL(NODE_EP1));
	zassert_not_null(eeprom_1, "EEPROM device %s not found", label_1);

	LOG_INF("Found EP1 %s on I2C Master device %s at addr %02x",
		label_1, DT_BUS_LABEL(NODE_EP1), addr_1);

	/* Program differentiable data into the two devices through a back door
	 * that doesn't use I2C.
	 */
	ret = eeprom_slave_program(eeprom_0, eeprom_0_data, TEST_DATA_SIZE);
	zassert_equal(ret, 0, "Failed to program EEPROM %s", label_0);
	ret = eeprom_slave_program(eeprom_1, eeprom_1_data, TEST_DATA_SIZE);
	zassert_equal(ret, 0, "Failed to program EEPROM %s", label_1);

	/* Attach each EEPROM to its owning bus as a follower device. */
	ret = i2c_slave_driver_register(eeprom_0);
	zassert_equal(ret, 0, "Failed to register EEPROM %s", label_0);
	LOG_INF("EEPROM %s Attached !", label_0);

	ret = i2c_slave_driver_register(eeprom_1);
	zassert_equal(ret, 0, "Failed to register EEPROM %s", label_1);
	LOG_INF("EEPROM %s Attached !", label_1);

	/* The simulated EP0 is configured to be accessed as a follower device
	 * at addr_0 on i2c_0 and should expose eeprom_0_data.  The validation
	 * uses i2c_1 as a bus leader to access this device, which works because
	 * i2c_0 and i2_c have their SDA (SCL) pins shorted (they are on the
	 * same physical bus).  Thus in these calls i2c_1 is a leader device
	 * operating on the follower address addr_0.
	 *
	 * Similarly validation of EP1 uses i2c_0 as a leader with addr_1 and
	 * eeprom_1_data for validation.
	 */
	ret = run_full_read(i2c_1, addr_0, eeprom_0_data);
	zassert_equal(ret, 0,
		     "Full I2C read from EP0 failed");
	ret = run_full_read(i2c_0, addr_1, eeprom_1_data);
	zassert_equal(ret, 0,
		     "Full I2C read from EP1 failed");

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		zassert_equal(0, run_partial_read(i2c_1, addr_0,
			      eeprom_0_data, offset),
			      "Partial I2C read EP0 failed");
		zassert_equal(0, run_partial_read(i2c_0, addr_1,
			      eeprom_1_data, offset),
			      "Partial I2C read EP1 failed");
	}

	for (offset = 0 ; offset < TEST_DATA_SIZE-1 ; ++offset) {
		zassert_equal(0, run_program_read(i2c_1, addr_0, offset),
			      "Program I2C read EP0 failed");
		zassert_equal(0, run_program_read(i2c_0, addr_1, offset),
			      "Program I2C read EP1 failed");
	}

	LOG_INF("Success !");

	/* Detach EEPROM */
	ret = i2c_slave_driver_unregister(eeprom_0);
	zassert_equal(ret, 0, "Failed to unregister EEPROM %s", label_0);
	LOG_INF("EEPROM %s Detached !", label_0);

	ret = i2c_slave_driver_unregister(eeprom_1);
	zassert_equal(ret, 0, "Failed to unregister EEPROM %s", label_1);
	LOG_INF("EEPROM %s Detached !", label_1);
}

void test_main(void)
{
	ztest_test_suite(test_eeprom_slave, ztest_unit_test(test_eeprom_slave));
	ztest_run_test_suite(test_eeprom_slave);
}
