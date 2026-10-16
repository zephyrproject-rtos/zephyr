/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/mfd/emul_ite_it8801.h>
#include <zephyr/drivers/mfd/mfd_ite_it8801.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/ztest.h>

#define IT8801_MFD_NODE DT_NODELABEL(it8801_mfd)
#define IT8801_KBD_NODE DT_NODELABEL(it8801_kbd)

#define REG_KSOMCR DT_REG_ADDR_BY_IDX(IT8801_KBD_NODE, 0)
#define REG_KSIDR  DT_REG_ADDR_BY_IDX(IT8801_KBD_NODE, 1)
#define REG_KSIEER DT_REG_ADDR_BY_IDX(IT8801_KBD_NODE, 2)
#define REG_KSIIER DT_REG_ADDR_BY_IDX(IT8801_KBD_NODE, 3)

/* GPIO control register of the pin switched to KSO18 by the kbd mfdctrl */
#define KSO18_GPIOCR_NODE DT_PHANDLE(DT_NODELABEL(kso18_gp01_default), altctrls)
#define REG_KSO18_GPIOCR                                                                           \
	(DT_REG_ADDR(KSO18_GPIOCR_NODE) + DT_PHA(DT_NODELABEL(kso18_gp01_default), altctrls, pin))

static const struct device *const kbd_dev = DEVICE_DT_GET(IT8801_KBD_NODE);
static const struct device *const mfd_dev = DEVICE_DT_GET(IT8801_MFD_NODE);
static const struct emul *const emul = EMUL_DT_GET(IT8801_MFD_NODE);

/* Scan-out line wired to each keyboard column */
static const uint8_t kso_mapping[] = DT_PROP(IT8801_KBD_NODE, kso_mapping);

static struct {
	int row;
	int col;
	int val;
	int count;
} test_event;

static int checked_event_count;

static void test_cb(struct input_event *evt, void *user_data)
{
	static int row, col, val;

	ARG_UNUSED(user_data);

	switch (evt->code) {
	case INPUT_ABS_X:
		col = evt->value;
		break;
	case INPUT_ABS_Y:
		row = evt->value;
		break;
	case INPUT_BTN_TOUCH:
		val = evt->value;
		break;
	}

	if (evt->sync) {
		test_event.row = row;
		test_event.col = col;
		test_event.val = val;
		test_event.count++;
		TC_PRINT("input event: count=%d row=%d col=%d val=%d\n", test_event.count, row,
			 col, val);
	}
}
INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(IT8801_KBD_NODE), test_cb, NULL);

#define assert_no_new_events()                                                                     \
	zassert_equal(checked_event_count, test_event.count, "unexpected input event")

#define assert_new_event(_row, _col, _val)                                                         \
	do {                                                                                       \
		checked_event_count++;                                                             \
		zassert_equal(checked_event_count, test_event.count, "missing input event");       \
		zassert_equal(test_event.row, (_row), "row");                                      \
		zassert_equal(test_event.col, (_col), "col");                                      \
		zassert_equal(test_event.val, (_val), "val");                                      \
	} while (false)

static const struct input_kbd_matrix_common_config *kbd_cfg(void)
{
	return kbd_dev->config;
}

static uint8_t emul_reg(uint8_t reg)
{
	uint8_t val = 0;

	zassert_ok(it8801_emul_get_reg(emul, reg, &val));
	return val;
}

static void set_key(int row, int col, bool pressed)
{
	zassert_ok(it8801_emul_set_key(emul, kso_mapping[col], row, pressed));
}

/* Time between matrix scans: the poll period rounded up to whole ticks. */
static uint32_t scan_period_us(void)
{
	return k_ticks_to_us_ceil32(k_us_to_ticks_ceil32(kbd_cfg()->poll_period_us));
}

/*
 * A key change is picked up by the next scan and reported by the first scan
 * after the debounce time, so allow two scan periods on top of it.
 */
static void wait_debounce_down(void)
{
	k_sleep(K_USEC(kbd_cfg()->debounce_down_us + 2 * scan_period_us()));
}

static void wait_debounce_up(void)
{
	k_sleep(K_USEC(kbd_cfg()->debounce_up_us + 2 * scan_period_us()));
}

/* Wait for the scan thread to time out and fall back to detect mode. */
static void wait_poll_timeout(void)
{
	k_sleep(K_MSEC(kbd_cfg()->poll_timeout_ms * 3 / 2));
}

static void assert_detect_mode(void)
{
	zassert_equal(it8801_emul_get_driven_column(emul, REG_KSOMCR), IT8801_EMUL_COLUMN_ALL,
		      "detect mode should drive all columns");
	zassert_equal(emul_reg(REG_KSIIER), 0xFF, "detect mode should enable KSI interrupts");
	zassert_equal(emul_reg(REG_KSIEER), 0x00, "no KSI edge event should be pending");
}

