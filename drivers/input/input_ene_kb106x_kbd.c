/*
 * Copyright (c) 2026 ENE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb106x_kbd

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <soc.h>

LOG_MODULE_REGISTER(input_kb106x_kbd, CONFIG_INPUT_LOG_LEVEL);

#define ROW_SIZE DT_INST_PROP(0, row_size)
#define COL_SIZE DT_INST_PROP(0, col_size)

/* Driver config */
struct kb106x_kbd_config {
	struct input_kbd_matrix_common_config common;
	/* Pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
	uint8_t row_pins[ROW_SIZE];
	uint8_t col_pins[COL_SIZE];
	const struct device *kbd_gpio_dev;
};

struct kbd_gpio_callback {
	struct gpio_callback gpio_cb;
	const struct device *kbd_dev;
};

struct kb106x_kbd_data {
	struct input_kbd_matrix_common_data common;
	bool detect_mode;
	struct kbd_gpio_callback row_cb;
};

INPUT_KBD_STRUCT_CHECK(struct kb106x_kbd_config, struct kb106x_kbd_data);

static void kb106x_kbd_set_detect_mode(const struct device *dev, bool enabled)
{
	const struct kb106x_kbd_config *const config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	struct kb106x_kbd_data *data = dev->data;
	const struct device *kbd_gpio_dev = config->kbd_gpio_dev;
	unsigned int key;

	key = irq_lock();
	data->detect_mode = enabled;
	for (int i = 0; i < common->row_size; i++) {
		uint8_t pin = config->row_pins[i];

		if (enabled) {
			gpio_pin_interrupt_configure(kbd_gpio_dev, pin, GPIO_INT_EDGE_FALLING);
		} else {
			gpio_pin_interrupt_configure(kbd_gpio_dev, pin, GPIO_INT_DISABLE);
		}
	}
	irq_unlock(key);
}

static void kb106x_kbd_drive_column(const struct device *dev, int col)
{
	const struct kb106x_kbd_config *config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	const struct device *kbd_gpio_dev = config->kbd_gpio_dev;
	gpio_port_pins_t set_pins, clear_pins;
	uint32_t col_bits;
	unsigned int key;

	if (col >= common->col_size) {
		LOG_ERR("invalid column: %d", col);
		return;
	}

	if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE) {
		/* Drive all lines to high: key detection is disabled */
		col_bits = ~0;

	} else if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL) {
		/* Drive all lines to low for detection any key press */
		col_bits = ~BIT_MASK(common->col_size);
	} else {
		/*
		 * Drive one line to low for determining which key's state
		 * changed.
		 */
		col_bits = ~BIT(col);
	}

	LOG_DBG("Drive col: %d col_bits: %x", col, col_bits);

	set_pins = clear_pins = 0;
	for (int i = 0; i < common->col_size; i++) {
		int val = col_bits & BIT(i);
		uint8_t pin = config->col_pins[i];

		if (val) {
			set_pins |= BIT(pin);
		} else {
			clear_pins |= BIT(pin);
		}
	}
	key = irq_lock();
	gpio_port_set_bits_raw(kbd_gpio_dev, set_pins);
	gpio_port_clear_bits_raw(kbd_gpio_dev, clear_pins);
	irq_unlock(key);
}

static kbd_row_t kb106x_kbd_read_row(const struct device *dev)
{
	const struct kb106x_kbd_config *config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	const struct device *kbd_gpio_dev = config->kbd_gpio_dev;
	gpio_port_value_t port_value;
	kbd_row_t val = 0;

	gpio_port_get_raw(kbd_gpio_dev, &port_value);
	for (int i = 0; i < common->row_size; i++) {
		uint8_t pin = config->row_pins[i];

		if (port_value & BIT(pin)) {
			val |= BIT(i);
		}
	}

	return (~val & BIT_MASK(common->row_size));
}

