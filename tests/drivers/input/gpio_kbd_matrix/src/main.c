/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define INTERRUPT_NODE DT_NODELABEL(kbd_matrix_interrupt)
#define POLL_NODE DT_NODELABEL(kbd_matrix_poll)
#define SCAN_NODE DT_NODELABEL(kbd_matrix_scan)

static const struct device *dev_interrupt = DEVICE_DT_GET_OR_NULL(INTERRUPT_NODE);
static const struct device *dev_poll = DEVICE_DT_GET_OR_NULL(POLL_NODE);
static const struct device *dev_scan = DEVICE_DT_GET_OR_NULL(SCAN_NODE);

#define INTERRUPT_R0_PIN DT_GPIO_PIN_BY_IDX(INTERRUPT_NODE, row_gpios, 0)
#define INTERRUPT_R1_PIN DT_GPIO_PIN_BY_IDX(INTERRUPT_NODE, row_gpios, 1)
#define INTERRUPT_C0_PIN DT_GPIO_PIN_BY_IDX(INTERRUPT_NODE, col_gpios, 0)
#define INTERRUPT_C1_PIN DT_GPIO_PIN_BY_IDX(INTERRUPT_NODE, col_gpios, 1)

#define POLL_R0_PIN DT_GPIO_PIN_BY_IDX(POLL_NODE, row_gpios, 0)
#define POLL_R1_PIN DT_GPIO_PIN_BY_IDX(POLL_NODE, row_gpios, 1)
#define POLL_C0_PIN DT_GPIO_PIN_BY_IDX(POLL_NODE, col_gpios, 0)
#define POLL_C1_PIN DT_GPIO_PIN_BY_IDX(POLL_NODE, col_gpios, 1)

#define SCAN_R0_PIN DT_GPIO_PIN_BY_IDX(SCAN_NODE, row_gpios, 0)
#define SCAN_R1_PIN DT_GPIO_PIN_BY_IDX(SCAN_NODE, row_gpios, 1)
#define SCAN_C0_PIN DT_GPIO_PIN_BY_IDX(SCAN_NODE, col_gpios, 0)
#define SCAN_C1_PIN DT_GPIO_PIN_BY_IDX(SCAN_NODE, col_gpios, 1)

static const struct device *dev_gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));

enum {
	KBD_DEV_INTERRUPT,
	KBD_DEV_POLL,
	KBD_DEV_SCAN,
};

#define KBD_DEV_COUNT (KBD_DEV_SCAN + 1)

#define COL_COUNT 2

BUILD_ASSERT(DT_PROP_LEN(INTERRUPT_NODE, col_gpios) == COL_COUNT);
BUILD_ASSERT(DT_PROP_LEN(POLL_NODE, col_gpios) == COL_COUNT);
BUILD_ASSERT(DT_PROP_LEN(SCAN_NODE, col_gpios) == COL_COUNT);

static uint8_t test_rows[KBD_DEV_COUNT][COL_COUNT];
static int scan_set_count[KBD_DEV_COUNT];

static void gpio_kbd_scan_set_row(const struct device *dev, uint8_t row)
{
	if (dev == dev_interrupt) {
		gpio_emul_input_set(dev_gpio, INTERRUPT_R0_PIN, !(row & BIT(0)));
		gpio_emul_input_set(dev_gpio, INTERRUPT_R1_PIN, !(row & BIT(1)));
		return;
	} else if (dev == dev_poll) {
		gpio_emul_input_set(dev_gpio, POLL_R0_PIN, !(row & BIT(0)));
		gpio_emul_input_set(dev_gpio, POLL_R1_PIN, !(row & BIT(1)));
		return;
	} else if (dev == dev_scan) {
		gpio_emul_input_set(dev_gpio, SCAN_R0_PIN, !(row & BIT(0)));
		gpio_emul_input_set(dev_gpio, SCAN_R1_PIN, !(row & BIT(1)));
		return;
	}

	TC_PRINT("unknown device: %s\n", dev->name);
}

