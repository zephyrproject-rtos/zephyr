/*
 * Copyright (c) 2024 Shen Xuyang <shenxuyang@shlinyuantech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT istech_ist3931

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
LOG_MODULE_REGISTER(ist3931);

#include "display_ist3931.h"

struct ist3931_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
	uint16_t height;
	uint16_t width;
	bool vc;      /* voltage_converter_circuits_enabled */
	bool vf;      /* voltage_follower_circuits_enabled */
	uint8_t bias; /* 0-7 */
	uint8_t ct;   /* 0-255 */
	uint8_t duty; /* 1-64 */
	uint16_t fr;  /* frame_frequency_division */
	bool shl;     /* 0 COM1->COMN 1 COMN->COM1 */
	bool adc;     /* 0 seg1->seg132 1 seg132->seg1 */
	bool eon;     /* 0 normal 1 Entire ON */
	bool rev;     /* 0 RAM1->LCD ON 1 RAM0->LCD ON */
	uint8_t x_offset;
	uint8_t y_offset;
};

static int ist3931_write_bus(const struct device *dev, uint8_t *buf, bool command,
			     uint32_t num_bytes)
{
	const struct ist3931_config *config = dev->config;
	const uint8_t control_byte = command ? IST3931_CMD_BYTE : IST3931_DATA_BYTE;
	uint8_t i2c_write_buf[IST3931_RAM_WIDTH / 4];

	for (uint32_t i = 0; i < num_bytes; i++) {
		i2c_write_buf[i * 2] = control_byte;
		i2c_write_buf[i * 2 + 1] = buf[i];
	};
	return i2c_write_dt(&config->bus, i2c_write_buf, num_bytes * 2);
};

static inline bool ist3931_bus_ready(const struct device *dev)
{
	const struct ist3931_config *config = dev->config;

	return i2c_is_ready_dt(&config->bus);
};

static inline int ist3931_set_power(const struct device *dev)
{
	const struct ist3931_config *config = dev->config;
	uint8_t cmd_buf = IST3931_CMD_POWER_CONTROL | config->vc | config->vf << 1;

	return ist3931_write_bus(dev, &cmd_buf, 1, 1);
};

static inline int ist3931_set_bias(const struct device *dev)
{
	const struct ist3931_config *config = dev->config;
	uint8_t cmd_buf = IST3931_CMD_BIAS | config->bias;

	return ist3931_write_bus(dev, &cmd_buf, 1, 1);
};

static inline int ist3931_set_ct(const struct device *dev)
{
	const struct ist3931_config *config = dev->config;
	uint8_t cmd_buf[2] = {IST3931_CMD_CT, config->ct};

	return ist3931_write_bus(dev, cmd_buf, 1, 2);
};

static inline int ist3931_set_fr(const struct device *dev)
{
	const struct ist3931_config *config = dev->config;
	uint8_t cmd_buf[3] = {IST3931_CMD_FRAME_CONTROL, config->fr & 0x00FF, config->fr >> 8};

	return ist3931_write_bus(dev, cmd_buf, 1, 3);
};

static inline int ist3931_set_duty(const struct device *dev)
{
	const struct ist3931_config *config = dev->config;
	uint8_t cmd_buf[2] = {(IST3931_CMD_SET_DUTY_LSB | (config->duty & 0x0F)),
			      (IST3931_CMD_SET_DUTY_MSB | (config->duty >> 4))};

	return ist3931_write_bus(dev, cmd_buf, 1, 2);
};

static inline int ist3931_driver_display_control(const struct device *dev)
{
	const struct ist3931_config *config = dev->config;
	uint8_t cmd_buf = IST3931_CMD_DRIVER_DISPLAY_CONTROL | config->shl << 3 | config->adc << 2 |
			  config->eon << 1 | config->rev;

	return ist3931_write_bus(dev, &cmd_buf, 1, 1);
};

static inline int ist3931_driver_set_display_on(const struct device *dev)
{
	uint8_t cmd_buf = IST3931_CMD_DISPLAY_ON_OFF | 1;

	return ist3931_write_bus(dev, &cmd_buf, 1, 1);
};

static inline int ist3931_driver_sleep_on_off(const struct device *dev, const bool sleep)
{
	uint8_t cmd_buf = IST3931_CMD_SLEEP_MODE | sleep;

	return ist3931_write_bus(dev, &cmd_buf, 1, 1);
};

static inline int ist3931_driver_set_com_pad_map(const struct device *dev)
{
	uint8_t cmd_buf[5] = {
		IST3931_CMD_IST_COMMAND_ENTRY, IST3931_CMD_IST_COMMAND_ENTRY,
		IST3931_CMD_IST_COMMAND_ENTRY, IST3931_CMD_IST_COMMAND_ENTRY,
		IST3931_CMD_IST_COM_MAPPING,
	};
	uint8_t cmd_buf2 = IST3931_CMD_EXIT_ENTRY;

	ist3931_write_bus(dev, &cmd_buf[0], 1, 5);
	k_sleep(K_MSEC(IST3931_CMD_DELAY));
	ist3931_write_bus(dev, &cmd_buf2, 1, 1);
	return 0;
};

