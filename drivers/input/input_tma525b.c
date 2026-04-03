/*
 * Copyright (c) 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT parade_tma525b

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_touch.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tma525b, CONFIG_INPUT_LOG_LEVEL);

#define TMA525B_BOOT_DELAY_MS 120U

/* TMA525B maximum number of simultaneously detected touches */
#define TMA525B_MAX_TOUCHES 4U

/* TMA525B register address where touch data begin */
#define TMA525B_TOUCH_DATA_SUBADDR 1U

/* TMA525B raw touch data length */
#define TMA525B_TOUCH_DATA_LEN 264U

/* TMA525B touch data header length to read first */
#define TMA525B_TOUCH_DATA_LEN_BYTES 2U

/* TMA525B report ID for touch data */
#define TMA525B_REPORT_ID_TOUCH 0x01U

/* Touch event types - matching fsl_tma525b.h */
enum touch_event {
	TOUCH_RESERVED = 0, /* No touch event detected */
	TOUCH_DOWN = 1,     /* Touch down event detected */
	TOUCH_CONTACT = 2,  /* Touch point moving */
	TOUCH_UP = 3        /* Touch event finished */
};

/* TMA525B touch point structure */
struct tma525b_touch_point {
	uint8_t touch_type; /* 0 for standard finger/glove, 1 for proximity. Not used*/
	uint8_t event_id;   /* Bit 0-4: touch ID, bit 5-6: touch event */
	uint16_t x;
	uint16_t y;
	uint8_t pressure;     /* Touch intensity. Not used in current driver model. */
	uint16_t axis_len_mm; /* Axis length. Not used in current driver model. */
	/* Angle between panel vertical axis and major axis. Not used in current driver model. */
	uint8_t orientation;
} __packed;

/* TMA525B touch data structure */
struct tma525b_touch_data {
	uint16_t length; /* Packet length. */
	uint8_t report_id;
	/* Timestamp in 100us units. Not used in current driver model. */
	uint16_t timestamp_100us;
	uint8_t num_touch; /* Number of touch points detected */
	/* 2 MSB: report counter, 3 LSB: noise effects. Not used in current driver model. */
	uint8_t report_noise;
	struct tma525b_touch_point touch[TMA525B_MAX_TOUCHES];
} __packed;

/* Macros for extracting touch data. */
#define TMA525B_TOUCH_POINT_GET_ID(event_id)    (event_id & 0x1F)
#define TMA525B_TOUCH_POINT_GET_EVENT(event_id) ((event_id & 0x60) >> 5U)

/* TMA525B configuration (from device tree) */
struct tma525b_config {
	struct input_touchscreen_common_config common;
	struct i2c_dt_spec bus;
	struct gpio_dt_spec pwr_gpio;
	struct gpio_dt_spec rst_gpio;
	struct gpio_dt_spec int_gpio;
};

/* TMA525B runtime data */
struct tma525b_data {
	const struct device *dev;
	struct k_work work;
	uint8_t touch_buf[TMA525B_TOUCH_DATA_LEN];
	uint8_t prev_touch_count;
	struct {
		uint8_t id;
		uint16_t x;
		uint16_t y;
	} prev_touches[TMA525B_MAX_TOUCHES];
#ifdef CONFIG_INPUT_TMA525B_INTERRUPT
	struct gpio_callback int_gpio_cb;
#else
	struct k_timer timer;
#endif
};

INPUT_TOUCH_STRUCT_CHECK(struct tma525b_config);

