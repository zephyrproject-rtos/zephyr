/*
 * Copyright (c) 2024, Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Address by default is 0x55 */

#define DT_DRV_COMPAT sitronix_cf1133

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(cf1133, CONFIG_INPUT_LOG_LEVEL);

/* cf1133 used registers */
#define FIRMWARE_VERSION    0x0
#define STATUS_REG          0x1
#define DEVICE_CONTROL_REG  0x2
#define TIMEOUT_TO_IDLE_REG 0x3
#define XY_RESOLUTION_HIGH  0x4
#define X_RESOLUTION_LOW    0x5
#define Y_RESOLUTION_LOW    0x6
#define DEVICE_CONTROL_REG2 0x09
#define FIRMWARE_REVISION_3 0x0C
#define FIRMWARE_REVISION_2 0x0D
#define FIRMWARE_REVISION_1 0x0E
#define FIRMWARE_REVISION_0 0x0F
#define FINGERS             0x10
#define KEYS_REG            0x11
#define XY0_COORD_H         0x12
#define X0_COORD_L          0x13
#define Y0_COORD_L          0x14
#define I2C_PROTOCOL        0x3E
#define MAX_NUM_TOUCHES     0x3F
#define DATA_0_HIGH         0x40
#define DATA_0_LOW          0x41
#define MISC_CONTROL        0xF1
#define SMART_WAKE_UP_REG   0xF2
#define CHIP_ID             0xF4
#define PAGE_REG            0xFF

/* Constants */
#define SUPPORTED_POINT			 0x1
#define PIXEL_DATA_LENGTH_B      0x3
#define PIXEL_DATA_LENGTH_A      0x4
#define SITRONIX_RESERVED_TYPE_0 0x0
#define SITRONIX_A_TYPE          0x1
#define SITRONIX_B_TYPE          0x2

/* Mask */
#define ONE_D_SENSING_CONTROL_SHFT GENMASK(1, 1)
#define ONE_D_SENSING_CONTROL_BMSK GENMASK(1, 0)
#define I2C_PROTOCOL_BMSK          GENMASK(1, 0)
#define TOUCH_POINT_VALID_MSK	   GENMASK(7, 7)

/* Offset for coordinates registers */
#define XY_COORD_H               0x0
#define X_COORD_L                0x1
#define Y_COORD_L                0x2

/* CF1133 configuration (DT) */
struct cf1133_config {
	/** I2C bus. */
	struct i2c_dt_spec bus;
#ifdef CONFIG_INPUT_CF1133_INTERRUPT
	/** Interrupt GPIO information. */
	struct gpio_dt_spec int_gpio;
#endif
};

/* CF1133 data */
struct cf1133_data {
	/** Device pointer. */
	const struct device *dev;
	/** Work queue (for deferred read). */
	struct k_work work;
#ifdef CONFIG_INPUT_CF1133_INTERRUPT
	/** Interrupt GPIO callback. */
	struct gpio_callback int_gpio_cb;
#else
	/* Timer (polling mode) */
	struct k_timer timer;
#endif
	/* Last pressed state */
	uint8_t pressed_old : 1;
	uint8_t pressed : 1;

	int resolution_x;
	int resolution_y;
	uint8_t touch_protocol_type;
	uint8_t pixel_length;
	uint8_t chip_id;
};

static int cf1133_get_chip_id(const struct device *dev)
{
	int ret;
	uint8_t buffer[3];
	uint8_t num_x;
	uint8_t num_y;
	const struct cf1133_config *config = dev->config;
	struct cf1133_data *data = dev->data;

	ret = i2c_burst_read_dt(&config->bus, CHIP_ID, buffer, sizeof(buffer));
	if (ret < 0) {
		LOG_ERR("Read burst failed: %d", ret);
		return ret;
	}
	if (buffer[0] == 0) {
		if (buffer[1] + buffer[2] > 32) {
			data->chip_id = 2;
		} else {
			data->chip_id = 0;
		}
	} else {
		data->chip_id = buffer[0];
	}
	num_x = buffer[1];
	num_y = buffer[2];
	LOG_DBG("Chip ID = %d, num_x = %d, num_y = %d", data->chip_id, num_x, num_y);

	return 0;
}

static int cf1133_get_protocol_type(const struct device *dev)
{
	int ret;
	uint8_t buffer;
	uint8_t sensing_mode;
	const struct cf1133_config *config = dev->config;
	struct cf1133_data *data = dev->data;

	if (data->chip_id <= 3) {
		ret = i2c_reg_read_byte_dt(&config->bus, I2C_PROTOCOL, &buffer);
		if (ret < 0) {
			LOG_ERR("read i2c protocol failed: %d", ret);
			return ret;
		}
		data->touch_protocol_type = FIELD_GET(I2C_PROTOCOL_BMSK, buffer);
		LOG_DBG("i2c protocol = %d", data->touch_protocol_type);
		sensing_mode = FIELD_GET(ONE_D_SENSING_CONTROL_BMSK << ONE_D_SENSING_CONTROL_SHFT,
					 buffer);
		LOG_DBG("sensing mode = %d", sensing_mode);
	} else {
		data->touch_protocol_type = SITRONIX_A_TYPE;
		LOG_DBG("i2c protocol = %d", data->touch_protocol_type);

		ret = i2c_reg_read_byte_dt(&config->bus, 0xf0, &buffer);
		if (ret < 0) {
			LOG_ERR("read sensing mode failed: (%d)", ret);
			return ret;
		}
		sensing_mode = FIELD_GET(ONE_D_SENSING_CONTROL_BMSK, buffer);
		LOG_DBG("sensing mode = %d", sensing_mode);
	}
	return 0;
}

