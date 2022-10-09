/*
 * Copyright (c) 2022, tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT keyboard_gpio

/**
 * @file
 * @brief Keyboard-gpio driver
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/input.h>
#include <zephyr/dt-bindings/input/input.h>

#include "../input_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(keyboard_gpio);

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#define SCAN_INTERVAL	CONFIG_KEYBOARD_GPIO_SCAN_INTERVAL
#define TIME_DEBOUNCE	CONFIG_KEYBOARD_GPIO_TIME_DEBOUNCE
#define TIME_LONGPRESS	CONFIG_KEYBOARD_GPIO_TIME_LONGPRESS
#define TIME_HOLDPRESS  CONFIG_KEYBOARD_GPIO_TIME_HOLDPRESS

/**
 * @brief Key information structure
 *
 * This structure gathers useful information about keyboard controller.
 *
 * @param label Key label.
 * @param code  Key code.
 * @param debounce_ms  Key debounce time
 * @param longpress_ms Key long press time
 * @param holdpress_ms Key hold press time
 */
struct key_info_dt_spec {
	const char *label;
	uint16_t code;
	uint16_t debounce_ms;
	uint16_t longpress_ms;
	uint16_t holdpress_ms;
};

/**
 * @brief Get KEY INFO DT SPEC
 *
 * @param node_id devicetree node identifier
 */
#define KEY_INFO_DT_SPEC_GET(node_id) \
{ \
	.label = DT_PROP(node_id, label), \
	.code = DT_PROP(node_id, code), \
	.debounce_ms = DT_PROP_OR(node_id, debounce_ms, 0), \
	.longpress_ms = DT_PROP_OR(node_id, longpress_ms, 0), \
	.holdpress_ms = DT_PROP_OR(node_id, holdpress_ms, 0), \
}

enum key_state_t {
	KEY_STATE_NONE,
	KEY_STATE_PRESSED,
	KEY_STATE_LONG_PRESSED,
	KEY_STAT_MAXNBR
};

enum key_event_t {
	KEY_EVENT_RELEASE = KEY_RELEASE,
	KEY_EVENT_PRESSED = KEY_PRESSED,
	KEY_EVENT_LONG_PRESSED = KEY_LONG_PRESSED,
	KEY_EVENT_HOLD_PRESSED = KEY_HOLD_PRESSED,
	KEY_EVENT_LONG_RELEASE = KEY_LONG_RELEASE,
	KEY_EVENT_NONE = 0xff,
};

struct kbd_gpio_driver {
	struct gpio_callback gpio_data;

	void *pdata;
	bool pressed;
	enum key_state_t state;
	enum key_event_t event;

	uint32_t count_debounce;
	uint32_t count_cycle;
};

struct kbd_gpio_data {
	const struct device *dev;
	struct input_dev *input;
	struct k_work_delayable delayed_work;
	struct kbd_gpio_driver *driver;
};

struct kbd_gpio_config {
	uint8_t num_keys;
	const struct gpio_dt_spec *gpio;
	const struct key_info_dt_spec *info;
};

#define MS_TO_CYCLE(ms, default) \
	(((ms) ? (ms) : (default)) / SCAN_INTERVAL)
#define KEY_EVENT_CODE(code, default) \
	((code != KEY_CODE_RESERVED) ? code : default)
#define KEY_EVENT_CALL(input, code, event) \
{ \
	input_report_key(input, code, event); \
}

