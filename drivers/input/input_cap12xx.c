/*
 * Copyright (c) 2022 Keiya Nobuta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_cap12xx

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cap12xx, CONFIG_INPUT_LOG_LEVEL);

#define REG_MAIN_CONTROL 0x00
#define CONTROL_INT      0x01

#define REG_INPUT_STATUS 0x03

#define REG_INTERRUPT_ENABLE 0x27
#define INTERRUPT_ENABLE     0xFF
#define INTERRUPT_DISABLE    0x00

#define REG_REPEAT_ENABLE 0x28
#define REPEAT_ENABLE     0xFF
#define REPEAT_DISABLE    0x00

struct cap12xx_config {
	struct i2c_dt_spec i2c;
	const uint8_t input_channels;
	const uint16_t *input_codes;
	struct gpio_dt_spec *int_gpio;
	bool repeat;
	const uint16_t poll_interval_ms;
};

struct cap12xx_data {
	const struct device *dev;
	struct k_work work;
	uint8_t prev_input_state;
	struct gpio_callback int_gpio_cb;
	struct k_timer poll_timer;
};

static int cap12xx_clear_interrupt(const struct i2c_dt_spec *i2c)
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

static int cap12xx_enable_interrupt(const struct i2c_dt_spec *i2c, bool enable)
{
	uint8_t intr = enable ? INTERRUPT_ENABLE : INTERRUPT_DISABLE;

	return i2c_reg_write_byte_dt(i2c, REG_INTERRUPT_ENABLE, intr);
}

static int cap12xx_process(const struct device *dev)
{
	const struct cap12xx_config *config = dev->config;
	struct cap12xx_data *data = dev->data;
	int r;
	uint8_t input_state;

	/*
	 * Clear INT bit to clear SENSOR INPUT STATUS bits.
	 * Note that this is also required in polling mode.
	 */
	r = cap12xx_clear_interrupt(&config->i2c);

	if (r < 0) {
		return r;
	}
	r = i2c_reg_read_byte_dt(&config->i2c, REG_INPUT_STATUS, &input_state);
	if (r < 0) {
		return r;
	}

	if (config->int_gpio == NULL) {
		if (data->prev_input_state == input_state) {
			return 0;
		}
	}

	for (uint8_t i = 0; i < config->input_channels; i++) {
		if (input_state & BIT(i)) {
			input_report_key(dev, config->input_codes[i], input_state & BIT(i), true,
					 K_FOREVER);
		} else if (data->prev_input_state & BIT(i)) {
			input_report_key(dev, config->input_codes[i], input_state & BIT(i), true,
					 K_FOREVER);
		}
	}
	data->prev_input_state = input_state;

	return 0;
}

static void cap12xx_work_handler(struct k_work *work)
{
	struct cap12xx_data *data = CONTAINER_OF(work, struct cap12xx_data, work);

	cap12xx_process(data->dev);
}

static void cap12xx_timer_handler(struct k_timer *poll_timer)
{
	struct cap12xx_data *data = CONTAINER_OF(poll_timer, struct cap12xx_data, poll_timer);

	k_work_submit(&data->work);
}

static void cap12xx_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct cap12xx_data *data = CONTAINER_OF(cb, struct cap12xx_data, int_gpio_cb);

	k_work_submit(&data->work);
}

static int cap12xx_init(const struct device *dev)
{
	const struct cap12xx_config *config = dev->config;
	struct cap12xx_data *data = dev->data;
	int r;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C controller device not ready");
		return -ENODEV;
	}

	data->dev = dev;

	k_work_init(&data->work, cap12xx_work_handler);

	if (config->int_gpio == NULL) {
		LOG_DBG("cap12xx driver in polling mode");
		k_timer_init(&data->poll_timer, cap12xx_timer_handler, NULL);
		r = cap12xx_enable_interrupt(&config->i2c, true);
		if (r < 0) {
			LOG_ERR("Could not configure interrupt");
			return r;
		}
		k_timer_start(&data->poll_timer, K_MSEC(config->poll_interval_ms),
			      K_MSEC(config->poll_interval_ms));
	} else {
		LOG_DBG("cap12xx driver in interrupt mode");
		if (!gpio_is_ready_dt(config->int_gpio)) {
			LOG_ERR("Interrupt GPIO controller device not ready (missing device tree "
				"node?)");
			return -ENODEV;
		}

		r = gpio_pin_configure_dt(config->int_gpio, GPIO_INPUT);
		if (r < 0) {
			LOG_ERR("Could not configure interrupt GPIO pin");
			return r;
		}

		r = gpio_pin_interrupt_configure_dt(config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (r < 0) {
			LOG_ERR("Could not configure interrupt GPIO interrupt");
			return r;
		}

		gpio_init_callback(&data->int_gpio_cb, cap12xx_isr_handler,
				   BIT(config->int_gpio->pin));

		r = gpio_add_callback_dt(config->int_gpio, &data->int_gpio_cb);
		if (r < 0) {
			LOG_ERR("Could not set gpio callback");
			return r;
		}

		r = cap12xx_clear_interrupt(&config->i2c);
		if (r < 0) {
			LOG_ERR("Could not clear interrupt");
			return r;
		}
		r = cap12xx_enable_interrupt(&config->i2c, true);
		if (r < 0) {
			LOG_ERR("Could not configure interrupt");
			return r;
		}
		if (config->repeat) {
			r = i2c_reg_write_byte_dt(&config->i2c, REG_REPEAT_ENABLE, REPEAT_ENABLE);
			if (r < 0) {
				LOG_ERR("Could not disable repeated interrupts");
				return r;
			}
			LOG_DBG("cap12xx enabled repeated interrupts");
		} else {
			r = i2c_reg_write_byte_dt(&config->i2c, REG_REPEAT_ENABLE, REPEAT_DISABLE);
			if (r < 0) {
				LOG_ERR("Could not enable repeated interrupts");
				return r;
			}
			LOG_DBG("cap12xx disabled repeated interrupts");
		}
	}
	LOG_DBG("%d channels configured", config->input_channels);
	return 0;
}

#define CAP12XX_INIT(index)                                                                        \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(index, int_gpios), (                                      \
	static struct gpio_dt_spec cap12xx_int_gpio_##index =                                      \
		GPIO_DT_SPEC_INST_GET(index, int_gpios);))                                         \
	static const uint16_t cap12xx_input_codes_##index[] = DT_INST_PROP(index, input_codes);    \
	static const struct cap12xx_config cap12xx_config_##index = {                              \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		.input_channels = DT_INST_PROP_LEN(index, input_codes),                            \
		.input_codes = cap12xx_input_codes_##index,                                        \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(index, int_gpios), (                              \
				.int_gpio = &cap12xx_int_gpio_##index,))                           \
		.repeat = DT_INST_PROP(index, repeat),                                             \
		.poll_interval_ms = DT_INST_PROP_OR(index, poll_interval_ms, 10)};                 \
	static struct cap12xx_data cap12xx_data_##index;                                           \
	DEVICE_DT_INST_DEFINE(index, cap12xx_init, NULL, &cap12xx_data_##index,                    \
			      &cap12xx_config_##index, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,    \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(CAP12XX_INIT)
