/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ..._tma525b

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_touch.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tma525b, CONFIG_INPUT_LOG_LEVEL);

#if 0
/* TMA525B used registers */
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
#endif

/** TMA525B configuration (DT). */
struct tma525b_config {
	struct input_touchscreen_common_config common;
	/** I2C bus. */
	struct i2c_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
#ifdef CONFIG_INPUT_TMA525B_INTERRUPT
	/** Interrupt GPIO information. */
	struct gpio_dt_spec int_gpio;
#endif
};

/** TMA525B data. */
struct tma525b_data {
	/** Work queue (for deferred read). */
	struct k_work work;
#ifdef CONFIG_INPUT_TMA525B_INTERRUPT
	/** Interrupt GPIO callback. */
	struct gpio_callback int_gpio_cb;
#else
	/** Timer (polling mode). */
	struct k_timer timer;
#endif
	/** Last pressed state. */
	bool pressed_old;
};

INPUT_TOUCH_STRUCT_CHECK(struct tma525b_config);

static int tma525b_process(const struct device *dev)
{
#if 0
	const struct tma525b_config *config = dev->config;
	struct tma525b_data *data = dev->data;

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
		input_touchscreen_report_pos(dev, col, row, K_FOREVER);
		input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
	} else if (data->pressed_old && !pressed) {
		input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
	}
	data->pressed_old = pressed;
#endif

	return 0;
}

static void tma525b_work_handler(struct k_work *work)
{
	struct tma525b_data *data = CONTAINER_OF(work, struct tma525b_data, work);

	tma525b_process(data->dev);
}

#ifdef CONFIG_INPUT_TMA525B_INTERRUPT
static void tma525b_isr_handler(const struct device *dev,
			       struct gpio_callback *cb, uint32_t pins)
{
	struct tma525b_data *data = CONTAINER_OF(cb, struct tma525b_data, int_gpio_cb);

	k_work_submit(&data->work);
}
#else
static void tma525b_timer_handler(struct k_timer *timer)
{
	struct tma525b_data *data = CONTAINER_OF(timer, struct tma525b_data, timer);

	k_work_submit(&data->work);
}
#endif

static int tma525b_init(const struct device *dev)
{
	const struct tma525b_config *config = dev->config;
	struct tma525b_data *data = dev->data;
	int err_code;


	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C controller device not ready");
		return -ENODEV;
	}

	k_work_init(&data->work, tma525b_work_handler);

	/* GPIO Power on Sequence */
	if (config->power_gpio.port != NULL) {
		err_code = gpio_pin_set_dt(&config->power_gpio, 1);
		if (err_code < 0) {
			LOG_ERR("Could not enable power GPIO");
			return err_code;
		}
		if (config->reset_gpio.port != NULL) {
			err_code = gpio_pin_set_dt(&config->reset_gpio, 0);
			if (err_code < 0) {
				LOG_ERR("Could not set reset GPIO low");
				return r;
			}
			/* Datasheet states to hold for 5ms after activation of reset pin */
			k_sleep(K_MSEC(5));

			err_code = gpio_pin_set_dt(&config->reset_gpio, 1);
			if (err_code < 0) {
				LOG_ERR("Could not set reset GPIO high");
				return err_code;
			}
		}
	} else {
		LOG_ERR("Could not enable power GPIO");
		return err_code;
	}

#ifdef CONFIG_INPUT_TMA525B_INTERRUPT
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Interrupt GPIO controller device not ready");
		return -ENODEV;
	}

	err_code = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (err_code < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin");
		return err_code;
	}

	err_code = gpio_pin_interrupt_configure_dt(&config->int_gpio,
					    GPIO_INT_EDGE_TO_ACTIVE);
	if (err_code < 0) {
		LOG_ERR("Could not configure interrupt GPIO interrupt.");
		return err_code;
	}

	gpio_init_callback(&data->int_gpio_cb, tma525b_isr_handler,
			   BIT(config->int_gpio.pin));
	err_code = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (err_code < 0) {
		LOG_ERR("Could not set gpio callback");
		return err_code;
	}
#else
	k_timer_init(&data->timer, tma525b_timer_handler, NULL);
	k_timer_start(&data->timer, K_MSEC(CONFIG_INPUT_TMA525B_PERIOD),
		      K_MSEC(CONFIG_INPUT_TMA525B_PERIOD));
#endif
	return 0;
}

#define TMA525B_INIT(index)								\
	static const struct tma525b_config tma525b_config_##index = {			\
		.common = INPUT_TOUCH_DT_INST_COMMON_CONFIG_INIT(index),		\
		.bus = I2C_DT_SPEC_INST_GET(index),					\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(index, reset_gpios, {0}),	\
		IF_ENABLED(CONFIG_INPUT_TMA525B_INTERRUPT,				\
		(.int_gpio = GPIO_DT_SPEC_INST_GET(index, int_gpios),))			\
	};										\
	static struct tma525b_data tma525b_data_##index;				\
	DEVICE_DT_INST_DEFINE(index, tma525b_init, 0,					\
			      &tma525b_data_##index, &tma525b_config_##index,		\
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TMA525B_INIT)