static int cf1133_ts_init(const struct device *dev)
{
	struct cf1133_data *data = dev->data;
	int ret;

	/* Get device status before use to do at least once */
	ret = cf1133_get_chip_id(dev);
	if (ret < 0) {
		LOG_ERR("Read chip id failed: %d", ret);
		return ret;
	}

	ret = cf1133_get_protocol_type(dev);
	if (ret < 0) {
		LOG_ERR("Read protocol failed: %d", ret);
		return ret;
	}

	if (data->touch_protocol_type == SITRONIX_A_TYPE) {
		data->pixel_length = PIXEL_DATA_LENGTH_A;
	} else {
		data->pixel_length = PIXEL_DATA_LENGTH_B;
	}
	LOG_DBG("Pixel length: %d", data->pixel_length);
	return ret;
}

static int cf1133_process(const struct device *dev)
{
	const struct cf1133_config *config = dev->config;
	struct cf1133_data *data = dev->data;
	uint16_t y;
	uint16_t x;
	int ret;
	uint8_t buffer[1 + SUPPORTED_POINT * PIXEL_DATA_LENGTH_A];

	/* Coordinates are retrieved for one valid touch point detected*/
	ret = i2c_burst_read_dt(&config->bus, KEYS_REG, buffer,
			      1 + SUPPORTED_POINT * data->pixel_length);
	if (ret < 0) {
		LOG_ERR("Read coordinates failed: %d", ret);
		return ret;
	}

	/* Coordinates for one valid touch point */
	if (buffer[1 + XY_COORD_H] & TOUCH_POINT_VALID_MSK) {
		x = (uint16_t)(buffer[1 + XY_COORD_H] & 0x70) << 4 | buffer[1 + X_COORD_L];
		y = (uint16_t)(buffer[1 + XY_COORD_H] & 0x07) << 8 | buffer[1 + Y_COORD_L];
		data->pressed = true;

		if (!data->pressed_old) {
			/* Finger pressed */
			input_report_abs(dev, INPUT_ABS_X, x, false, K_FOREVER);
			input_report_abs(dev, INPUT_ABS_Y, y, false, K_FOREVER);
			input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
			LOG_DBG("Finger is touching x = %i y = %i", x, y);
		} else if (data->pressed_old) {
			/* Continuous pressed */
			input_report_abs(dev, INPUT_ABS_X, x, false, K_FOREVER);
			input_report_abs(dev, INPUT_ABS_Y, y, false, K_FOREVER);
			LOG_DBG("Finger keeps touching x = %i y = %i", x, y);
		}
	} else {
		data->pressed = false;

		if (data->pressed_old) {
			/* Finger removed */
			input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
			LOG_DBG("Finger is removed");
		}
	}

	data->pressed_old = data->pressed;

	return 0;
}

static void cf1133_work_handler(struct k_work *work)
{
	struct cf1133_data *data = CONTAINER_OF(work, struct cf1133_data, work);

	cf1133_process(data->dev);
}

#ifdef CONFIG_INPUT_CF1133_INTERRUPT

static void cf1133_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct cf1133_data *data = CONTAINER_OF(cb, struct cf1133_data, int_gpio_cb);

	k_work_submit(&data->work);
}
#else
static void cf1133_timer_handler(struct k_timer *timer)
{
	struct cf1133_data *data = CONTAINER_OF(timer, struct cf1133_data, timer);

	k_work_submit(&data->work);
}
#endif

static int cf1133_init(const struct device *dev)
{
	const struct cf1133_config *config = dev->config;
	struct cf1133_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C controller device not ready");
		return -ENODEV;
	}

	data->dev = dev;
	k_work_init(&data->work, cf1133_work_handler);

#ifdef CONFIG_INPUT_CF1133_INTERRUPT

	LOG_DBG("Int conf for TS gpio: %d", &config->int_gpio);

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Interrupt GPIO controller device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin");
		return ret;
	}

	gpio_init_callback(&data->int_gpio_cb, cf1133_isr_handler, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback");
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO interrupt.");
		return ret;
	}
#else
	LOG_DBG("Timer Mode");
	k_timer_init(&data->timer, cf1133_timer_handler, NULL);
	k_timer_start(&data->timer, K_MSEC(CONFIG_INPUT_CF1133_PERIOD_MS),
		      K_MSEC(CONFIG_INPUT_CF1133_PERIOD_MS));
#endif

	ret = cf1133_ts_init(dev);
	if (ret < 0) {
		LOG_ERR("Init information of sensor failed: %d", ret);
		return ret;
	}
	return 0;
}

#define CF1133_INIT(index)								\
	static const struct cf1133_config cf1133_config_##index = {			\
		.bus = I2C_DT_SPEC_INST_GET(index),					\
		IF_ENABLED(CONFIG_INPUT_CF1133_INTERRUPT,				\
		(.int_gpio = GPIO_DT_SPEC_INST_GET(index, int_gpios),			\
		))									\
		};									\
	static struct cf1133_data cf1133_data_##index;					\
											\
	DEVICE_DT_INST_DEFINE(index, cf1133_init, NULL, &cf1133_data_##index,		\
			      &cf1133_config_##index, POST_KERNEL,			\
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(CF1133_INIT);
