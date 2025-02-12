/*
 * Copyright (c) 2020,2023 NXP
 * Copyright (c) 2020 Mark Olsson <mark@markolsson.se>
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT focaltech_ft5336

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ft5336, CONFIG_INPUT_LOG_LEVEL);

/* FT5336 used registers */
#define REG_TD_STATUS		0x02U
#define REG_P1_XH		0x03U
#define REG_G_PMODE		0xA5U

/* REG_TD_STATUS: Touch points. */
#define TOUCH_POINTS_POS	0U
#define TOUCH_POINTS_MSK	0x0FU

/* REG_Pn_XH: Events. */
#define EVENT_POS		6U
#define EVENT_MSK		0x03U

#define EVENT_PRESS_DOWN	0x00U
#define EVENT_LIFT_UP		0x01U
#define EVENT_CONTACT		0x02U
#define EVENT_NONE		0x03U

/* REG_Pn_YH: Touch ID */
#define TOUCH_ID_POS		4U
#define TOUCH_ID_MSK		0x0FU

#define TOUCH_ID_INVALID	0x0FU

/* REG_Pn_XH and REG_Pn_YH: Position */
#define POSITION_H_MSK		0x0FU

/* REG_G_PMODE: Power Consume Mode */
#define PMOD_HIBERNATE		0x03U

/** FT5336 configuration (DT). */
struct ft5336_config {
	/** I2C bus. */
	struct i2c_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
#ifdef CONFIG_INPUT_FT5336_INTERRUPT
	/** Interrupt GPIO information. */
	struct gpio_dt_spec int_gpio;
#endif
};

/** FT5336 data. */
struct ft5336_data {
	/** Device pointer. */
	const struct device *dev;
	/** Work queue (for deferred read). */
	struct k_work work;
#ifdef CONFIG_INPUT_FT5336_INTERRUPT
	/** Interrupt GPIO callback. */
	struct gpio_callback int_gpio_cb;
#else
	/** Timer (polling mode). */
	struct k_timer timer;
#endif
	/** Last pressed state. */
	bool pressed_old;
};

static int ft5336_process(const struct device *dev)
{
	const struct ft5336_config *config = dev->config;
	struct ft5336_data *data = dev->data;

	int r;
	uint8_t points;
	uint8_t coords[4U];
	uint16_t row, col;
	bool pressed;

	/* obtain number of touch points */
	r = i2c_reg_read_byte_dt(&config->bus, REG_TD_STATUS, &points);
	if (r < 0) {
		return r;
	}

	points = FIELD_GET(TOUCH_POINTS_MSK, points);
	if (points != 0) {
		/* Any number of touches still counts as one touch. All touch
		 * points except the first are ignored. Obtain first point
		 * X, Y coordinates from:
		 * REG_P1_XH, REG_P1_XL, REG_P1_YH, REG_P1_YL.
		 * We ignore the Event Flag because Zephyr only cares about
		 * pressed / not pressed and not press down / lift up
		 */
		r = i2c_burst_read_dt(&config->bus, REG_P1_XH, coords, sizeof(coords));
		if (r < 0) {
			return r;
		}

		row = ((coords[0] & POSITION_H_MSK) << 8U) | coords[1];
		col = ((coords[2] & POSITION_H_MSK) << 8U) | coords[3];

		uint8_t touch_id = FIELD_GET(TOUCH_ID_MSK, coords[2]);

		if (touch_id != TOUCH_ID_INVALID) {
			pressed = true;
			LOG_DBG("points: %d, touch_id: %d, row: %d, col: %d",
				 points, touch_id, row, col);
		} else {
			pressed = false;
			LOG_WRN("bad TOUCH_ID: row: %d, col: %d", row, col);
		}
	} else  {
		/* no touch = no press */
		pressed = false;
	}

	if (pressed) {
		input_report_abs(dev, INPUT_ABS_X, col, false, K_FOREVER);
		input_report_abs(dev, INPUT_ABS_Y, row, false, K_FOREVER);
		input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
	} else if (data->pressed_old && !pressed) {
		input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
	}
	data->pressed_old = pressed;

	return 0;
}

static void ft5336_work_handler(struct k_work *work)
{
	struct ft5336_data *data = CONTAINER_OF(work, struct ft5336_data, work);

	ft5336_process(data->dev);
}

