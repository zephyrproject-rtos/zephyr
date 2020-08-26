/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_i2c

/**
 * @file
 * @brief Driver for software driven I2C using GPIO lines
 *
 * This driver implements an I2C interface by driving two GPIO lines under
 * software control.
 *
 * The GPIO pins used must be configured (through devicetree and pinmux) with
 * suitable flags, i.e. the SDA pin as open-collector/open-drain with a pull-up
 * resistor (possibly as an external component attached to the pin).
 *
 * When the SDA pin is read it must return the state of the physical hardware
 * line, not just the last state written to it for output.
 *
 * The SCL pin should be configured in the same manner as SDA, or, if it is known
 * that the hardware attached to pin doesn't attempt clock stretching, then the
 * SCL pin may be a push/pull output.
 */

#include <device.h>
#include <errno.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_gpio);

#include "i2c-priv.h"
#include "i2c_bitbang.h"

/* Driver config */
struct i2c_gpio_config {
	const char *scl_gpio_name;
	const char *sda_gpio_name;
	gpio_pin_t scl_pin;
	gpio_pin_t sda_pin;
	gpio_dt_flags_t scl_flags;
	gpio_dt_flags_t sda_flags;
	uint32_t bitrate;
};

/* Driver instance data */
struct i2c_gpio_context {
	struct i2c_bitbang bitbang;	/* Bit-bang library data */
	struct device *scl_gpio;	/* GPIO used for I2C SCL line */
	struct device *sda_gpio;	/* GPIO used for I2C SDA line */
	gpio_pin_t scl_pin;		/* Pin on gpio used for SCL line */
	gpio_pin_t sda_pin;		/* Pin on gpio used for SDA line */
};

static void i2c_gpio_set_scl(void *io_context, int state)
{
	struct i2c_gpio_context *context = io_context;

	gpio_pin_set(context->scl_gpio, context->scl_pin, state);
}

static void i2c_gpio_set_sda(void *io_context, int state)
{
	struct i2c_gpio_context *context = io_context;

	gpio_pin_set(context->sda_gpio, context->sda_pin, state);
}

static int i2c_gpio_get_sda(void *io_context)
{
	struct i2c_gpio_context *context = io_context;
	int rc = gpio_pin_get(context->sda_gpio, context->sda_pin);

	/* Default high as that would be a NACK */
	return rc != 0;
}

static const struct i2c_bitbang_io io_fns = {
	.set_scl = &i2c_gpio_set_scl,
	.set_sda = &i2c_gpio_set_sda,
	.get_sda = &i2c_gpio_get_sda,
};

static int i2c_gpio_configure(struct device *dev, uint32_t dev_config)
{
	struct i2c_gpio_context *context = dev->data;

	return i2c_bitbang_configure(&context->bitbang, dev_config);
}

static int i2c_gpio_transfer(struct device *dev, struct i2c_msg *msgs,
				uint8_t num_msgs, uint16_t slave_address)
{
	struct i2c_gpio_context *context = dev->data;

	return i2c_bitbang_transfer(&context->bitbang, msgs, num_msgs,
				    slave_address);
}

static int i2c_gpio_recover_bus(struct device *dev)
{
	struct i2c_gpio_context *context = dev->data;

	return i2c_bitbang_recover_bus(&context->bitbang);
}

static struct i2c_driver_api api = {
	.configure = i2c_gpio_configure,
	.transfer = i2c_gpio_transfer,
	.recover_bus = i2c_gpio_recover_bus,
};

static int i2c_gpio_init(struct device *dev)
{
	struct i2c_gpio_context *context = dev->data;
	const struct i2c_gpio_config *config = dev->config;
	uint32_t bitrate_cfg;
	int err;

	context->scl_gpio = device_get_binding(config->scl_gpio_name);
	if (!context->scl_gpio) {
		LOG_ERR("failed to get SCL GPIO device");
		return -EINVAL;
	}

	err = gpio_config(context->scl_gpio, config->scl_pin,
			  config->scl_flags | GPIO_OUTPUT_HIGH);
	if (err) {
		LOG_ERR("failed to configure SCL GPIO pin (err %d)", err);
		return err;
	}

	context->sda_gpio = device_get_binding(config->sda_gpio_name);
	if (!context->sda_gpio) {
		LOG_ERR("failed to get SCL GPIO device");
		return -EINVAL;
	}

	err = gpio_config(context->sda_gpio, config->sda_pin,
			  config->sda_flags | GPIO_OUTPUT_HIGH);
	if (err) {
		LOG_ERR("failed to configure SDA GPIO pin (err %d)", err);
		return err;
	}

	context->sda_pin = config->sda_pin;
	context->scl_pin = config->scl_pin;

	i2c_bitbang_init(&context->bitbang, &io_fns, context);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);
	err = i2c_bitbang_configure(&context->bitbang,
				    I2C_MODE_MASTER | bitrate_cfg);
	if (err) {
		LOG_ERR("failed to configure I2C bitbang (err %d)", err);
		return err;
	}

	return 0;
}

#define	DEFINE_I2C_GPIO(_num)						\
									\
static struct i2c_gpio_context i2c_gpio_dev_data_##_num;		\
									\
static const struct i2c_gpio_config i2c_gpio_dev_cfg_##_num = {		\
	.scl_gpio_name	= DT_INST_GPIO_LABEL(_num, scl_gpios),		\
	.sda_gpio_name	= DT_INST_GPIO_LABEL(_num, sda_gpios),		\
	.scl_pin	= DT_INST_GPIO_PIN(_num, scl_gpios),		\
	.sda_pin	= DT_INST_GPIO_PIN(_num, sda_gpios),		\
	.scl_flags	= DT_INST_GPIO_FLAGS(_num, scl_gpios),		\
	.sda_flags	= DT_INST_GPIO_FLAGS(_num, sda_gpios),		\
	.bitrate	= DT_INST_PROP(_num, clock_frequency),		\
};									\
									\
DEVICE_AND_API_INIT(i2c_gpio_##_num, DT_INST_LABEL(_num),		\
	    i2c_gpio_init,						\
	    &i2c_gpio_dev_data_##_num,					\
	    &i2c_gpio_dev_cfg_##_num,					\
	    PRE_KERNEL_2, CONFIG_I2C_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_I2C_GPIO)