static int tma525b_process(const struct device *dev)
{
	const struct tma525b_config *config = dev->config;
	struct tma525b_data *data = dev->data;
	struct tma525b_touch_data *touch_data;
	int ret;
	uint8_t touch_id;
	enum touch_event event;
	uint8_t touch_count;

	/* Read first two bytes to get the data length */
	ret = i2c_burst_read_dt(&config->bus, TMA525B_TOUCH_DATA_SUBADDR, data->touch_buf,
				TMA525B_TOUCH_DATA_LEN_BYTES);
	if (ret < 0) {
		LOG_ERR("Failed to read data length: %d", ret);
		return ret;
	}

	touch_data = (struct tma525b_touch_data *)data->touch_buf;

	/* Validate touch data length */
	if (touch_data->length <= 0x02 || touch_data->length > TMA525B_TOUCH_DATA_LEN) {
		LOG_DBG("Invalid touch data length: %d", touch_data->length);
		return -EINVAL;
	}

	/* Read the full touch data according to length */
	ret = i2c_burst_read_dt(&config->bus, TMA525B_TOUCH_DATA_SUBADDR, data->touch_buf,
				touch_data->length);
	if (ret < 0) {
		LOG_ERR("Failed to read touch data: %d", ret);
		return ret;
	}

	/* Only when report ID is 0x01 the touch data is valid */
	if (touch_data->report_id != TMA525B_REPORT_ID_TOUCH) {
		LOG_DBG("Invalid report ID: 0x%02x", touch_data->report_id);
		return -EINVAL;
	}

	/* Get number of touches */
	touch_count = MIN(touch_data->num_touch, CONFIG_INPUT_TMA525B_MAX_TOUCH_POINTS);

	/* Process current touch points */
	for (uint8_t i = 0; i < touch_count; i++) {
		int x, y;

		touch_id = TMA525B_TOUCH_POINT_GET_ID(touch_data->touch[i].event_id);
		event = TMA525B_TOUCH_POINT_GET_EVENT(touch_data->touch[i].event_id);
		x = touch_data->touch[i].x;
		y = touch_data->touch[i].y;

		/* Update coordinates only if there is valid touch event */
		if (event == TOUCH_RESERVED) {
			continue;
		}

		if (CONFIG_INPUT_TMA525B_MAX_TOUCH_POINTS > 1) {
			input_report_abs(dev, INPUT_ABS_MT_SLOT, touch_id, true, K_FOREVER);
		}

		input_touchscreen_report_pos(dev, x, y, K_FOREVER);

		/* Report touch state based on event type */
		if (event == TOUCH_DOWN || event == TOUCH_CONTACT) {
			input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
		} else if (event == TOUCH_UP) {
			input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
		}

		/* Store current touch for tracking */
		data->prev_touches[i].id = touch_id;
		data->prev_touches[i].x = x;
		data->prev_touches[i].y = y;
	}

	/* Handle release events for touches that disappeared */
	for (uint8_t i = 0; i < data->prev_touch_count; i++) {
		bool touch_found = false;

		/* Look for previous touch in current touch list */
		for (uint8_t j = 0; j < touch_count; j++) {
			if (data->prev_touches[i].id ==
			    TMA525B_TOUCH_POINT_GET_ID(touch_data->touch[j].event_id)) {
				touch_found = true;
				break;
			}
		}

		/* If previous touch not found, report release */
		if (!touch_found) {
			if (CONFIG_INPUT_TMA525B_MAX_TOUCH_POINTS > 1) {
				input_report_abs(dev, INPUT_ABS_MT_SLOT, data->prev_touches[i].id,
						 true, K_FOREVER);
			}
			input_touchscreen_report_pos(dev, data->prev_touches[i].x,
						     data->prev_touches[i].y, K_FOREVER);
			input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
		}
	}

	/* Update previous touch count */
	data->prev_touch_count = touch_count;

	return 0;
}

static void tma525b_work_handler(struct k_work *work)
{
	struct tma525b_data *data = CONTAINER_OF(work, struct tma525b_data, work);

	tma525b_process(data->dev);
}

#ifdef CONFIG_INPUT_TMA525B_INTERRUPT
static void tma525b_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
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

static int tma525b_chip_init(const struct device *dev)
{
	const struct tma525b_config *config = dev->config;
	uint8_t read_buf[2];
	int ret;
	int retry = 0;

	/* Power on sequence */
	if (config->pwr_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->pwr_gpio, 1);
		if (ret < 0) {
			LOG_ERR("Failed to enable power: %d", ret);
			return ret;
		}
		k_sleep(K_MSEC(10));
	}

	/* Reset the controller */
	if (config->rst_gpio.port != NULL) {
		gpio_pin_set_dt(&config->rst_gpio, 1);
		k_sleep(K_MSEC(5));
		gpio_pin_set_dt(&config->rst_gpio, 0);
	}

	k_sleep(K_MSEC(TMA525B_BOOT_DELAY_MS));

	/* Enter application mode - check if ready */
	while (retry < CONFIG_INPUT_TMA525B_RETRY_TIMES) {
		ret = i2c_burst_read_dt(&config->bus, TMA525B_TOUCH_DATA_SUBADDR, read_buf,
					sizeof(read_buf));
		if (ret == 0) {
			/* Check for application mode signature */
			if ((read_buf[0] == 0x02U && read_buf[1] == 0x00U) ||
			    read_buf[1] == 0xFFU) {
				LOG_INF("TMA525B entered application mode");
				break;
			}
		}
		k_sleep(K_MSEC(TMA525B_BOOT_DELAY_MS));
		retry++;
	}

	if (retry == 0U) {
		LOG_ERR("TMA525B failed to enter application mode");
		return -ENODEV;
	}

	return 0;
}

