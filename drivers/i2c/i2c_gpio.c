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
 * The SCL pin should be configured in the same manner as SDA, or, if it is
 * known that the hardware attached to pin doesn't attempt clock stretching,
 * then the SCL pin may be a push/pull output.
 */

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_gpio);

#include "i2c-priv.h"
#include "i2c_bitbang.h"

/* Driver config */
struct i2c_gpio_config {
	struct gpio_dt_spec scl_gpio;
	struct gpio_dt_spec sda_gpio;
	uint32_t bitrate;
};

/* Driver instance data */
struct i2c_gpio_context {
	struct i2c_bitbang bitbang;	/* Bit-bang library data */
	struct k_mutex mutex;
};

static void i2c_gpio_set_scl(void *io_context, int state)
{
	const struct i2c_gpio_config *config = io_context;

	gpio_pin_set_dt(&config->scl_gpio, state);
}

static void i2c_gpio_set_sda(void *io_context, int state)
{
	const struct i2c_gpio_config *config = io_context;

	gpio_pin_set_dt(&config->sda_gpio, state);
}

static int i2c_gpio_get_sda(void *io_context)
{
	const struct i2c_gpio_config *config = io_context;
	int rc = gpio_pin_get_dt(&config->sda_gpio);

	/* Default high as that would be a NACK */
	return rc != 0;
}

static const struct i2c_bitbang_io io_fns = {
	.set_scl = &i2c_gpio_set_scl,
	.set_sda = &i2c_gpio_set_sda,
	.get_sda = &i2c_gpio_get_sda,
};

static int i2c_gpio_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_gpio_context *context = dev->data;
	int rc;

	k_mutex_lock(&context->mutex, K_FOREVER);

	rc = i2c_bitbang_configure(&context->bitbang, dev_config);

	k_mutex_unlock(&context->mutex);

	return rc;
}

static int i2c_gpio_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_gpio_context *context = dev->data;
	int rc;

	k_mutex_lock(&context->mutex, K_FOREVER);

	rc = i2c_bitbang_get_config(&context->bitbang, config);
	if (rc < 0) {
		LOG_ERR("I2C controller not configured: %d", rc);
	}

	k_mutex_unlock(&context->mutex);

	return rc;
}

static int i2c_gpio_transfer(const struct device *dev, struct i2c_msg *msgs,
				uint8_t num_msgs, uint16_t slave_address)
{
	struct i2c_gpio_context *context = dev->data;
	int rc;

	k_mutex_lock(&context->mutex, K_FOREVER);

	rc = i2c_bitbang_transfer(&context->bitbang, msgs, num_msgs,
				    slave_address);

	k_mutex_unlock(&context->mutex);

	return rc;
}

static int i2c_gpio_recover_bus(const struct device *dev)
{
	struct i2c_gpio_context *context = dev->data;
	int rc;

	k_mutex_lock(&context->mutex, K_FOREVER);

	rc = i2c_bitbang_recover_bus(&context->bitbang);

	k_mutex_unlock(&context->mutex);

	return rc;
}

static DEVICE_API(i2c, api) = {
	.configure = i2c_gpio_configure,
	.get_config = i2c_gpio_get_config,
	.transfer = i2c_gpio_transfer,
	.recover_bus = i2c_gpio_recover_bus,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

static int i2c_gpio_init(const struct device *dev)
{
	struct i2c_gpio_context *context = dev->data;
	const struct i2c_gpio_config *config = dev->config;
	uint32_t bitrate_cfg;
	int err;

	if (!gpio_is_ready_dt(&config->scl_gpio)) {
		LOG_ERR("SCL GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->scl_gpio, GPIO_OUTPUT_HIGH);
	if (err) {
		LOG_ERR("failed to configure SCL GPIO pin (err %d)", err);
		return err;
	}

	if (!gpio_is_ready_dt(&config->sda_gpio)) {
		LOG_ERR("SDA GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->sda_gpio,
				    GPIO_INPUT | GPIO_OUTPUT_HIGH);
	if (err == -ENOTSUP) {
		err = gpio_pin_configure_dt(&config->sda_gpio,
					    GPIO_OUTPUT_HIGH);
	}
	if (err) {
		LOG_ERR("failed to configure SDA GPIO pin (err %d)", err);
		return err;
	}

	i2c_bitbang_init(&context->bitbang, &io_fns, (void *)config);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);
	err = i2c_bitbang_configure(&context->bitbang,
				    I2C_MODE_CONTROLLER | bitrate_cfg);
	if (err) {
		LOG_ERR("failed to configure I2C bitbang (err %d)", err);
		return err;
	}

	err = k_mutex_init(&context->mutex);
	if (err) {
		LOG_ERR("Failed to create the i2c lock mutex : %d", err);
		return err;
	}

	return 0;
}

#define	DEFINE_I2C_GPIO(_num)						\
									\
static struct i2c_gpio_context i2c_gpio_dev_data_##_num;		\
									\
static const struct i2c_gpio_config i2c_gpio_dev_cfg_##_num = {		\
	.scl_gpio	= GPIO_DT_SPEC_INST_GET(_num, scl_gpios),	\
	.sda_gpio	= GPIO_DT_SPEC_INST_GET(_num, sda_gpios),	\
	.bitrate	= DT_INST_PROP(_num, clock_frequency),		\
};									\
									\
I2C_DEVICE_DT_INST_DEFINE(_num,						\
	    i2c_gpio_init,						\
	    NULL,							\
	    &i2c_gpio_dev_data_##_num,					\
	    &i2c_gpio_dev_cfg_##_num,					\
	    POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_I2C_GPIO)
