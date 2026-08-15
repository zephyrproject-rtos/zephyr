/*
 * Copyright (c) 2024 Dylan Rowe <dylanthomasrowe@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hynitron_cst3530

#include <zephyr/sys/byteorder.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

LOG_MODULE_REGISTER(cst3530, CONFIG_INPUT_LOG_LEVEL);

#define CST3530_DATA_REG     0xD0070000
#define CST3530_END_READ_REG 0xD00002AB

#define CST3530_RESET_DELAY_MS 10
#define CST3530_WAIT_DELAY_MS  200

struct cst3530_config {
	struct i2c_dt_spec i2c;
	const struct gpio_dt_spec rst_gpio;
	const struct gpio_dt_spec int_gpio;
};

struct cst3530_data {
	const struct device *dev;
	struct k_work work;
	struct gpio_callback int_gpio_cb;
};

static int cst3530_read_reg32(const struct i2c_dt_spec *i2c, uint32_t reg, uint8_t *data,
			      size_t len)
{
	uint8_t addr[4];

	addr[0] = (uint8_t)(reg >> 24);
	addr[1] = (uint8_t)(reg >> 16);
	addr[2] = (uint8_t)(reg >> 8);
	addr[3] = (uint8_t)(reg & 0xFF);

	return i2c_write_read_dt(i2c, addr, sizeof(addr), data, len);
}

static int cst3530_write_reg32(const struct i2c_dt_spec *i2c, uint32_t reg)
{
	uint8_t addr[4];

	addr[0] = (uint8_t)(reg >> 24);
	addr[1] = (uint8_t)(reg >> 16);
	addr[2] = (uint8_t)(reg >> 8);
	addr[3] = (uint8_t)(reg & 0xFF);

	return i2c_write_dt(i2c, addr, sizeof(addr));
}

static int cst3530_process(const struct device *dev)
{
	const struct cst3530_config *cfg = dev->config;
	int ret;
	uint8_t buf[9] = {0};
	uint8_t count;
	uint16_t x;
	uint16_t y;

	ret = cst3530_read_reg32(&cfg->i2c, CST3530_DATA_REG, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Could not read data (%d)", ret);
		return ret;
	}

	count = buf[3] & 0x0F;
	if (count == 0 || (buf[8] & 0xF0) == 0x00) {
		cst3530_write_reg32(&cfg->i2c, CST3530_END_READ_REG);
		input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
		return 0;
	}

	cst3530_write_reg32(&cfg->i2c, CST3530_END_READ_REG);

	x = ((uint16_t)(buf[7] & 0x0F) << 8) | buf[4];
	y = ((uint16_t)(buf[7] & 0xF0) << 4) | buf[5];

	LOG_DBG("touch raw x=%u y=%u (points=%u)", x, y, count);

	input_report_abs(dev, INPUT_ABS_X, x, false, K_FOREVER);
	input_report_abs(dev, INPUT_ABS_Y, y, false, K_FOREVER);
	input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);

	return 0;
}

static void cst3530_work_handler(struct k_work *work)
{
	struct cst3530_data *data = CONTAINER_OF(work, struct cst3530_data, work);

	cst3530_process(data->dev);
}

static void cst3530_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct cst3530_data *data = CONTAINER_OF(cb, struct cst3530_data, int_gpio_cb);

	k_work_submit(&data->work);
}

static void cst3530_chip_reset(const struct device *dev)
{
	const struct cst3530_config *config = dev->config;
	int ret;

	if (gpio_is_ready_dt(&config->rst_gpio)) {
		ret = gpio_pin_configure_dt(&config->rst_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO pin (%d)", ret);
			return;
		}
		k_msleep(CST3530_RESET_DELAY_MS);
		gpio_pin_set_dt(&config->rst_gpio, 0);
		k_msleep(CST3530_WAIT_DELAY_MS);
	}
}

static int cst3530_init(const struct device *dev)
{
	struct cst3530_data *data = dev->data;
	const struct cst3530_config *config = dev->config;
	int ret;

	data->dev = dev;
	k_work_init(&data->work, cst3530_work_handler);

	cst3530_chip_reset(dev);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus %s not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Interrupt GPIO %s not ready", config->int_gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO interrupt (%d)", ret);
		return ret;
	}

	gpio_init_callback(&data->int_gpio_cb, cst3530_isr_handler, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback (%d)", ret);
		return ret;
	}

	return 0;
}

#define CST3530_DEFINE(index)                                                                      \
	static struct cst3530_data cst3530_data_##index;                                           \
	static const struct cst3530_config cst3530_config_##index = {                              \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		.rst_gpio = GPIO_DT_SPEC_INST_GET_OR(index, reset_gpios, {}),                      \
		.int_gpio = GPIO_DT_SPEC_INST_GET(index, irq_gpios),                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, cst3530_init, NULL, &cst3530_data_##index,                    \
			      &cst3530_config_##index, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,    \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(CST3530_DEFINE)
