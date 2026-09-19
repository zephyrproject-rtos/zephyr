/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/emul_ite_it8801.h>
#include <zephyr/drivers/mfd/mfd_ite_it8801.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/ztest.h>

#define IT8801_MFD_NODE DT_NODELABEL(it8801_mfd)
#define IT8801_KBD_NODE DT_NODELABEL(it8801_kbd)

/* Register addresses from DT reg property */
#define REG_KSOMCR 0x40
#define REG_KSIDR  0x41
#define REG_KSIEER 0x42
#define REG_KSIIER 0x43

/* Test helpers from stubs.c */
extern struct it8801_mfd_callback *test_get_registered_callback(void);
extern void test_set_configure_pins_retval(int retval);
extern int test_get_configure_pins_call_count(void);
extern void test_reset_stubs(void);

static const struct device *kbd_dev = DEVICE_DT_GET(IT8801_KBD_NODE);
static const struct device *mfd_dev = DEVICE_DT_GET(IT8801_MFD_NODE);
static const struct emul *emul = EMUL_DT_GET(IT8801_MFD_NODE);

static const struct input_kbd_matrix_api *get_api(void)
{
	const struct input_kbd_matrix_common_config *cfg = kbd_dev->config;

	return cfg->api;
}

static uint8_t emul_reg(uint8_t reg)
{
	uint8_t val = 0;

	it8801_emul_get_reg(emul, reg, &val);
	return val;
}

static void reset_emul(void)
{
	it8801_emul_reset(emul);
}

static void reset_device_state(void)
{
	kbd_dev->state->initialized = false;
	kbd_dev->state->init_res = 0;
}

/*
 * Initialization error tests: each test causes init to fail before the
 * scanning thread is created, so device state can be safely reset between
 * runs.
 */

static void init_before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_emul();
	reset_device_state();
	test_reset_stubs();
}

ZTEST(init_error, test_init_mfd_not_ready)
{
	uint8_t saved = mfd_dev->state->init_res;

	mfd_dev->state->init_res = 1;

	int ret = device_init(kbd_dev);

	zassert_equal(ret, -ENODEV,
		      "Expected -ENODEV when MFD not ready, got %d", ret);

	mfd_dev->state->init_res = saved;
}

ZTEST(init_error, test_init_configure_pins_fail)
{
	test_set_configure_pins_retval(-EIO);

	int ret = device_init(kbd_dev);

	zassert_equal(ret, -EIO,
		      "Expected -EIO from configure_pins failure, got %d", ret);
}

ZTEST(init_error, test_init_configures_mfdctrl_pins)
{
	test_set_configure_pins_retval(0);

	int ret = device_init(kbd_dev);

	zassert_equal(ret, 0, "Init should succeed when pins configure");
	zassert_equal(test_get_configure_pins_call_count(), 1,
		      "Expected 1 pin configuration, got %d",
		      test_get_configure_pins_call_count());
}

ZTEST(init_error, test_init_ksomcr_write_fail)
{
	it8801_emul_set_write_fail_reg(emul, REG_KSOMCR);

	int ret = device_init(kbd_dev);

	zassert_not_equal(ret, 0, "Init should fail when KSOMCR write fails");
}

ZTEST(init_error, test_init_giecr_write_fail)
{
	it8801_emul_set_write_fail_reg(emul, IT8801_REG_GIECR);

	int ret = device_init(kbd_dev);

	zassert_not_equal(ret, 0, "Init should fail when GIECR write fails");
}

ZTEST(init_error, test_init_smbcr_write_fail)
{
	it8801_emul_set_write_fail_reg(emul, IT8801_REG_SMBCR);

	int ret = device_init(kbd_dev);

	zassert_not_equal(ret, 0, "Init should fail when SMBCR write fails");
}

ZTEST(init_error, test_init_ksiier_disable_fail)
{
	it8801_emul_set_write_fail_reg(emul, REG_KSIIER);

	int ret = device_init(kbd_dev);

	/* set_detect_mode errors during init are logged but not fatal */
	zassert_equal(ret, 0,
		      "Init should succeed even if KSIIER disable fails");
}

ZTEST_SUITE(init_error, NULL, NULL, init_before, NULL, NULL);

/*
 * API tests: suite setup performs one successful device_init (which creates
 * the kbd_matrix scanning thread). Individual tests exercise API functions
 * by controlling emulator state.
 */

struct api_fixture {
	const struct input_kbd_matrix_api *api;
	uint8_t init_giecr;
	uint8_t init_smbcr;
	int init_column;
};

