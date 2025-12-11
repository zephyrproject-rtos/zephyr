/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define TEST_KBD_SCAN_NODE DT_INST(0, test_kbd_scan)

/* test driver */

/* Mock data for every valid column. */
static struct {
	kbd_row_t rows[3];
	int col;
	bool detect_mode;
} state;

static void test_drive_column(const struct device *dev, int col)
{
	state.col = col;
}

static kbd_row_t test_read_row(const struct device *dev)
{
	if (state.col == INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE ||
	    state.col == INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL) {
		return 0;
	}

	return state.rows[state.col];
}

static void test_set_detect_mode(const struct device *dev, bool enabled)
{
	TC_PRINT("detect mode: enabled=%d\n", enabled);
	state.detect_mode = enabled;
}

static const struct input_kbd_matrix_api test_api = {
	.drive_column = test_drive_column,
	.read_row = test_read_row,
	.set_detect_mode = test_set_detect_mode,
};

INPUT_KBD_MATRIX_DT_DEFINE(TEST_KBD_SCAN_NODE);

static const struct input_kbd_matrix_common_config
	test_cfg = INPUT_KBD_MATRIX_DT_COMMON_CONFIG_INIT(
			TEST_KBD_SCAN_NODE, &test_api);

static struct input_kbd_matrix_common_data test_data;

DEVICE_DT_DEFINE(TEST_KBD_SCAN_NODE, input_kbd_matrix_common_init, NULL,
		 &test_data, &test_cfg,
		 POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

static const struct device *const test_dev = DEVICE_DT_GET(TEST_KBD_SCAN_NODE);

/* The test only supports a 3 column matrix */
BUILD_ASSERT(DT_PROP(TEST_KBD_SCAN_NODE, col_size) == 3);

/* support stuff */

static const struct device *column_hook_last_dev;
static int column_hook_last_col;

void input_kbd_matrix_drive_column_hook(const struct device *dev, int col)
{
	column_hook_last_dev = dev;
	column_hook_last_col = col;
}

static void state_set_rows_by_column(kbd_row_t c0, kbd_row_t c1, kbd_row_t c2)
{
	memcpy(&state.rows, (kbd_row_t[]){c0, c1, c2}, sizeof(state.rows));
	TC_PRINT("set state [%" PRIkbdrow " %" PRIkbdrow " %" PRIkbdrow "]\n", c0, c1, c2);
}

static struct {
	int row;
	int col;
	int val;
	int event_count;
} test_event_data;

static int last_checked_event_count;

#define assert_no_new_events() \
	zassert_equal(last_checked_event_count, test_event_data.event_count);

#define  assert_new_event(_row, _col, _val) { \
	last_checked_event_count++; \
	zassert_equal(last_checked_event_count, test_event_data.event_count); \
	zassert_equal(_row, test_event_data.row); \
	zassert_equal(_col, test_event_data.col); \
	zassert_equal(_val, test_event_data.val); \
}

static void test_cb(struct input_event *evt, void *user_data)
{
	static int row, col, val;

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
		test_event_data.row = row;
		test_event_data.col = col;
		test_event_data.val = val;
		test_event_data.event_count++;
		TC_PRINT("input event: count=%d row=%d col=%d val=%d\n",
			 test_event_data.event_count, row, col, val);
	}
}
INPUT_CALLBACK_DEFINE(test_dev, test_cb, NULL);

#define WAIT_FOR_IDLE_TIMEOUT_US (5 * USEC_PER_SEC)

static void kbd_scan_wait_for_idle(void)
{
	bool to;

	to = WAIT_FOR(state.detect_mode,
		      WAIT_FOR_IDLE_TIMEOUT_US,
		      k_sleep(K_MSEC(100)));

	zassert_true(to, "timeout waiting for idle state");
}

/* actual tests */

/* no event before debounce time, event after */
ZTEST(kbd_scan, test_kbd_scan)
{
	const struct input_kbd_matrix_common_config *cfg = test_dev->config;

	input_kbd_matrix_poll_start(test_dev);

	state_set_rows_by_column(0x00, BIT(2), 0x00);
	k_sleep(K_USEC(cfg->debounce_down_us / 2));
	assert_no_new_events();

	k_sleep(K_USEC(cfg->debounce_down_us));
	assert_new_event(2, 1, 1);

	state_set_rows_by_column(0x00, 0x00, 0x00);
	k_sleep(K_USEC(cfg->debounce_up_us / 2));
	assert_no_new_events();

	k_sleep(K_USEC(cfg->debounce_up_us));
	assert_new_event(2, 1, 0);

	kbd_scan_wait_for_idle();
	assert_no_new_events();

	zassert_equal(column_hook_last_dev, test_dev);
	zassert_equal(column_hook_last_col, INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL);
}

