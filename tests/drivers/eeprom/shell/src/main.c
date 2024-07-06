/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/eeprom/eeprom_fake.h>
#include <zephyr/fff.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/ztest.h>

#define FAKE_EEPROM_NAME DEVICE_DT_NAME(DT_NODELABEL(fake_eeprom))

/* Global variables */
static const struct device *const fake_eeprom_dev = DEVICE_DT_GET(DT_NODELABEL(fake_eeprom));
static uint8_t data_capture[CONFIG_EEPROM_SHELL_BUFFER_SIZE];
DEFINE_FFF_GLOBALS;

static int eeprom_shell_test_write_capture_data(const struct device *dev, k_off_t offset,
						const void *data, size_t len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(offset);

	zassert_true(len <= sizeof(data_capture));
	memcpy(&data_capture, data, MIN(sizeof(data_capture), len));

	return 0;
}

static int eeprom_shell_test_read_captured_data(const struct device *dev, k_off_t offset,
						void *data, size_t len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(offset);

	zassert_true(len <= sizeof(data_capture));
	memcpy(data, &data_capture, MIN(sizeof(data_capture), len));

	return 0;
}

ZTEST(eeprom_shell, test_eeprom_write)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const uint8_t expected[] = { 0x11, 0x22, 0x33, 0x44, 0xaa, 0xbb, 0xcc, 0xdd };
	int err;

	/* This test relies on the EEPROM shell using a buffer size of at least 8 bytes */
	BUILD_ASSERT(CONFIG_EEPROM_SHELL_BUFFER_SIZE >= 8);

	/* Setup data capture to satisfy EEPROM shell verification read-back */
	fake_eeprom_write_fake.custom_fake = eeprom_shell_test_write_capture_data;
	fake_eeprom_read_fake.custom_fake = eeprom_shell_test_read_captured_data;

	err = shell_execute_cmd(sh, "eeprom write " FAKE_EEPROM_NAME
				" 8 0x11 0x22 0x33 0x44 0xaa 0xbb 0xcc 0xdd");
	zassert_ok(err, "failed to execute shell command (err %d)", err);

	/* The EEPROM shell will write the bytes ... */
	zassert_equal(fake_eeprom_write_fake.call_count, 1);
	zassert_equal(fake_eeprom_write_fake.arg0_val, fake_eeprom_dev);
	zassert_equal(fake_eeprom_write_fake.arg1_val, 8);
	zassert_not_null(fake_eeprom_write_fake.arg2_val);
	zassert_equal(fake_eeprom_write_fake.arg3_val, 8);

	/* ... and verify the written bytes by reading them back */
	zassert_equal(fake_eeprom_read_fake.call_count, 1);
	zassert_equal(fake_eeprom_read_fake.arg0_val, fake_eeprom_dev);
	zassert_equal(fake_eeprom_read_fake.arg1_val, 8);
	zassert_not_null(fake_eeprom_read_fake.arg2_val);
	zassert_equal(fake_eeprom_write_fake.arg3_val, 8);

	/* Verify data values parsed and written correctly */
	zassert_mem_equal(&data_capture, &expected, sizeof(expected));
}

ZTEST(eeprom_shell, test_eeprom_write_failed_verification)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, "eeprom write " FAKE_EEPROM_NAME " 0 0xaa 0x55");
	zassert_true(err < 0, "shell command should have failed (err %d)", err);

	/* The EEPROM shell will write the bytes ... */
	zassert_equal(fake_eeprom_write_fake.call_count, 1);
	zassert_equal(fake_eeprom_write_fake.arg0_val, fake_eeprom_dev);
	zassert_equal(fake_eeprom_write_fake.arg1_val, 0);
	zassert_not_null(fake_eeprom_write_fake.arg2_val);
	zassert_equal(fake_eeprom_write_fake.arg3_val, 2);

	/* ... and attempt to verify the written bytes by reading them back */
	zassert_equal(fake_eeprom_read_fake.call_count, 1);
	zassert_equal(fake_eeprom_read_fake.arg0_val, fake_eeprom_dev);
	zassert_equal(fake_eeprom_read_fake.arg1_val, 0);
	zassert_not_null(fake_eeprom_read_fake.arg2_val);
	zassert_equal(fake_eeprom_write_fake.arg3_val, 2);
}