static bool kbd_gpio_one_key_proc(const struct device *dev, int idx)
{
	const struct kbd_gpio_config *config = dev->config;
	struct kbd_gpio_data *data = dev->data;
	struct input_dev *input = data->input;

	const struct gpio_dt_spec *gpio = &config->gpio[idx];
	const struct key_info_dt_spec *info = &config->info[idx];
	struct kbd_gpio_driver *driver = &data->driver[idx];

	bool pressed = gpio_pin_get(gpio->port, gpio->pin);

	if (driver->state != KEY_STATE_NONE) {
		driver->count_cycle++;
	}

	if (driver->pressed != pressed) {
		if (driver->count_debounce++ >= \
			MS_TO_CYCLE(info->debounce_ms, TIME_DEBOUNCE)) {
			driver->pressed = pressed;
			driver->count_debounce = 0;
		}
	} else {
		driver->count_debounce = 0;
	}

	switch (driver->state) {
	case KEY_STATE_NONE:
		if (driver->pressed == true) {
			driver->event = KEY_EVENT_PRESSED;
			driver->state = KEY_STATE_PRESSED;
			driver->count_cycle = 0;
			KEY_EVENT_CALL(input, \
				KEY_EVENT_CODE(info->code, (idx + 1)), driver->event);
		} else {
			driver->event = KEY_EVENT_NONE;
		}
		break;

	case KEY_STATE_PRESSED:
		if (driver->pressed == false) {
			driver->event = KEY_EVENT_RELEASE;
			driver->state = KEY_STATE_NONE;
			KEY_EVENT_CALL(input, \
				KEY_EVENT_CODE(info->code, (idx + 1)), driver->event);
		} else if (MS_TO_CYCLE(info->longpress_ms, TIME_LONGPRESS) > 0 &&
					driver->count_cycle >= \
					MS_TO_CYCLE(info->longpress_ms, TIME_LONGPRESS)) {
			driver->event = KEY_EVENT_LONG_PRESSED;
			driver->state = KEY_STATE_LONG_PRESSED;
			driver->count_cycle = 0;
			KEY_EVENT_CALL(input, \
				KEY_EVENT_CODE(info->code, (idx + 1)), driver->event);
		}
		break;

	case KEY_STATE_LONG_PRESSED:
		if (driver->pressed == true) {
			if (MS_TO_CYCLE(info->holdpress_ms, TIME_HOLDPRESS) > 0 &&
				driver->count_cycle >= \
					MS_TO_CYCLE(info->holdpress_ms, TIME_HOLDPRESS)) {
				driver->event = KEY_EVENT_HOLD_PRESSED;
				driver->count_cycle = 0;
				KEY_EVENT_CALL(input,
					KEY_EVENT_CODE(info->code, (idx + 1)), driver->event);
			}
		} else {
			driver->event = KEY_EVENT_LONG_RELEASE;
			driver->state = KEY_STATE_NONE;
			KEY_EVENT_CALL(input,
				KEY_EVENT_CODE(info->code, (idx + 1)), driver->event);
		}
		break;

	default:
		LOG_ERR("Unknown key state[%d]", driver->state);
		break;
	}

	/* return key is pressed. */
	return ((pressed || (driver->state != KEY_STATE_NONE)) ? true : false);
}

static void gpio_isr_handler(const struct device *dev,
					struct gpio_callback *cb, uint32_t pins)
{
	struct kbd_gpio_driver *driver = \
			CONTAINER_OF(cb, struct kbd_gpio_driver, gpio_data);
	struct kbd_gpio_data *data = driver->pdata;
	int work_busy = k_work_delayable_busy_get(&data->delayed_work);

	if (!(work_busy & (K_WORK_DELAYED | K_WORK_QUEUED))) {
		k_work_schedule(&data->delayed_work, K_MSEC(SCAN_INTERVAL));
	}
}

static void delayed_work_handler(struct k_work *work)
{
	struct k_work_delayable *delayed_work = k_work_delayable_from_work(work);
	struct kbd_gpio_data *data = \
			CONTAINER_OF(delayed_work, struct kbd_gpio_data, delayed_work);
	const struct kbd_gpio_config *config = data->dev->config;
	int key_count = 0;

	for (int i = 0; i < config->num_keys; i++) {
		if (kbd_gpio_one_key_proc(data->dev, i)) {
			key_count++;
		}
	}

	if (key_count > 0) {
		k_work_schedule(&data->delayed_work, K_MSEC(SCAN_INTERVAL));
	}
}

static int kbd_gpio_setup(const struct device *dev)
{
	const struct kbd_gpio_config *config = dev->config;
	struct kbd_gpio_data *data = dev->data;
	struct input_dev *input = data->input;

	input_internal_setup(input);

	for (int i = 0; i < config->num_keys; i++) {
		const struct gpio_dt_spec *gpio = &config->gpio[i];
		struct kbd_gpio_driver *driver = &data->driver[i];

		gpio_add_callback(gpio->port, &driver->gpio_data);
	}

	/* Maybe key pressed before driver setup. */
	k_work_schedule(&data->delayed_work, K_NO_WAIT);

	return 0;
}

static int kbd_gpio_release(const struct device *dev)
{
	const struct kbd_gpio_config *config = dev->config;
	struct kbd_gpio_data *data = dev->data;
	struct input_dev *input = data->input;

	for (int i = 0; i < config->num_keys; i++) {
		const struct gpio_dt_spec *gpio = &config->gpio[i];
		struct kbd_gpio_driver *driver = &data->driver[i];

		gpio_remove_callback(gpio->port, &driver->gpio_data);
	}

	k_work_cancel_delayable(&data->delayed_work);

	input_internal_release(input);

	return 0;
}

static int kbd_gpio_attr_get(const struct device *dev,
			enum input_attr_type type, union input_attr_data *attr)
{
	struct kbd_gpio_data *data = dev->data;
	struct input_dev *input = data->input;
	int retval = 0;

	retval = input_internal_attr_get(input, type, attr);

	return retval;
}

static int kbd_gpio_attr_set(const struct device *dev,
			enum input_attr_type type, union input_attr_data *attr)
{
	struct kbd_gpio_data *data = dev->data;
	struct input_dev *input = data->input;
	int retval = 0;

	retval = input_internal_attr_set(input, type, attr);

	return retval;
}