static void kb106x_kb_gpio_handler(const struct device *gpio_dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct kbd_gpio_callback *kbd_cb = CONTAINER_OF(cb, struct kbd_gpio_callback, gpio_cb);
	const struct device *dev = kbd_cb->kbd_dev;
	struct kb106x_kbd_data *data = dev->data;

	if (!data->detect_mode) {
		return;
	}
	input_kbd_matrix_poll_start(dev);
}

static int kb106x_kbd_init(const struct device *dev)
{
	const struct kb106x_kbd_config *const config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	const struct device *kbd_gpio_dev = config->kbd_gpio_dev;
	struct kb106x_kbd_data *data = dev->data;
	uint32_t mask;
	uint8_t pin;
	int ret;

	if (common->row_size != ROW_SIZE) {
		LOG_ERR("Unexpected ROW_SIZE: %d != %d", common->row_size, ROW_SIZE);
		return -EINVAL;
	}

	for (int i = 0; i < common->row_size; i++) {
		pin = config->row_pins[i];
		gpio_pin_interrupt_configure(kbd_gpio_dev, pin, GPIO_INT_DISABLE);
		gpio_pin_configure(kbd_gpio_dev, pin, GPIO_INPUT | GPIO_PULL_UP);
	}

	for (int i = 0; i < common->col_size; i++) {
		pin = config->col_pins[i];
		gpio_pin_configure(kbd_gpio_dev, pin, GPIO_OUTPUT_LOW);
	}

	/* Drive all column lines to low for detection any key press */
	kb106x_kbd_drive_column(dev, INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE);

	/* Configure wake-up input and callback for keyboard input signal */
	mask = 0;
	for (int i = 0; i < common->row_size; i++) {
		mask |= BIT(config->row_pins[i]);
	}
	data->row_cb.kbd_dev = dev;
	gpio_init_callback(&data->row_cb.gpio_cb, kb106x_kb_gpio_handler, mask);
	gpio_add_callback(kbd_gpio_dev, &data->row_cb.gpio_cb);

	/* Configure pin-mux for keyboard scan device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("keyboard scan pinctrl setup failed (%d)", ret);
		return ret;
	}

	return input_kbd_matrix_common_init(dev);
}

PINCTRL_DT_INST_DEFINE(0);
INPUT_KBD_MATRIX_DT_INST_DEFINE(0);

static const struct input_kbd_matrix_api kb106x_kbd_api = {
	.drive_column = kb106x_kbd_drive_column,
	.read_row = kb106x_kbd_read_row,
	.set_detect_mode = kb106x_kbd_set_detect_mode,
};

#define GET_PIN_ELEM(node_id, prop, idx) (DT_PROP_BY_IDX(node_id, prop, idx)),

static const struct kb106x_kbd_config kb106x_kbd_cfg_0 = {
	.common = INPUT_KBD_MATRIX_DT_INST_COMMON_CONFIG_INIT(0, &kb106x_kbd_api),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.row_pins = {DT_INST_FOREACH_PROP_ELEM(0, row_pins, GET_PIN_ELEM)},
	.col_pins = {DT_INST_FOREACH_PROP_ELEM(0, col_pins, GET_PIN_ELEM)},
	.kbd_gpio_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, gpio_dev)),
};

static struct kb106x_kbd_data kb106x_kbd_data_0 = {
	.detect_mode = false,
};

PM_DEVICE_DT_INST_DEFINE(0, input_kbd_matrix_pm_action);
DEVICE_DT_INST_DEFINE(0, kb106x_kbd_init, PM_DEVICE_DT_INST_GET(0), &kb106x_kbd_data_0,
		      &kb106x_kbd_cfg_0, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

BUILD_ASSERT(!IS_ENABLED(CONFIG_PM_DEVICE_SYSTEM_MANAGED) || IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME),
	     "CONFIG_PM_DEVICE_RUNTIME must be enabled when using CONFIG_PM_DEVICE_SYSTEM_MANAGED");

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one ene,kb106x-kbd compatible node can be supported");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, row_size), 1, 8), "invalid row-size");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, col_size), 1, 18), "invalid col-size");