/* no event for short glitches */
ZTEST(kbd_scan, test_kbd_scan_glitch)
{
	const struct input_kbd_matrix_common_config *cfg = test_dev->config;

	input_kbd_matrix_poll_start(test_dev);

	state_set_rows_by_column(0x00, BIT(2), 0x00);
	k_sleep(K_USEC(cfg->debounce_down_us / 2));
	assert_no_new_events();

	state_set_rows_by_column(0x00, 0x00, 0x00);
	k_sleep(K_USEC(cfg->debounce_down_us));
	assert_no_new_events();

	kbd_scan_wait_for_idle();
	assert_no_new_events();
}

/* very bouncy key delays events indefinitely */
ZTEST(kbd_scan, test_kbd_long_debounce)
{
	const struct input_kbd_matrix_common_config *cfg = test_dev->config;

	input_kbd_matrix_poll_start(test_dev);

	state_set_rows_by_column(0x00, BIT(2), 0x00);
	k_sleep(K_USEC(cfg->debounce_down_us / 2));
	assert_no_new_events();

	for (int i = 0; i < 10; i++) {
		state_set_rows_by_column(0x00, 0x00, 0x00);
		k_sleep(K_USEC(cfg->debounce_down_us / 2));
		assert_no_new_events();

		state_set_rows_by_column(0x00, BIT(2), 0x00);
		k_sleep(K_USEC(cfg->debounce_down_us / 2));
		assert_no_new_events();
	}

	k_sleep(K_USEC(cfg->debounce_down_us));
	assert_new_event(2, 1, 1);

	state_set_rows_by_column(0x00, 0x00, 0x00);
	k_sleep(K_USEC(cfg->debounce_up_us / 2));
	assert_no_new_events();

	for (int i = 0; i < 10; i++) {
		state_set_rows_by_column(0x00, BIT(2), 0x00);
		k_sleep(K_USEC(cfg->debounce_up_us / 2));
		assert_no_new_events();

		state_set_rows_by_column(0x00, 0x00, 0x00);
		k_sleep(K_USEC(cfg->debounce_up_us / 2));
		assert_no_new_events();
	}

	k_sleep(K_USEC(cfg->debounce_up_us));
	assert_new_event(2, 1, 0);

	kbd_scan_wait_for_idle();
	assert_no_new_events();
}

/* ghosting keys should not produce any event */
ZTEST(kbd_scan, test_kbd_ghosting_check)
{
	const struct input_kbd_matrix_common_config *cfg = test_dev->config;

	if (cfg->ghostkey_check == false) {
		ztest_test_skip();
		return;
	}

	input_kbd_matrix_poll_start(test_dev);

	state_set_rows_by_column(BIT(0), 0x00, 0x00);
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(0, 0, 1);

	state_set_rows_by_column(BIT(0), BIT(1), 0x00);
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(1, 1, 1);

	/* ghosting */
	state_set_rows_by_column(BIT(0) | BIT(1), BIT(0) | BIT(1), 0x00);
	k_sleep(K_USEC(cfg->debounce_down_us * 10));
	assert_no_new_events();

	/* back to not ghosting anymore */
	state_set_rows_by_column(BIT(0), BIT(1), 0x00);
	k_sleep(K_USEC(cfg->debounce_down_us * 10));
	assert_no_new_events();

	state_set_rows_by_column(0x00, BIT(1), 0x00);
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(0, 0, 0);

	state_set_rows_by_column(0x00, 0x00, 0x00);
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(1, 1, 0);

	kbd_scan_wait_for_idle();
	assert_no_new_events();
}

