/*
 * Copyright (c) 2025 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT chipsemi_chsc5x

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_touch.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

LOG_MODULE_REGISTER(chsc5x, CONFIG_INPUT_LOG_LEVEL);

struct chsc5x_config {
	struct input_touchscreen_common_config common;
	struct i2c_dt_spec i2c;
	const struct gpio_dt_spec int_gpio;
	const struct gpio_dt_spec reset_gpio;
};

struct chsc5x_data {
	const struct device *dev;
	struct k_work work;
	struct gpio_callback int_gpio_cb;
};

enum {
	CHSC5X_IC_TYPE_CHSC5472 = 0x00,
	CHSC5X_IC_TYPE_CHSC5448 = 0x01,
	CHSC5X_IC_TYPE_CHSC5448A = 0x02,
	CHSC5X_IC_TYPE_CHSC5460 = 0x03,
	CHSC5X_IC_TYPE_CHSC5468 = 0x04,
	CHSC5X_IC_TYPE_CHSC5432 = 0x05,
	CHSC5X_IC_TYPE_CHSC5816 = 0x10,
	CHSC5X_IC_TYPE_CHSC1716 = 0x11,
};

#define CHSC5X_BASE_ADDR1         0x20
#define CHSC5X_BASE_ADDR2         0x00
#define CHSC5X_BASE_ADDR3         0x00
#define CHSC5X_ADDRESS_MODE       0x00
#define CHSC5X_ADDRESS_IC_TYPE    0x81
#define CHSC5X_ADDRESS_TOUCH_DATA 0x2C
#define CHSC5X_SIZE_TOUCH_DATA    7

#define CHSC5X_OFFSET_EVENT_TYPE    0x00
#define CHSC5X_OFFSET_FINGER_NUMBER 0x01
#define CHSC5X_OFFSET_X_COORDINATE  0x02
#define CHSC5X_OFFSET_Y_COORDINATE  0x03
#define CHSC5X_OFFSET_PRESSURE      0x04
#define CHSC5X_OFFSET_XY_COORDINATE 0x05
#define CHSC5X_OFFSET_TOUCH_EVENT   0x06

static void chsc5x_work_handler(struct k_work *work)
{
	struct chsc5x_data *data = CONTAINER_OF(work, struct chsc5x_data, work);
	const struct device *dev = data->dev;
	const struct chsc5x_config *cfg = dev->config;
	uint16_t row, col;
	bool is_pressed;
	int ret;
	const uint8_t write_buffer[] = {
		CHSC5X_BASE_ADDR1,
		CHSC5X_BASE_ADDR2,
		CHSC5X_BASE_ADDR3,
		CHSC5X_ADDRESS_TOUCH_DATA,
	};
	uint8_t read_buffer[CHSC5X_SIZE_TOUCH_DATA];

	ret = i2c_write_read_dt(&cfg->i2c, write_buffer, sizeof(write_buffer), read_buffer,
				sizeof(read_buffer));
	if (ret < 0) {
		LOG_ERR("Could not read data: %i", ret);
		return;
	}

	is_pressed = (read_buffer[CHSC5X_OFFSET_TOUCH_EVENT] & 0x40) == 0U;
	col = read_buffer[CHSC5X_OFFSET_X_COORDINATE] +
	      ((read_buffer[CHSC5X_OFFSET_XY_COORDINATE] & 0x0f) << 8);
	row = read_buffer[CHSC5X_OFFSET_Y_COORDINATE] +
	      ((read_buffer[CHSC5X_OFFSET_XY_COORDINATE] & 0xf0) << 4);

	if (is_pressed) {
		input_touchscreen_report_pos(dev, col, row, K_FOREVER);
	}

	input_report_key(dev, INPUT_BTN_TOUCH, is_pressed, true, K_FOREVER);
}

static void chsc5x_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t mask)
{
	struct chsc5x_data *data = CONTAINER_OF(cb, struct chsc5x_data, int_gpio_cb);

	k_work_submit(&data->work);
}

static int chsc5x_chip_init(const struct device *dev)
{
	const struct chsc5x_config *cfg = dev->config;
	uint8_t ic_type;
	int ret;
	const uint8_t write_buffer[] = {
		CHSC5X_BASE_ADDR1,
		CHSC5X_BASE_ADDR2,
		CHSC5X_BASE_ADDR3,
		CHSC5X_ADDRESS_IC_TYPE,
	};

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus %s not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	ret = i2c_write_read_dt(&cfg->i2c, write_buffer, sizeof(write_buffer), &ic_type, 1);
	if (ret < 0) {
		LOG_ERR("Could not read data: %i", ret);
		return ret;
	}

	switch (ic_type) {
	case CHSC5X_IC_TYPE_CHSC5472:
	case CHSC5X_IC_TYPE_CHSC5448:
	case CHSC5X_IC_TYPE_CHSC5448A:
	case CHSC5X_IC_TYPE_CHSC5460:
	case CHSC5X_IC_TYPE_CHSC5468:
	case CHSC5X_IC_TYPE_CHSC5432:
	case CHSC5X_IC_TYPE_CHSC5816:
	case CHSC5X_IC_TYPE_CHSC1716:
		break;
	default:
		LOG_ERR("CHSC5X wrong ic type: returned 0x%02x", ic_type);
		return -ENODEV;
	}

	return 0;
}

static int chsc5x_reset(const struct device *dev)
{
	const struct chsc5x_config *config = dev->config;
	int ret;

	if (config->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset_gpio)) {
			LOG_ERR("GPIO port %s not ready", config->reset_gpio.port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", ret);
			return ret;
		}

		k_usleep(500);

		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (ret < 0) {
			LOG_ERR("Could not pull reset low (%d)", ret);
			return ret;
		}

		k_msleep(1);
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int chsc5x_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct chsc5x_config *config = dev->config;
	int ret;

	if (config->reset_gpio.port == NULL) {
		return -ENOTSUP;
	}

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = chsc5x_reset(dev);
		break;

	case PM_DEVICE_ACTION_SUSPEND: {
		const uint8_t write_buffer[] = {
			CHSC5X_BASE_ADDR1,
			CHSC5X_BASE_ADDR2,
			CHSC5X_BASE_ADDR3,
			CHSC5X_ADDRESS_MODE,
			/* Fixed sequence with checksum */
			0xF7, 0x16, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE9,
		};

		ret = i2c_write_dt(&config->i2c, write_buffer, sizeof(write_buffer));
		break;
	}

	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int chsc5x_init(const struct device *dev)
{
	const struct chsc5x_config *config = dev->config;
	struct chsc5x_data *data = dev->data;
	int ret;

	data->dev = dev;

	k_work_init(&data->work, chsc5x_work_handler);

	ret = chsc5x_reset(dev);
	if (ret < 0) {
		LOG_ERR("Failed to reset (%d)", ret);
		return ret;
	}

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("GPIO port %s not ready", config->int_gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO interrupt: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->int_gpio_cb, chsc5x_isr_handler, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback: %d", ret);
		return ret;
	}

	return chsc5x_chip_init(dev);
};

#define CHSC5X_DEFINE(index)                                                                       \
	PM_DEVICE_DT_INST_DEFINE(inst, chsc5x_pm_action);                                          \
	static const struct chsc5x_config chsc5x_config_##index = {                                \
		.common = INPUT_TOUCH_DT_INST_COMMON_CONFIG_INIT(index),                           \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		.int_gpio = GPIO_DT_SPEC_INST_GET(index, int_gpios),                               \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(index, reset_gpios, {0}),                   \
	};                                                                                         \
	static struct chsc5x_data chsc5x_data_##index;                                             \
	DEVICE_DT_INST_DEFINE(index, chsc5x_init, PM_DEVICE_DT_INST_GET(inst),                     \
			      &chsc5x_data_##index, &chsc5x_config_##index, POST_KERNEL,           \
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(CHSC5X_DEFINE)