static void *api_suite_setup(void)
{
	static struct api_fixture f;

	reset_emul();
	reset_device_state();
	test_reset_stubs();

	int ret = device_init(kbd_dev);

	zassert_equal(ret, 0, "API suite: device_init failed (%d)", ret);

	/*
	 * Capture the hardware state the driver's init routine configured
	 * before api_before() clears the emulator for the per-test cases.
	 * GIECR and SMBCR are written only by init, and both init and the
	 * scanning thread drive all columns, so these are stable here.
	 */
	f.init_giecr = emul_reg(IT8801_REG_GIECR);
	f.init_smbcr = emul_reg(IT8801_REG_SMBCR);
	f.init_column = it8801_emul_get_driven_column(emul, REG_KSOMCR);

	f.api = get_api();
	return &f;
}

static void api_before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_emul();
}

ZTEST_F(api, test_init_success_results)
{
	zassert_not_null(test_get_registered_callback(),
			 "Callback should be registered after init");

	/* Assert on the hardware state the driver's init routine left,
	 * captured in api_suite_setup before the emulator was cleared.
	 */
	zassert_equal(fixture->init_giecr, IT8801_REG_MASK_GKSIIE,
		      "init should enable the gather KSI interrupt");
	zassert_equal(fixture->init_smbcr, IT8801_REG_MASK_ARE,
		      "init should enable the SMBus alert response");
	zassert_equal(fixture->init_column, IT8801_EMUL_COLUMN_ALL,
		      "init should drive all keyboard columns");
}

/* drive_column tests */

ZTEST_F(api, test_drive_column_none)
{
	fixture->api->drive_column(kbd_dev, INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE);

	zassert_equal(it8801_emul_get_driven_column(emul, REG_KSOMCR),
		      IT8801_EMUL_COLUMN_NONE,
		      "DRIVE_NONE should leave no column driven");
}

ZTEST_F(api, test_drive_column_all)
{
	fixture->api->drive_column(kbd_dev, INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL);

	zassert_equal(it8801_emul_get_driven_column(emul, REG_KSOMCR),
		      IT8801_EMUL_COLUMN_ALL,
		      "DRIVE_ALL should drive every column");
}

ZTEST_F(api, test_drive_column_specific)
{
	/* Overlay has col-size=3 with kso-mapping = <0x00 0x01 0x02>: driving
	 * column N should drive the scan-out line kso-mapping[N].
	 */
	static const uint8_t kso_mapping[] = {0x00, 0x01, 0x02};

	for (size_t col = 0; col < ARRAY_SIZE(kso_mapping); col++) {
		fixture->api->drive_column(kbd_dev, (int)col);

		zassert_equal(it8801_emul_get_driven_column(emul, REG_KSOMCR),
			      kso_mapping[col],
			      "Column %zu should drive KSO line 0x%02x",
			      col, kso_mapping[col]);
	}
}

ZTEST_F(api, test_drive_column_i2c_fail)
{
	it8801_emul_set_write_fail_reg(emul, REG_KSOMCR);

	/* Should not crash, just log error */
	fixture->api->drive_column(kbd_dev, INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL);
}

/* read_row tests: hardware is active-low, driver inverts */

ZTEST_F(api, test_read_row_single_key)
{
	it8801_emul_set_reg(emul, REG_KSIDR, 0xFE);

	kbd_row_t row = fixture->api->read_row(kbd_dev);

	zassert_equal(row, 0x01, "Expected 0x01 (inverted 0xFE), got 0x%02x",
		      row);
}

ZTEST_F(api, test_read_row_all_pressed)
{
	it8801_emul_set_reg(emul, REG_KSIDR, 0x00);

	kbd_row_t row = fixture->api->read_row(kbd_dev);

	zassert_equal(row, 0xFF, "Expected 0xFF (all keys pressed), got 0x%02x",
		      row);
}

ZTEST_F(api, test_read_row_none_pressed)
{
	it8801_emul_set_reg(emul, REG_KSIDR, 0xFF);

	kbd_row_t row = fixture->api->read_row(kbd_dev);

	zassert_equal(row, 0x00, "Expected 0x00 (no keys pressed), got 0x%02x",
		      row);
}

ZTEST_F(api, test_read_row_pattern_alternating)
{
	it8801_emul_set_reg(emul, REG_KSIDR, 0xA5);

	kbd_row_t row = fixture->api->read_row(kbd_dev);

	zassert_equal(row, 0x5A, "Expected 0x5A (inverted 0xA5), got 0x%02x",
		      row);
}

ZTEST_F(api, test_read_row_pattern_grouped)
{
	it8801_emul_set_reg(emul, REG_KSIDR, 0x3C);

	kbd_row_t row = fixture->api->read_row(kbd_dev);

	zassert_equal(row, 0xC3, "Expected 0xC3 (inverted 0x3C), got 0x%02x",
		      row);
}