void input_kbd_matrix_drive_column_hook(const struct device *dev, int col)
{
	gpio_flags_t flags0, flags1;

	if (col >= COL_COUNT) {
		TC_PRINT("invalid column: %d\n", col);
		return;
	}

	if (dev == dev_interrupt) {
		scan_set_count[KBD_DEV_INTERRUPT]++;
		gpio_kbd_scan_set_row(dev, test_rows[KBD_DEV_INTERRUPT][col]);

		/* Verify that columns are NOT driven. */
		gpio_emul_flags_get(dev_gpio, INTERRUPT_C0_PIN, &flags0);
		gpio_emul_flags_get(dev_gpio, INTERRUPT_C1_PIN, &flags1);
		switch (col) {
		case 0:
			zassert_equal(flags0 & GPIO_DIR_MASK, GPIO_OUTPUT);
			zassert_equal(flags1 & GPIO_DIR_MASK, GPIO_INPUT);
			break;
		case 1:
			zassert_equal(flags0 & GPIO_DIR_MASK, GPIO_INPUT);
			zassert_equal(flags1 & GPIO_DIR_MASK, GPIO_OUTPUT);
			break;
		case INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE:
			zassert_equal(flags0 & GPIO_DIR_MASK, GPIO_INPUT);
			zassert_equal(flags1 & GPIO_DIR_MASK, GPIO_INPUT);
			break;
		case INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL:
			zassert_equal(flags0 & GPIO_DIR_MASK, GPIO_OUTPUT);
			zassert_equal(flags1 & GPIO_DIR_MASK, GPIO_OUTPUT);
			break;
		}

		return;
	} else if (dev == dev_poll) {
		scan_set_count[KBD_DEV_POLL]++;
		gpio_kbd_scan_set_row(dev, test_rows[KBD_DEV_POLL][col]);

		/* Verify that columns are always driven */
		gpio_emul_flags_get(dev_gpio, POLL_C0_PIN, &flags0);
		gpio_emul_flags_get(dev_gpio, POLL_C1_PIN, &flags1);
		zassert_equal(flags0 & GPIO_DIR_MASK, GPIO_OUTPUT);
		zassert_equal(flags1 & GPIO_DIR_MASK, GPIO_OUTPUT);

		switch (col) {
		case 0:
			zassert_equal(gpio_emul_output_get(dev_gpio, POLL_C0_PIN), 0);
			zassert_equal(gpio_emul_output_get(dev_gpio, POLL_C1_PIN), 1);
			break;
		case 1:
			zassert_equal(gpio_emul_output_get(dev_gpio, POLL_C0_PIN), 1);
			zassert_equal(gpio_emul_output_get(dev_gpio, POLL_C1_PIN), 0);
			break;
		case INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE:
			zassert_equal(gpio_emul_output_get(dev_gpio, POLL_C0_PIN), 1);
			zassert_equal(gpio_emul_output_get(dev_gpio, POLL_C1_PIN), 1);
			break;
		case INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL:
			zassert_equal(gpio_emul_output_get(dev_gpio, POLL_C0_PIN), 0);
			zassert_equal(gpio_emul_output_get(dev_gpio, POLL_C1_PIN), 0);
			break;
		}

		return;
	} else if (dev == dev_scan) {
		scan_set_count[KBD_DEV_SCAN]++;
		gpio_kbd_scan_set_row(dev, test_rows[KBD_DEV_SCAN][col]);

		/* Verify that columns are always driven */
		gpio_emul_flags_get(dev_gpio, SCAN_C0_PIN, &flags0);
		gpio_emul_flags_get(dev_gpio, SCAN_C1_PIN, &flags1);
		zassert_equal(flags0 & GPIO_DIR_MASK, GPIO_OUTPUT);
		zassert_equal(flags1 & GPIO_DIR_MASK, GPIO_OUTPUT);

		switch (col) {
		case 0:
			zassert_equal(gpio_emul_output_get(dev_gpio, SCAN_C0_PIN), 0);
			zassert_equal(gpio_emul_output_get(dev_gpio, SCAN_C1_PIN), 1);
			break;
		case 1:
			zassert_equal(gpio_emul_output_get(dev_gpio, SCAN_C0_PIN), 1);
			zassert_equal(gpio_emul_output_get(dev_gpio, SCAN_C1_PIN), 0);
			break;
		case INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE:
			zassert_equal(gpio_emul_output_get(dev_gpio, SCAN_C0_PIN), 1);
			zassert_equal(gpio_emul_output_get(dev_gpio, SCAN_C1_PIN), 1);
			break;
		case INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL:
			zassert_equal(gpio_emul_output_get(dev_gpio, SCAN_C0_PIN), 0);
			zassert_equal(gpio_emul_output_get(dev_gpio, SCAN_C1_PIN), 0);
			break;
		}

		return;
	}

	TC_PRINT("unknown device: %s\n", dev->name);
}