/* Let a device whose deferred init failed be initialized again. */
static void reset_failed_init(const struct device *dev)
{
	if (dev->state->initialized && dev->state->init_res != 0) {
		dev->state->initialized = false;
		dev->state->init_res = 0;
	}
}

/*
 * Initialization errors. Both the MFD and the keyboard use deferred init;
 * every case fails before the keyboard scan thread is created, so a failed
 * init can be retried. The MFD is left initialized for the kbd_scan suite.
 */

static void init_before(void *fixture)
{
	ARG_UNUSED(fixture);

	it8801_emul_reset(emul);
}

static void init_after(void *fixture)
{
	ARG_UNUSED(fixture);

	it8801_emul_set_read_fail(emul, 0, 0);
	it8801_emul_set_write_fail(emul, 0, 0);
	reset_failed_init(kbd_dev);
	reset_failed_init(mfd_dev);
}

/* Runs before test_init_reg_write_fail, while the MFD is still uninitialized. */
ZTEST(kbd_init, test_init_mfd_not_ready)
{
	zassert_ok(it8801_emul_set_reg(emul, IT8801_REG_HBVIDR, 0x00));

	zassert_equal(device_init(mfd_dev), -ENODEV, "MFD should reject a wrong vendor ID");
	zassert_equal(device_init(kbd_dev), -ENODEV, "kbd should fail without its MFD parent");
}

ZTEST(kbd_init, test_init_reg_write_fail)
{
	/* Registers the keyboard init writes, each fatal when the write fails */
	static const uint8_t regs[] = {
		REG_KSO18_GPIOCR,
		REG_KSOMCR,
		IT8801_REG_GIECR,
		IT8801_REG_SMBCR,
	};

	zassert_ok(device_init(mfd_dev));

	for (size_t i = 0; i < ARRAY_SIZE(regs); i++) {
		it8801_emul_set_write_fail(emul, regs[i], IT8801_EMUL_FAIL_ALWAYS);

		zassert_equal(device_init(kbd_dev), -EIO,
			      "kbd init should fail on a write error to reg 0x%02x", regs[i]);

		it8801_emul_set_write_fail(emul, regs[i], 0);
		reset_failed_init(kbd_dev);
	}
}

ZTEST_SUITE(kbd_init, NULL, NULL, init_before, init_after, NULL);

/*
 * Key scanning. The keyboard is initialized once; each case starts and ends
 * with every key released and the scan thread back in detect mode.
 */

static void *scan_setup(void)
{
	if (!device_is_ready(mfd_dev)) {
		zassert_ok(device_init(mfd_dev));
	}
	zassert_ok(device_init(kbd_dev));

	/* Let the scan thread arm detect mode. */
	k_sleep(K_MSEC(1));

	return NULL;
}

static void scan_before(void *fixture)
{
	ARG_UNUSED(fixture);

	checked_event_count = test_event.count;
}

static void scan_after(void *fixture)
{
	ARG_UNUSED(fixture);

	it8801_emul_set_read_fail(emul, 0, 0);
	it8801_emul_set_write_fail(emul, 0, 0);
	wait_poll_timeout();
}

static void press_key(int row, int col)
{
	set_key(row, col, true);
	wait_debounce_down();
	assert_new_event(row, col, 1);
}

static void release_key(int row, int col)
{
	set_key(row, col, false);
	wait_debounce_up();
	assert_new_event(row, col, 0);
}

ZTEST(kbd_scan, test_init_state)
{
	zassert_equal(emul_reg(REG_KSO18_GPIOCR) & GENMASK(7, 6), IT8801_GPIOAFS_FUN2 << 6,
		      "mfdctrl pin should be switched to its KSO function");
	zassert_equal(emul_reg(IT8801_REG_GIECR), IT8801_REG_MASK_GKSIIE,
		      "gather KSI interrupt should be enabled");
	zassert_equal(emul_reg(IT8801_REG_SMBCR), IT8801_REG_MASK_ARE,
		      "SMBus alert response should be enabled");
	assert_detect_mode();
	assert_no_new_events();
}

ZTEST(kbd_scan, test_key_press_release)
{
	set_key(3, 1, true);
	wait_debounce_down();
	assert_new_event(3, 1, 1);

	set_key(3, 1, false);
	wait_debounce_up();
	assert_new_event(3, 1, 0);

	wait_poll_timeout();
	assert_no_new_events();
}

ZTEST(kbd_scan, test_multiple_keys)
{
	set_key(0, 0, true);
	wait_debounce_down();
	assert_new_event(0, 0, 1);

	set_key(5, 2, true);
	wait_debounce_down();
	assert_new_event(5, 2, 1);

	set_key(0, 0, false);
	wait_debounce_up();
	assert_new_event(0, 0, 0);

	set_key(5, 2, false);
	wait_debounce_up();
	assert_new_event(5, 2, 0);

	wait_poll_timeout();
	assert_no_new_events();
}

