/*
 * Copyright (c) 2022 Keiya Nobuta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_cap1203

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cap1203, CONFIG_INPUT_LOG_LEVEL);

#define REG_MAIN_CONTROL 0x0
#define CONTROL_INT 0x1

#define REG_INPUT_STATUS 0x03

#define REG_INTERRUPT_ENABLE 0x27
#define INTERRUPT_ENABLE     0x7
#define INTERRUPT_DISABLE    0x0

#define TOUCH_INPUT_COUNT 3

struct cap1203_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int_gpio;
	const uint16_t *input_codes;
};

struct cap1203_data {
	const struct device *dev;
	struct k_work work;
	/* Interrupt GPIO callback. */
	struct gpio_callback int_gpio_cb;
	uint8_t prev_input_state;
#ifdef CONFIG_INPUT_CAP1203_POLL
	/* Timer (polling mode). */
	struct k_timer timer;
#endif
};

static int cap1203_clear_interrupt(const struct i2c_dt_spec *i2c)
{
	uint8_t ctrl;
	int r;

	r = i2c_reg_read_byte_dt(i2c, REG_MAIN_CONTROL, &ctrl);
	if (r < 0) {
		return r;
	}

	ctrl = ctrl & ~CONTROL_INT;
	return i2c_reg_write_byte_dt(i2c, REG_MAIN_CONTROL, ctrl);
}

static int cap1203_enable_interrupt(const struct i2c_dt_spec *i2c, bool enable)
{
	uint8_t intr = enable ? INTERRUPT_ENABLE : INTERRUPT_DISABLE;

	return i2c_reg_write_byte_dt(i2c, REG_INTERRUPT_ENABLE, intr);
}

static int cap1203_process(const struct device *dev)
{
	const struct cap1203_config *config = dev->config;
	struct cap1203_data *data = dev->data;
	int r;
	uint8_t input;
	uint8_t single_input_state;

	r = i2c_reg_read_byte_dt(&config->i2c, REG_INPUT_STATUS, &input);
	if (r < 0) {
		return r;
	}

	for (uint8_t i = 0; i < TOUCH_INPUT_COUNT; i++) {
		single_input_state = input & BIT(i);
		if (single_input_state != (data->prev_input_state & BIT(i))) {
			input_report_key(dev, config->input_codes[i], single_input_state, true,
					 K_FOREVER);
		}
	}
	data->prev_input_state = input;

	LOG_DBG("event: input: %d\n", input);

	/*
	 * Clear INT bit to clear SENSOR INPUT STATUS bits.
	 * Note that this is also required in polling mode.
	 */
	r = cap1203_clear_interrupt(&config->i2c);
	if (r < 0) {
		return r;
	}

	return 0;
}

static void cap1203_work_handler(struct k_work *work)
{
	struct cap1203_data *data = CONTAINER_OF(work, struct cap1203_data, work);

	cap1203_process(data->dev);
}

static void cap1203_isr_handler(const struct device *dev,
				struct gpio_callback *cb, uint32_t pins)
{
	struct cap1203_data *data = CONTAINER_OF(cb, struct cap1203_data, int_gpio_cb);

	k_work_submit(&data->work);
}

#ifdef CONFIG_INPUT_CAP1203_POLL
static void cap1203_timer_handler(struct k_timer *timer)
{
	struct cap1203_data *data = CONTAINER_OF(timer, struct cap1203_data, timer);

	k_work_submit(&data->work);
}
#endif

static int cap1203_init(const struct device *dev)
{
	const struct cap1203_config *config = dev->config;
	struct cap1203_data *data = dev->data;
	int r;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C controller device not ready");
		return -ENODEV;
	}

	data->dev = dev;

	k_work_init(&data->work, cap1203_work_handler);

	if (config->int_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->int_gpio)) {
			LOG_ERR("Interrupt GPIO controller device not ready");
			return -ENODEV;
		}

		r = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
		if (r < 0) {
			LOG_ERR("Could not confighure interrupt GPIO pin");
			return r;
		}

		r = gpio_pin_interrupt_configure_dt(&config->int_gpio,
						   GPIO_INT_EDGE_TO_ACTIVE);
		if (r < 0) {
			LOG_ERR("Could not configure interrupt GPIO interrupt");
			return r;
		}

		gpio_init_callback(&data->int_gpio_cb, cap1203_isr_handler,
				   BIT(config->int_gpio.pin));

		r = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
		if (r < 0) {
			LOG_ERR("Could not set gpio callback");
			return r;
		}

		r = cap1203_clear_interrupt(&config->i2c);
		if (r < 0) {
			LOG_ERR("Could not clear interrupt");
			return r;
		}

		r = cap1203_enable_interrupt(&config->i2c, true);
		if (r < 0) {
			LOG_ERR("Could not configure interrupt");
			return r;
		}
	}
#ifdef CONFIG_INPUT_CAP1203_POLL
	else {
		k_timer_init(&data->timer, cap1203_timer_handler, NULL);

		r = cap1203_enable_interrupt(&config->i2c, false);
		if (r < 0) {
			LOG_ERR("Could not configure interrupt");
			return r;
		}

		k_timer_start(&data->timer, K_MSEC(CONFIG_INPUT_CAP1203_PERIOD),
			      K_MSEC(CONFIG_INPUT_CAP1203_PERIOD));
	}
#endif

	return 0;
}

#define CAP1203_INIT(index)                                                                        \
	static const uint16_t cap1203_input_codes_##inst[] = DT_INST_PROP(index, input_codes);     \
	BUILD_ASSERT(DT_INST_PROP_LEN(index, input_codes) == TOUCH_INPUT_COUNT);                   \
	static const struct cap1203_config cap1203_config_##index = {                              \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(index, int_gpios, {0}),                       \
		.input_codes = cap1203_input_codes_##inst,                                         \
	};                                                                                         \
	static struct cap1203_data cap1203_data_##index;                                           \
	DEVICE_DT_INST_DEFINE(index, cap1203_init, NULL, &cap1203_data_##index,                    \
			      &cap1203_config_##index, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,    \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(CAP1203_INIT)