#ifdef CONFIG_INPUT_FT5336_INTERRUPT
static void ft5336_isr_handler(const struct device *dev,
			       struct gpio_callback *cb, uint32_t pins)
{
	struct ft5336_data *data = CONTAINER_OF(cb, struct ft5336_data, int_gpio_cb);

	k_work_submit(&data->work);
}
#else
static void ft5336_timer_handler(struct k_timer *timer)
{
	struct ft5336_data *data = CONTAINER_OF(timer, struct ft5336_data, timer);

	k_work_submit(&data->work);
}
#endif

static int ft5336_init(const struct device *dev)
{
	const struct ft5336_config *config = dev->config;
	struct ft5336_data *data = dev->data;
	int r;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C controller device not ready");
		return -ENODEV;
	}

	data->dev = dev;

	k_work_init(&data->work, ft5336_work_handler);

	if (config->reset_gpio.port != NULL) {
		/* Enable reset GPIO and assert reset */
		r = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (r < 0) {
			LOG_ERR("Could not enable reset GPIO");
			return r;
		}
		/*
		 * Datasheet requires reset be held low 1 ms, or
		 * 1 ms + 100us if powering on controller. Hold low for
		 * 5 ms to be safe.
		 */
		k_sleep(K_MSEC(5));
		/* Pull reset pin high to complete reset sequence */
		r = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (r < 0) {
			return r;
		}
	}

#ifdef CONFIG_INPUT_FT5336_INTERRUPT
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Interrupt GPIO controller device not ready");
		return -ENODEV;
	}

	r = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (r < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin");
		return r;
	}

	r = gpio_pin_interrupt_configure_dt(&config->int_gpio,
					    GPIO_INT_EDGE_TO_ACTIVE);
	if (r < 0) {
		LOG_ERR("Could not configure interrupt GPIO interrupt.");
		return r;
	}

	gpio_init_callback(&data->int_gpio_cb, ft5336_isr_handler,
			   BIT(config->int_gpio.pin));
	r = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (r < 0) {
		LOG_ERR("Could not set gpio callback");
		return r;
	}
#else
	k_timer_init(&data->timer, ft5336_timer_handler, NULL);
	k_timer_start(&data->timer, K_MSEC(CONFIG_INPUT_FT5336_PERIOD),
		      K_MSEC(CONFIG_INPUT_FT5336_PERIOD));
#endif

	r = pm_device_runtime_enable(dev);
	if (r < 0 && r != -ENOTSUP) {
		LOG_ERR("Failed to enable runtime power management");
		return r;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int ft5336_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	const struct ft5336_config *config = dev->config;
#ifndef CONFIG_INPUT_FT5336_INTERRUPT
	struct ft5336_data *data = dev->data;
#endif
	int ret;

	if (config->reset_gpio.port == NULL) {
		return -ENOTSUP;
	}

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = i2c_reg_write_byte_dt(&config->bus,
					    REG_G_PMODE, PMOD_HIBERNATE);
		if (ret < 0) {
			return ret;
		}

#ifndef CONFIG_INPUT_FT5336_INTERRUPT
		k_timer_stop(&data->timer);
#endif
		break;
	case PM_DEVICE_ACTION_RESUME:
		ret = gpio_pin_set_dt(&config->reset_gpio, 1);
		if (ret < 0) {
			return ret;
		}

		k_sleep(K_MSEC(5));

		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (ret < 0) {
			return ret;
		}

#ifndef CONFIG_INPUT_FT5336_INTERRUPT
		k_timer_start(&data->timer,
			      K_MSEC(CONFIG_INPUT_FT5336_PERIOD),
			      K_MSEC(CONFIG_INPUT_FT5336_PERIOD));
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#define FT5336_INIT(index)								\
	PM_DEVICE_DT_INST_DEFINE(n, ft5336_pm_action);					\
	static const struct ft5336_config ft5336_config_##index = {			\
		.bus = I2C_DT_SPEC_INST_GET(index),					\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(index, reset_gpios, {0}),	\
		IF_ENABLED(CONFIG_INPUT_FT5336_INTERRUPT,				\
		(.int_gpio = GPIO_DT_SPEC_INST_GET(index, int_gpios),))			\
	};										\
	static struct ft5336_data ft5336_data_##index;					\
	DEVICE_DT_INST_DEFINE(index, ft5336_init, PM_DEVICE_DT_INST_GET(n),		\
			      &ft5336_data_##index, &ft5336_config_##index,		\
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(FT5336_INIT)