ZTEST(kbd_scan, test_kso_mapping)
{
	/* A key on KSO line kso_mapping[col] must be reported in column col. */
	for (int col = 0; col < (int)ARRAY_SIZE(kso_mapping); col++) {
		set_key(col, col, true);
		wait_debounce_down();
		assert_new_event(col, col, 1);

		set_key(col, col, false);
		wait_debounce_up();
		assert_new_event(col, col, 0);
	}

	wait_poll_timeout();
	assert_no_new_events();
}

ZTEST(kbd_scan, test_detect_mode_rearmed)
{
	set_key(7, 0, true);
	wait_debounce_down();
	assert_new_event(7, 0, 1);

	/* Scanning: the alert was acknowledged and KSI interrupts are off. */
	zassert_equal(emul_reg(REG_KSIEER), 0x00, "alert handler should clear KSI events");
	zassert_equal(emul_reg(REG_KSIIER), 0x00, "KSI interrupts should be off while polling");

	set_key(7, 0, false);
	wait_debounce_up();
	assert_new_event(7, 0, 0);

	wait_poll_timeout();
	assert_detect_mode();

	/* A new press must be picked up through the interrupt again. */
	set_key(7, 0, true);
	wait_debounce_down();
	assert_new_event(7, 0, 1);

	set_key(7, 0, false);
	wait_debounce_up();
	assert_new_event(7, 0, 0);
}

/*
 * I2C failures. Each case fails a single register access at a known point
 * of the scan cycle and checks the events reported around it.
 */

ZTEST(kbd_scan, test_fault_detect_disable)
{
	/* Fail the write that disables KSI interrupts when scanning starts. */
	it8801_emul_set_write_fail(emul, REG_KSIIER, 1);

	press_key(2, 1);
	release_key(2, 1);

	/*
	 * With KSI interrupts left enabled the column scan itself raises
	 * alerts, which queue one more polling round before detect mode.
	 */
	wait_poll_timeout();
	wait_poll_timeout();
	assert_detect_mode();
	assert_no_new_events();
}

ZTEST(kbd_scan, test_fault_drive_column)
{
	press_key(4, 2);

	/* One column select fails mid-scan; debouncing hides the misread. */
	it8801_emul_set_write_fail(emul, REG_KSOMCR, 1);
	wait_debounce_up();
	assert_no_new_events();

	release_key(4, 2);
}

ZTEST(kbd_scan, test_fault_read_row)
{
	press_key(6, 0);

	/* One row read fails mid-scan and reads as released; debouncing hides it. */
	it8801_emul_set_read_fail(emul, REG_KSIDR, 1);
	wait_debounce_up();
	assert_no_new_events();

	release_key(6, 0);
}

ZTEST(kbd_scan, test_fault_alert_status_read)
{
	/* A failed status read still acknowledges the alert and starts a scan. */
	it8801_emul_set_read_fail(emul, REG_KSIEER, 1);

	press_key(1, 2);
	zassert_equal(emul_reg(REG_KSIEER), 0x00, "alert should be acknowledged");

	release_key(1, 2);
}

ZTEST(kbd_scan, test_fault_alert_clear)
{
	/* A failed acknowledge still starts a scan. */
	it8801_emul_set_write_fail(emul, REG_KSIEER, 1);

	press_key(5, 1);
	zassert_not_equal(emul_reg(REG_KSIEER), 0x00, "KSI edge event should stay pending");

	release_key(5, 1);

	/* Re-arming detect mode clears the pending event. */
	wait_poll_timeout();
	assert_detect_mode();
}

/* Fail the write to @p reg that re-arms detect mode after a poll timeout. */
static void check_rearm_fault(uint8_t reg)
{
	press_key(1, 0);
	it8801_emul_set_write_fail(emul, reg, 1);
	release_key(1, 0);

	wait_poll_timeout();
	zassert_equal(emul_reg(REG_KSIIER), 0x00, "detect mode should not be armed");

	/* Without detect mode a key press goes unnoticed. */
	set_key(1, 0, true);
	wait_debounce_down();
	assert_no_new_events();

	/* A PM suspend and resume restarts scanning. */
	zassert_ok(pm_device_action_run(kbd_dev, PM_DEVICE_ACTION_SUSPEND));
	zassert_ok(pm_device_action_run(kbd_dev, PM_DEVICE_ACTION_RESUME));
	wait_debounce_down();
	assert_new_event(1, 0, 1);

	release_key(1, 0);
	wait_poll_timeout();
	assert_detect_mode();
}

ZTEST(kbd_scan, test_fault_rearm_clear)
{
	check_rearm_fault(REG_KSIEER);
}

ZTEST(kbd_scan, test_fault_rearm_enable)
{
	check_rearm_fault(REG_KSIIER);
}

ZTEST_SUITE(kbd_scan, NULL, scan_setup, scan_before, scan_after, NULL);

void test_main(void)
{
	/* kbd_init needs the MFD and keyboard still uninitialized. */
	ztest_run_test_suite(kbd_init, false, 1, 1, NULL);
	ztest_run_test_suite(kbd_scan, false, 1, 1, NULL);
}