/* support stuff */

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

static void test_cb(struct input_event *evt)
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
INPUT_CALLBACK_DEFINE(NULL, test_cb);

/* actual tests */

ZTEST(gpio_kbd_scan, test_gpio_kbd_scan_interrupt)
{
	const struct device *dev = dev_interrupt;

	if (dev == NULL) {
		ztest_test_skip();
		return;
	}

	const struct input_kbd_matrix_common_config *cfg = dev->config;
	uint8_t *rows = test_rows[KBD_DEV_INTERRUPT];
	int *set_count = &scan_set_count[KBD_DEV_INTERRUPT];
	int prev_count;
	gpio_flags_t flags;

	k_sleep(K_SECONDS(1));
	assert_no_new_events();
	zassert_equal(*set_count, 1);

	/* Verify that interrupts are enabled. */
	gpio_emul_flags_get(dev_gpio, INTERRUPT_R0_PIN, &flags);
	zassert_equal(flags & GPIO_INT_ENABLE, GPIO_INT_ENABLE);
	gpio_emul_flags_get(dev_gpio, INTERRUPT_R1_PIN, &flags);
	zassert_equal(flags & GPIO_INT_ENABLE, GPIO_INT_ENABLE);

	rows[0] = BIT(0);
	gpio_kbd_scan_set_row(dev, 0x01);
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(0, 0, 1);

	rows[1] = BIT(1);
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(1, 1, 1);

	rows[0] = 0x00;
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(0, 0, 0);

	rows[1] = 0x00;
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(1, 1, 0);

	k_sleep(K_MSEC(cfg->poll_timeout_ms * 1.5));

	/* Check that scanning is NOT running */
	prev_count = *set_count;
	k_sleep(K_MSEC(cfg->poll_timeout_ms * 10));
	assert_no_new_events();
	TC_PRINT("scan_set_count=%d, prev_count=%d\n", *set_count, prev_count);
	zassert_equal(*set_count, prev_count);

	/* Verify that interrupts are still enabled. */
	gpio_emul_flags_get(dev_gpio, INTERRUPT_R0_PIN, &flags);
	zassert_equal(flags & GPIO_INT_ENABLE, GPIO_INT_ENABLE);
	gpio_emul_flags_get(dev_gpio, INTERRUPT_R1_PIN, &flags);
	zassert_equal(flags & GPIO_INT_ENABLE, GPIO_INT_ENABLE);
}