static int tma525b_init(const struct device *dev)
{
	const struct tma525b_config *config = dev->config;
	struct tma525b_data *data = dev->data;
	int ret;

	data->dev = dev;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C controller not ready");
		return -ENODEV;
	}

	/* Configure power GPIO if available */
	if (config->pwr_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->pwr_gpio)) {
			LOG_ERR("Power GPIO controller not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->pwr_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure power GPIO: %d", ret);
			return ret;
		}
	}

	/* Configure reset GPIO if available */
	if (config->rst_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->rst_gpio)) {
			LOG_ERR("Reset GPIO controller not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->rst_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO: %d", ret);
			return ret;
		}
	}

	/* Initialize work queue */
	k_work_init(&data->work, tma525b_work_handler);

	/* Initialize the chip */
	ret = tma525b_chip_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize TMA525B chip: %d", ret);
		return ret;
	}

#ifdef CONFIG_INPUT_TMA525B_INTERRUPT
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Interrupt GPIO controller not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt GPIO: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->int_gpio_cb, tma525b_isr_handler, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add GPIO callback: %d", ret);
		return ret;
	}
	LOG_DBG("TMA525B using interrupt mode");
#else
	k_timer_init(&data->timer, tma525b_timer_handler, NULL);
	k_timer_start(&data->timer, K_MSEC(CONFIG_INPUT_TMA525B_PERIOD_MS),
		      K_MSEC(CONFIG_INPUT_TMA525B_PERIOD_MS));
	LOG_DBG("TMA525B using polling mode");
#endif

	ret = pm_device_runtime_enable(dev);
	if (ret < 0 && ret != -ENOTSUP) {
		LOG_ERR("Failed to enable runtime power management: %d", ret);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int tma525b_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct tma525b_config *config = dev->config;
#ifndef CONFIG_INPUT_TMA525B_INTERRUPT
	struct tma525b_data *data = dev->data;
#endif
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* Power down the device */
		if (config->pwr_gpio.port != NULL) {
			ret = gpio_pin_set_dt(&config->pwr_gpio, 0);
			if (ret < 0) {
				return ret;
			}
		}

#ifndef CONFIG_INPUT_TMA525B_INTERRUPT
		k_timer_stop(&data->timer);
#endif
		break;
	case PM_DEVICE_ACTION_RESUME:
		/* Re-initialize the chip on resume */
		ret = tma525b_chip_init(dev);
		if (ret < 0) {
			return ret;
		}

#ifndef CONFIG_INPUT_TMA525B_INTERRUPT
		k_timer_start(&data->timer, K_MSEC(CONFIG_INPUT_TMA525B_PERIOD_MS),
			      K_MSEC(CONFIG_INPUT_TMA525B_PERIOD_MS));
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#define TMA525B_INIT(index)                                                                        \
	PM_DEVICE_DT_INST_DEFINE(index, tma525b_pm_action);                                        \
	static const struct tma525b_config tma525b_config_##index = {                              \
		.common = INPUT_TOUCH_DT_INST_COMMON_CONFIG_INIT(index),                           \
		.bus = I2C_DT_SPEC_INST_GET(index),                                                \
		.rst_gpio = GPIO_DT_SPEC_INST_GET_OR(index, reset_gpios, {0}),                     \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(index, int_gpios, {0}),                       \
		.pwr_gpio = GPIO_DT_SPEC_INST_GET_OR(index, power_gpios, {0}),                     \
	};                                                                                         \
	static struct tma525b_data tma525b_data_##index;                                           \
	DEVICE_DT_INST_DEFINE(index, tma525b_init, PM_DEVICE_DT_INST_GET(index),                   \
			      &tma525b_data_##index, &tma525b_config_##index, POST_KERNEL,         \
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TMA525B_INIT)