ZTEST(eeprom_shell, test_eeprom_read)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	/* This test relies on the shell hexdumping 16 bytes per line */
	BUILD_ASSERT(SHELL_HEXDUMP_BYTES_IN_LINE == 16);

	/* The EEPROM shell will split this read into two calls to eeprom_read() */
	err = shell_execute_cmd(sh, "eeprom read " FAKE_EEPROM_NAME " 8 32");
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(fake_eeprom_read_fake.call_count, 2);

	/* 1st read */
	zassert_equal(fake_eeprom_read_fake.arg0_history[0], fake_eeprom_dev);
	zassert_equal(fake_eeprom_read_fake.arg1_history[0], 8);
	zassert_not_null(fake_eeprom_read_fake.arg2_history[0]);
	zassert_equal(fake_eeprom_read_fake.arg3_history[0], SHELL_HEXDUMP_BYTES_IN_LINE);

	/* 2nd read */
	zassert_equal(fake_eeprom_read_fake.arg0_history[1], fake_eeprom_dev);
	zassert_equal(fake_eeprom_read_fake.arg1_history[1], 8 + SHELL_HEXDUMP_BYTES_IN_LINE);
	zassert_not_null(fake_eeprom_read_fake.arg2_history[1]);
	zassert_equal(fake_eeprom_read_fake.arg3_history[1], SHELL_HEXDUMP_BYTES_IN_LINE);
}

ZTEST(eeprom_shell, test_eeprom_size)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, "eeprom size " FAKE_EEPROM_NAME);
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(fake_eeprom_size_fake.call_count, 1);
	zassert_equal(fake_eeprom_size_fake.arg0_val, fake_eeprom_dev);
}

ZTEST(eeprom_shell, test_eeprom_fill)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const uint8_t expected[] = { 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa };
	int err;

	/* This test relies on the EEPROM shell using a buffer size of at least 8 bytes */
	BUILD_ASSERT(CONFIG_EEPROM_SHELL_BUFFER_SIZE >= 8);

	/* Setup data capture to satisfy EEPROM shell verification read-back */
	fake_eeprom_write_fake.custom_fake = eeprom_shell_test_write_capture_data;
	fake_eeprom_read_fake.custom_fake = eeprom_shell_test_read_captured_data;

	err = shell_execute_cmd(sh, "eeprom fill " FAKE_EEPROM_NAME
				" 16 8 0xaa");
	zassert_ok(err, "failed to execute shell command (err %d)", err);

	/* The EEPROM shell will write the bytes ... */
	zassert_equal(fake_eeprom_write_fake.call_count, 1);
	zassert_equal(fake_eeprom_write_fake.arg0_val, fake_eeprom_dev);
	zassert_equal(fake_eeprom_write_fake.arg1_val, 16);
	zassert_not_null(fake_eeprom_write_fake.arg2_val);
	zassert_equal(fake_eeprom_write_fake.arg3_val, 8);

	/* ... and verify the written bytes by reading them back */
	zassert_equal(fake_eeprom_read_fake.call_count, 1);
	zassert_equal(fake_eeprom_read_fake.arg0_val, fake_eeprom_dev);
	zassert_equal(fake_eeprom_read_fake.arg1_val, 16);
	zassert_not_null(fake_eeprom_read_fake.arg2_val);
	zassert_equal(fake_eeprom_write_fake.arg3_val, 8);

	/* Verify data values parsed and written correctly */
	zassert_mem_equal(&data_capture, &expected, sizeof(expected));
}

static void eeprom_shell_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&data_capture, 0, sizeof(data_capture));
}

static void *eeprom_shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	zassert_true(device_is_ready(fake_eeprom_dev));

	/* Verify that the EEPROM size is as expected by the tests */
	zassert_equal(KB(8), eeprom_get_size(fake_eeprom_dev));

	/* Wait for the initialization of the shell dummy backend. */
	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");

	return NULL;
}

ZTEST_SUITE(eeprom_shell, NULL, eeprom_shell_setup, eeprom_shell_before, NULL, NULL);