static int kbd_gpio_event_read(const struct device *dev, struct input_event *event)
{
	struct kbd_gpio_data *data = dev->data;
	struct input_dev *input = data->input;
	int retval = 0;

	retval = input_internal_event_read(input, event);

	return retval;
}

static int kbd_gpio_event_write(const struct device *dev, struct input_event *event)
{
	struct kbd_gpio_data *data = dev->data;
	struct input_dev *input = data->input;
	int retval = 0;

	retval = input_internal_event_write(input, event);

	return retval;
}

static int kbd_gpio_init(const struct device *dev)
{
	const struct kbd_gpio_config *config = dev->config;
	struct kbd_gpio_data *data = dev->data;
	int err = 0;

	if (config->num_keys <= 0) {
		LOG_ERR("%s: no KEYs found (DT child nodes missing)", dev->name);
		return -ENODEV;
	}

	data->dev = dev;

	k_work_init_delayable(&data->delayed_work, delayed_work_handler);

	memset(data->driver, 0x00, config->num_keys * sizeof(struct kbd_gpio_driver));

	LOG_DBG("gpio key map:");

	for (int i = 0; i < config->num_keys; i++) {
		const struct gpio_dt_spec *gpio = &config->gpio[i];
		const struct key_info_dt_spec *info = &config->info[i];
		struct kbd_gpio_driver *driver = &data->driver[i];

		driver->pdata = data;

		if (!device_is_ready(gpio->port)) {
			LOG_WRN("gpio port[%s] is not ready", gpio->port->name);
			continue;
		}

		err = gpio_pin_configure_dt(gpio, GPIO_INPUT);
		if (err != 0) {
			LOG_WRN("configure extra_flags on gpio[%s %d] fail[%d]",
					gpio->port->name, gpio->pin, err);
			continue;
		}

		err = gpio_pin_interrupt_configure_dt(gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (err != 0) {
			LOG_WRN("Configure interrupt on gpio[%s %d] fail[%d]",
					gpio->port->name, gpio->pin, err);
			continue;
		}

		gpio_init_callback(&driver->gpio_data, gpio_isr_handler, BIT(gpio->pin));

		LOG_DBG("KEY%d: label[%s] gpio[%p %d 0x%04x] code[0x%04x] interval[%d %d %d %d]",
			i, info->label, gpio->port, gpio->pin, gpio->dt_flags, info->code,
			SCAN_INTERVAL,
			(MS_TO_CYCLE(info->debounce_ms, TIME_DEBOUNCE) * SCAN_INTERVAL),
			(MS_TO_CYCLE(info->longpress_ms, TIME_LONGPRESS) * SCAN_INTERVAL),
			(MS_TO_CYCLE(info->holdpress_ms, TIME_HOLDPRESS) * SCAN_INTERVAL));
	}

	return err;
}

static const struct input_driver_api kbd_gpio_api = {
	.setup		 = kbd_gpio_setup,
	.release	 = kbd_gpio_release,
	.attr_get	 = kbd_gpio_attr_get,
	.attr_set	 = kbd_gpio_attr_set,
	.event_read  = kbd_gpio_event_read,
	.event_write = kbd_gpio_event_write,
};

#define KEY_GPIO_DT_SPEC(key_node_id) GPIO_DT_SPEC_GET(key_node_id, gpios),

#define KEY_INFO_DT_SPEC(key_node_id) KEY_INFO_DT_SPEC_GET(key_node_id),

#define KBD_GPIO_DEVICE(i) \
	static const struct gpio_dt_spec gpio_dt_spec_##i[] = { \
		DT_INST_FOREACH_CHILD(i, KEY_GPIO_DT_SPEC)}; \
	static const struct key_info_dt_spec info_dt_sepc_##i[] = { \
		DT_INST_FOREACH_CHILD(i, KEY_INFO_DT_SPEC)}; \
	RING_BUF_DECLARE(input_buf_##i, \
		CONFIG_KEYBOARD_EVENT_MAX_NUMBERS * input_event_size); \
	static struct input_dev kbd_gpio_input_##i = { \
		.buf = &input_buf_##i, \
	}; \
	static struct kbd_gpio_driver \
		kbd_gpio_driver_##i[ARRAY_SIZE(gpio_dt_spec_##i)]; \
	static struct kbd_gpio_data kbd_gpio_data_##i = { \
		.input  = &kbd_gpio_input_##i, \
		.driver = kbd_gpio_driver_##i, \
	}; \
	static const struct kbd_gpio_config kbd_gpio_config_##i = { \
		.num_keys = ARRAY_SIZE(gpio_dt_spec_##i), \
		.gpio = gpio_dt_spec_##i, \
		.info = info_dt_sepc_##i, \
	}; \
	DEVICE_DT_INST_DEFINE(i, \
							kbd_gpio_init, NULL, \
							&kbd_gpio_data_##i, \
							&kbd_gpio_config_##i, \
							POST_KERNEL, \
							CONFIG_INPUT_INIT_PRIORITY, \
							&kbd_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(KBD_GPIO_DEVICE)