static inline int ist3931_driver_set_ay(const struct device *dev, uint16_t y)
{
	const struct ist3931_config *config = dev->config;
	uint8_t cmd_buf1 = IST3931_CMD_SET_AY_ADD_LSB | ((config->y_offset + y) & 0x0F);
	uint8_t cmd_buf2 = IST3931_CMD_SET_AY_ADD_MSB | ((config->y_offset + y) >> 4);
	uint8_t cmd_buf[2] = {cmd_buf1, cmd_buf2};

	return ist3931_write_bus(dev, &cmd_buf[0], 1, 2);
};

static inline int ist3931_driver_set_ax(const struct device *dev, uint8_t x)
{
	const struct ist3931_config *config = dev->config;
	uint8_t cmd_buf = IST3931_CMD_SET_AX_ADD | (config->x_offset + x);

	return ist3931_write_bus(dev, &cmd_buf, 1, 1);
};

static int ist3931_init_device(const struct device *dev)
{
	const struct ist3931_config *config = dev->config;

	gpio_pin_set_dt(&config->reset_gpio, 1);
	k_sleep(K_MSEC(IST3931_RESET_DELAY));
	gpio_pin_set_dt(&config->reset_gpio, 0);
	k_sleep(K_MSEC(IST3931_RESET_DELAY));
	gpio_pin_set_dt(&config->reset_gpio, 1);
	k_sleep(K_MSEC(IST3931_RESET_DELAY));
	ist3931_set_power(dev);
	ist3931_set_bias(dev);
	ist3931_set_ct(dev);
	ist3931_set_fr(dev);
	ist3931_set_duty(dev);
	ist3931_driver_display_control(dev);
	ist3931_driver_set_display_on(dev);
	ist3931_driver_set_com_pad_map(dev);
	return 0;
}

static int ist3931_init(const struct device *dev)
{
	const struct ist3931_config *config = dev->config;
	int ret;

	if (ist3931_bus_ready(dev)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}
	LOG_INF("I2C device ready");

	if (!gpio_is_ready_dt(&config->reset_gpio)) {
		LOG_ERR("Reset GPIO device not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Couldn't configure reset pin");
		return ret;
	}
	ret = ist3931_init_device(dev);
	if (ret) {
		LOG_ERR("Failed to initialize device!");
		return ret;
	}

	return 0;
}

static void ist3931_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct ist3931_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10 | PIXEL_FORMAT_MONO01;
	caps->current_pixel_format = PIXEL_FORMAT_MONO01;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int ist3931_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	uint8_t *write_data_start = (uint8_t *)buf;
	int ret;

	ret = ist3931_driver_set_ay(dev, y);
	if (ret < 0) {
		return ret;
	}
	ret = ist3931_driver_set_ax(dev, x);
	if (ret < 0) {
		return ret;
	}

	__ASSERT(x <= IST3931_RAM_WIDTH, "x is out of range");
	__ASSERT(y <= IST3931_RAM_HEIGHT, "y is out of range");
	__ASSERT(x + desc->width <= IST3931_RAM_WIDTH, "x+width is out of range");
	__ASSERT(y + desc->height <= IST3931_RAM_HEIGHT, "y+height is out of range");

	for (uint16_t i = 0; i < desc->height; i++) {
		ist3931_driver_set_ay(dev, i + y);
		ist3931_driver_set_ax(dev, x);
		ist3931_write_bus(dev, write_data_start, 0, desc->width / 8);
		write_data_start += desc->width / 8;
	}
	return 0;
}

static inline int ist3931_blanking_on(const struct device *dev)
{
	return ist3931_driver_sleep_on_off(dev, false);
}

static inline int ist3931_blanking_off(const struct device *dev)
{
	return ist3931_driver_sleep_on_off(dev, true);
}

static const struct display_driver_api ist3931_api = {
	.write = ist3931_write,
	.get_capabilities = ist3931_get_capabilities,
	.blanking_on = ist3931_blanking_on,
	.blanking_off = ist3931_blanking_off,
};

#define IST3931_INIT(inst)                                                                         \
	static const struct ist3931_config ist3931_config_##inst = {                               \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                            \
		.vc = DT_INST_PROP(inst, voltage_converter),                                       \
		.vf = DT_INST_PROP(inst, voltage_follower),                                        \
		.bias = DT_INST_PROP(inst, lcd_bias),                                              \
		.ct = DT_INST_PROP(inst, lcd_ct),                                                  \
		.duty = DT_INST_PROP(inst, duty_ratio),                                            \
		.fr = DT_INST_PROP(inst, frame_control),                                           \
		.shl = DT_INST_PROP(inst, reverse_com_output),                                     \
		.adc = DT_INST_PROP(inst, reverse_seg_driver),                                     \
		.eon = DT_INST_PROP(inst, e_force_on),                                             \
		.rev = DT_INST_PROP(inst, reverse_ram_lcd),                                        \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
		.x_offset = DT_INST_PROP(inst, x_offset),                                          \
		.y_offset = DT_INST_PROP(inst, y_offset),                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &ist3931_init, NULL, NULL, &ist3931_config_##inst,             \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ist3931_api);

DT_INST_FOREACH_STATUS_OKAY(IST3931_INIT)
