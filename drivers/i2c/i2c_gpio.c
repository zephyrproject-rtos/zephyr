/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for software driven I2C using GPIO lines
 *
 * This driver implements an I2C interface by driving two GPIO lines under
 * software control. The GPIO pins used must be preconfigured to to a suitable
 * state, i.e. the SDA pin as open-colector/open-drain with a pull-up resistor
 * (possibly as an external component attached to the pin). When the SDA pin
 * is read it must return the state of the physical hardware line, not just the
 * last state written to it for output.
 *
 * The SCL pin should be configured in the same manner as SDA, or, if it is known
 * that the hardware attached to pin doesn't attempt clock stretching, then the
 * SCL pin may be a push/pull output.
 */

#include <device.h>
#include <errno.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include "i2c_bitbang.h"

/* Driver config */
struct i2c_gpio_config {
	char *gpio_name;
	u8_t scl_pin;
	u8_t sda_pin;
};

/* Driver instance data */
struct i2c_gpio_context {
	struct i2c_bitbang bitbang;	/* Bit-bang library data */
	struct device *gpio;		/* GPIO used for I2C lines */
	u8_t scl_pin;			/* Pin on gpio used for SCL line */
	u8_t sda_pin;			/* Pin on gpio used for SDA line */
};

static void i2c_gpio_set_scl(void *io_context, int state)
{
	struct i2c_gpio_context *context = io_context;

	gpio_pin_write(context->gpio, context->scl_pin, state);
}

static void i2c_gpio_set_sda(void *io_context, int state)
{
	struct i2c_gpio_context *context = io_context;

	gpio_pin_write(context->gpio, context->sda_pin, state);
}

static int i2c_gpio_get_sda(void *io_context)
{
	struct i2c_gpio_context *context = io_context;
	u32_t state = 1U; /* Default high as that would be a NACK */

	gpio_pin_read(context->gpio, context->sda_pin, &state);
	return state;
}

static const struct i2c_bitbang_io io_fns = {
	.set_scl = &i2c_gpio_set_scl,
	.set_sda = &i2c_gpio_set_sda,
	.get_sda = &i2c_gpio_get_sda,
};

static int i2c_gpio_configure(struct device *dev, u32_t dev_config)
{
	struct i2c_gpio_context *context = dev->driver_data;

	return i2c_bitbang_configure(&context->bitbang, dev_config);
}

static int i2c_gpio_transfer(struct device *dev, struct i2c_msg *msgs,
				u8_t num_msgs, u16_t slave_address)
{
	struct i2c_gpio_context *context = dev->driver_data;

	return i2c_bitbang_transfer(&context->bitbang, msgs, num_msgs,
							slave_address);
}

static struct i2c_driver_api api = {
	.configure = i2c_gpio_configure,
	.transfer = i2c_gpio_transfer,
};

static int i2c_gpio_init(struct device *dev)
{
	struct i2c_gpio_context *context = dev->driver_data;
	const struct i2c_gpio_config *config = dev->config->config_info;

	context->gpio = device_get_binding(config->gpio_name);
	if (!context->gpio) {
		return -EINVAL;
	}
	context->sda_pin = config->sda_pin;
	context->scl_pin = config->scl_pin;

	i2c_bitbang_init(&context->bitbang, &io_fns, context);

	return 0;
}

#define	DEFINE_I2C_GPIO(_num)						\
									\
static struct i2c_gpio_context i2c_gpio_dev_data_##_num;		\
									\
static const struct i2c_gpio_config i2c_gpio_dev_cfg_##_num = {		\
	.gpio_name	= CONFIG_I2C_GPIO_##_num##_GPIO,		\
	.scl_pin	= CONFIG_I2C_GPIO_##_num##_SCL_PIN,		\
	.sda_pin	= CONFIG_I2C_GPIO_##_num##_SDA_PIN,		\
};									\
									\
DEVICE_AND_API_INIT(i2c_gpio_##_num, CONFIG_I2C_GPIO_##_num##_NAME,	\
	    i2c_gpio_init,						\
	    &i2c_gpio_dev_data_##_num,					\
	    &i2c_gpio_dev_cfg_##_num,					\
	    PRE_KERNEL_2, CONFIG_I2C_INIT_PRIORITY, &api)

#ifdef CONFIG_I2C_GPIO_0
DEFINE_I2C_GPIO(0);
#endif

#ifdef CONFIG_I2C_GPIO_1
DEFINE_I2C_GPIO(1);
#endif

#ifdef CONFIG_I2C_GPIO_2
DEFINE_I2C_GPIO(2);
#endif

#ifdef CONFIG_I2C_GPIO_3
DEFINE_I2C_GPIO(3);
#endif