ZTEST_F(api, test_read_row_i2c_fail)
{
	it8801_emul_set_read_fail_reg(emul, REG_KSIDR);

	/* Should not crash, driver logs error and returns inverted value */
	kbd_row_t row = fixture->api->read_row(kbd_dev);

	ARG_UNUSED(row);
}

/* set_detect_mode tests */

ZTEST_F(api, test_set_detect_mode_enable)
{
	fixture->api->set_detect_mode(kbd_dev, true);

	zassert_equal(emul_reg(REG_KSIEER), 0xFF,
		      "KSIEER should be 0xFF after enabling (clear pending)");
	zassert_equal(emul_reg(REG_KSIIER), 0xFF,
		      "KSIIER should be 0xFF after enabling");
}

ZTEST_F(api, test_set_detect_mode_disable)
{
	it8801_emul_set_reg(emul, REG_KSIIER, 0xFF);

	fixture->api->set_detect_mode(kbd_dev, false);

	zassert_equal(emul_reg(REG_KSIIER), 0x00,
		      "KSIIER should be 0x00 after disabling");
}

ZTEST_F(api, test_set_detect_mode_enable_clear_fail)
{
	it8801_emul_set_write_fail_reg(emul, REG_KSIEER);

	fixture->api->set_detect_mode(kbd_dev, true);

	/* Early return: KSIIER should not be written */
	zassert_equal(emul_reg(REG_KSIIER), 0x00,
		      "KSIIER should remain unchanged after KSIEER failure");
}

ZTEST_F(api, test_set_detect_mode_enable_ksiier_fail)
{
	it8801_emul_set_write_fail_reg(emul, REG_KSIIER);

	fixture->api->set_detect_mode(kbd_dev, true);

	/* KSIEER write should succeed before KSIIER failure */
	zassert_equal(emul_reg(REG_KSIEER), 0xFF,
		      "KSIEER should be written before KSIIER failure");
}

ZTEST_F(api, test_set_detect_mode_disable_fail)
{
	it8801_emul_set_write_fail_reg(emul, REG_KSIIER);

	/* Should not crash, just log error */
	fixture->api->set_detect_mode(kbd_dev, false);
}

/* alert_handler tests: invoked via the registered MFD callback */

ZTEST_F(api, test_alert_handler_no_interrupt)
{
	struct it8801_mfd_callback *cb = test_get_registered_callback();

	zassert_not_null(cb, "Callback should be registered");
	zassert_not_null(cb->cb, "Callback function should not be NULL");

	it8801_emul_set_reg(emul, REG_KSIEER, 0x00);

	cb->cb(kbd_dev);

	zassert_equal(emul_reg(REG_KSIEER), 0x00,
		      "KSIEER should remain 0x00 when no interrupt pending");
}

ZTEST_F(api, test_alert_handler_with_interrupt)
{
	struct it8801_mfd_callback *cb = test_get_registered_callback();

	zassert_not_null(cb, "Callback should be registered");

	it8801_emul_set_reg(emul, REG_KSIEER, 0x01);

	cb->cb(kbd_dev);

	zassert_equal(emul_reg(REG_KSIEER), 0xFF,
		      "KSIEER should be 0xFF after interrupt clear");
}

ZTEST_F(api, test_alert_handler_multiple_interrupts)
{
	struct it8801_mfd_callback *cb = test_get_registered_callback();

	zassert_not_null(cb, "Callback should be registered");

	it8801_emul_set_reg(emul, REG_KSIEER, 0x87);

	cb->cb(kbd_dev);

	zassert_equal(emul_reg(REG_KSIEER), 0xFF,
		      "KSIEER should be 0xFF to clear all interrupts");
}

ZTEST_F(api, test_alert_handler_read_fail)
{
	struct it8801_mfd_callback *cb = test_get_registered_callback();

	zassert_not_null(cb, "Callback should be registered");

	it8801_emul_set_read_fail_reg(emul, REG_KSIEER);

	/* Should not crash */
	cb->cb(kbd_dev);
}

ZTEST_F(api, test_alert_handler_clear_fail)
{
	struct it8801_mfd_callback *cb = test_get_registered_callback();

	zassert_not_null(cb, "Callback should be registered");

	it8801_emul_set_reg(emul, REG_KSIEER, 0x01);
	it8801_emul_set_write_fail_reg(emul, REG_KSIEER);

	/* Should log error but still start polling */
	cb->cb(kbd_dev);
}

ZTEST_SUITE(api, NULL, api_suite_setup, api_before, NULL, NULL);