/* ghosting keys can be disabled */
ZTEST(kbd_scan, test_kbd_no_ghosting_check)
{
	const struct input_kbd_matrix_common_config *cfg = test_dev->config;

	if (cfg->ghostkey_check == true) {
		ztest_test_skip();
		return;
	}

	input_kbd_matrix_poll_start(test_dev);

	state_set_rows_by_column(BIT(0), 0x00, 0x00);
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(0, 0, 1);

	state_set_rows_by_column(BIT(0), BIT(1), 0x00);
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(1, 1, 1);

	state_set_rows_by_column(BIT(0) | BIT(1), BIT(1), 0x00);
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(1, 0, 1);

	state_set_rows_by_column(BIT(0) | BIT(1), BIT(0) | BIT(1), 0x00);
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(0, 1, 1);

	k_sleep(K_USEC(cfg->debounce_down_us * 10));
	assert_no_new_events();

	state_set_rows_by_column(BIT(1), BIT(0) | BIT(1), 0x00);
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(0, 0, 0);

	state_set_rows_by_column(BIT(1), BIT(0), 0x00);
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(1, 1, 0);

	state_set_rows_by_column(0x00, BIT(0), 0x00);
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(1, 0, 0);

	state_set_rows_by_column(0x00, 0x00, 0x00);
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(0, 1, 0);

	kbd_scan_wait_for_idle();
	assert_no_new_events();
}

/* keymap is applied and can skip ghosting */
ZTEST(kbd_scan, test_kbd_actual_keymap)
{
	const struct input_kbd_matrix_common_config *cfg = test_dev->config;

	if (cfg->actual_key_mask == NULL) {
		ztest_test_skip();
		return;
	}

	input_kbd_matrix_poll_start(test_dev);

	state_set_rows_by_column(BIT(0), 0x00, 0x00);
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(0, 0, 1);

	state_set_rows_by_column(BIT(0), 0x00, BIT(0));
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(0, 2, 1);

	/* ghosting cleared by the keymap */
	state_set_rows_by_column(BIT(0) | BIT(2), 0x00, BIT(0) | BIT(2));
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(2, 0, 1);

	state_set_rows_by_column(BIT(0) | BIT(2), 0x00, BIT(2));
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(0, 2, 0);

	state_set_rows_by_column(BIT(2), 0x00, BIT(2));
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(0, 0, 0);

	state_set_rows_by_column(BIT(2), 0x00, 0x00);
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_no_new_events();

	state_set_rows_by_column(0x00, 0x00, 0x00);
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(2, 0, 0);

	kbd_scan_wait_for_idle();
	assert_no_new_events();
}

ZTEST(kbd_scan, test_kbd_actual_key_map_set)
{
#if CONFIG_INPUT_KBD_ACTUAL_KEY_MASK_DYNAMIC
	kbd_row_t mask[4] = {0x00, 0xff, 0x00, 0x00};
	const struct input_kbd_matrix_common_config cfg = {
		.row_size = 3,
		.col_size = 4,
		.actual_key_mask = mask,
	};
	const struct device fake_dev = {
		.config = &cfg,
	};
	int ret;

	ret = input_kbd_matrix_actual_key_mask_set(&fake_dev, 0, 0, true);
	zassert_equal(ret, 0);
	zassert_equal(mask[0], 0x01);

	ret = input_kbd_matrix_actual_key_mask_set(&fake_dev, 2, 1, false);
	zassert_equal(ret, 0);
	zassert_equal(mask[1], 0xfb);

	ret = input_kbd_matrix_actual_key_mask_set(&fake_dev, 2, 3, true);
	zassert_equal(ret, 0);
	zassert_equal(mask[3], 0x04);

	ret = input_kbd_matrix_actual_key_mask_set(&fake_dev, 3, 0, true);
	zassert_equal(ret, -EINVAL);

	ret = input_kbd_matrix_actual_key_mask_set(&fake_dev, 0, 4, true);
	zassert_equal(ret, -EINVAL);

	zassert_equal(memcmp(mask, (uint8_t[]){0x01, 0xfb, 0x00, 0x04}, 4), 0);
#else
	ztest_test_skip();
#endif
}

static void *kbd_scan_setup(void)
{
	const struct input_kbd_matrix_common_config *cfg = test_dev->config;

	TC_PRINT("actual kbd-matrix timing: poll_period_us=%d "
		 "debounce_down_us=%d debounce_up_us=%d\n",
		 cfg->poll_period_us,
		 cfg->debounce_down_us,
		 cfg->debounce_up_us);

	return NULL;
}

static void kbd_scan_before(void *data)
{
	memset(&state, 0, sizeof(state));
	state.detect_mode = true;

	last_checked_event_count = 0;
	memset(&test_event_data, 0, sizeof(test_event_data));
}

static void kbd_scan_after(void *data)
{
	/* Clear the test data so if a test fails early the testsuite does not
	 * hang indefinitely.
	 */
	state_set_rows_by_column(0x00, 0x00, 0x00);
	kbd_scan_wait_for_idle();
}

ZTEST_SUITE(kbd_scan, NULL, kbd_scan_setup, kbd_scan_before, kbd_scan_after, NULL);