ZTEST(gpio_kbd_scan, test_gpio_kbd_scan_poll)
{
	const struct device *dev = dev_poll;

	if (dev == NULL) {
		ztest_test_skip();
		return;
	}

	const struct input_kbd_matrix_common_config *cfg = dev->config;
	uint8_t *rows = test_rows[KBD_DEV_POLL];
	int *set_count = &scan_set_count[KBD_DEV_POLL];
	int prev_count;
	gpio_flags_t flags;

	k_sleep(K_SECONDS(1));
	assert_no_new_events();
	zassert_equal(*set_count, 0);

	/* Verify that interrupts are NOT enabled. */
	gpio_emul_flags_get(dev_gpio, POLL_R0_PIN, &flags);
	zassert_equal(flags & GPIO_INT_MASK, 0);
	gpio_emul_flags_get(dev_gpio, POLL_R1_PIN, &flags);
	zassert_equal(flags & GPIO_INT_MASK, 0);

	rows[0] = BIT(0);
	gpio_kbd_scan_set_row(dev, 0x01);
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(0, 0, 1);

	rows[1] = BIT(1);
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(1, 1, 1);

	rows[0] = 0x00;
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(0, 0, 0);

	rows[1] = 0x00;
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(1, 1, 0);

	k_sleep(K_MSEC(cfg->poll_timeout_ms * 1.5));

	/* Check that scanning is NOT running */
	prev_count = *set_count;
	k_sleep(K_MSEC(cfg->poll_timeout_ms * 10));
	assert_no_new_events();
	TC_PRINT("scan_set_count=%d, prev_count=%d\n", *set_count, prev_count);
	zassert_equal(*set_count, prev_count);

	/* Verify that interrupts are still NOT enabled. */
	gpio_emul_flags_get(dev_gpio, POLL_R0_PIN, &flags);
	zassert_equal(flags & GPIO_INT_MASK, 0);
	gpio_emul_flags_get(dev_gpio, POLL_R1_PIN, &flags);
	zassert_equal(flags & GPIO_INT_MASK, 0);
}

ZTEST(gpio_kbd_scan, test_gpio_kbd_scan_scan)
{
	const struct device *dev = dev_scan;

	if (dev == NULL) {
		ztest_test_skip();
		return;
	}

	const struct input_kbd_matrix_common_config *cfg = dev->config;
	uint8_t *rows = test_rows[KBD_DEV_SCAN];
	int *set_count = &scan_set_count[KBD_DEV_SCAN];
	int prev_count;
	int delta_count;
	gpio_flags_t flags;

	/* check that scanning is already running */
	prev_count = *set_count;
	k_sleep(K_SECONDS(1));
	assert_no_new_events();
	delta_count = *set_count - prev_count;
	TC_PRINT("scan_set_count=%d, delta=%d\n", *set_count, delta_count);
	zassert_true(delta_count > 100);

	/* Verify that interrupts are NOT enabled. */
	gpio_emul_flags_get(dev_gpio, SCAN_R0_PIN, &flags);
	zassert_equal(flags & GPIO_INT_MASK, 0);
	gpio_emul_flags_get(dev_gpio, SCAN_R1_PIN, &flags);
	zassert_equal(flags & GPIO_INT_MASK, 0);

	rows[0] = BIT(0);
	gpio_kbd_scan_set_row(dev, 0x01);
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(0, 0, 1);

	rows[1] = BIT(1);
	k_sleep(K_USEC(cfg->debounce_down_us * 1.5));
	assert_new_event(1, 1, 1);

	rows[0] = 0x00;
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(0, 0, 0);

	rows[1] = 0x00;
	k_sleep(K_USEC(cfg->debounce_up_us * 1.5));
	assert_new_event(1, 1, 0);

	k_sleep(K_MSEC(cfg->poll_timeout_ms * 1.5));

	/* Check that scanning is still running */
	prev_count = *set_count;
	k_sleep(K_SECONDS(1));
	assert_no_new_events();
	delta_count = *set_count - prev_count;
	TC_PRINT("scan_set_count=%d, delta=%d\n", *set_count, delta_count);
	zassert_true(delta_count > 100);

	/* Verify that interrupts are still NOT enabled. */
	gpio_emul_flags_get(dev_gpio, SCAN_R0_PIN, &flags);
	zassert_equal(flags & GPIO_INT_MASK, 0);
	gpio_emul_flags_get(dev_gpio, SCAN_R1_PIN, &flags);
	zassert_equal(flags & GPIO_INT_MASK, 0);
}

static void gpio_kbd_scan_before(void *data)
{
	last_checked_event_count = 0;
	memset(&test_event_data, 0, sizeof(test_event_data));
	memset(&scan_set_count, 0, sizeof(scan_set_count));
}

ZTEST_SUITE(gpio_kbd_scan, NULL, NULL, gpio_kbd_scan_before, NULL, NULL);
