/*
 * Copyright (c) 2022 Keiya Nobuta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_cap1203

#include <zephyr/drivers/kscan.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cap1203, CONFIG_KSCAN_LOG_LEVEL);

#define REG_MAIN_CONTROL 0x0
#define CONTROL_INT 0x1

#define REG_INPUT_STATUS 0x03

#define REG_INTERRUPT_ENABLE 0x27
#define INTERRUPT_ENABLE 0x7
#define INTERRUPT_DISABLE 0x0

struct cap1203_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int_gpio;
};

struct cap1203_data {
	struct device *dev;
	kscan_callback_t callback;
	struct k_work work;
	/* Interrupt GPIO callback. */
	struct gpio_callback int_gpio_cb;
#ifdef CONFIG_KSCAN_CAP1203_POLL
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
	uint16_t col;
	bool pressed;

	r = i2c_reg_read_byte_dt(&config->i2c, REG_INPUT_STATUS, &input);
	if (r < 0) {
		return r;
	}

	pressed = !!input;
	if (input & BIT(0)) {
		col = 0;
	}
	if (input & BIT(1)) {
		col = 1;
	}
	if (input & BIT(2)) {
		col = 2;
	}

	LOG_DBG("event: input: %d\n", input);

	/*
	 * Clear INT bit to clear SENSOR INPUT STATUS bits.
	 * Note that this is also required in polling mode.
	 */
	r = cap1203_clear_interrupt(&config->i2c);
	if (r < 0) {
		return r;
	}

	data->callback(dev, 0, col, pressed);

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

#ifdef CONFIG_KSCAN_CAP1203_POLL
static void cap1203_timer_handler(struct k_timer *timer)
{
	struct cap1203_data *data = CONTAINER_OF(timer, struct cap1203_data, timer);

	k_work_submit(&data->work);
}
#endif

static int cap1203_configure(const struct device *dev,
			     kscan_callback_t callback)
{
	struct cap1203_data *data = dev->data;
	const struct cap1203_config *config = dev->config;

	data->callback = callback;

	if (config->int_gpio.port != NULL) {
		int r;

		/* Clear pending interrupt */
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

	return 0;
}

static int cap1203_enable_callback(const struct device *dev)
{
	struct cap1203_data *data = dev->data;

	const struct cap1203_config *config = dev->config;

	if (config->int_gpio.port != NULL) {
		gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	}
#ifdef CONFIG_KSCAN_CAP1203_POLL
	else {
		k_timer_start(&data->timer, K_MSEC(CONFIG_KSCAN_CAP1203_PERIOD),
			      K_MSEC(CONFIG_KSCAN_CAP1203_PERIOD));
	}
#endif
	return 0;
}

static int cap1203_disable_callback(const struct device *dev)
{
	struct cap1203_data *data = dev->data;

	const struct cap1203_config *config = dev->config;

	if (config->int_gpio.port != NULL) {
		gpio_remove_callback(config->int_gpio.port, &data->int_gpio_cb);
	}
#ifdef CONFIG_KSCAN_CAP1203_POLL
	else {
		k_timer_stop(&data->timer);
	}
#endif
	return 0;
}

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
		if (!device_is_ready(config->int_gpio.port)) {
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
	}
#ifdef CONFIG_KSCAN_CAP1203_POLL
	else {
		k_timer_init(&data->timer, cap1203_timer_handler, NULL);

		r = cap1203_enable_interrupt(&config->i2c, false);
		if (r < 0) {
			LOG_ERR("Could not configure interrupt");
			return r;
		}
	}
#endif

	return 0;
}

static const struct kscan_driver_api cap1203_driver_api = {
	.config = cap1203_configure,
	.enable_callback = cap1203_enable_callback,
	.disable_callback = cap1203_disable_callback,
};

#define CAP1203_INIT(index)							\
	static const struct cap1203_config cap1203_config_##index = {		\
		.i2c = I2C_DT_SPEC_INST_GET(index),				\
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(index, int_gpios, {0}),	\
	};									\
	static struct cap1203_data cap1203_data_##index;			\
	DEVICE_DT_INST_DEFINE(index, cap1203_init, NULL,			\
			      &cap1203_data_##index, &cap1203_config_##index,	\
			      POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,		\
			      &cap1203_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAP1203_INIT)
