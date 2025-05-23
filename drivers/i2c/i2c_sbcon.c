/*
 * Copyright (c) 2017 Linaro Ltd.
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_versatile_i2c

/**
 * @file
 * @brief Driver for ARM's SBCon 2-wire serial bus interface
 *
 * SBCon is a simple device which allows directly setting and getting the
 * hardware state of two-bit serial interfaces like I2C.
 */

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_sbcon, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"
#include "i2c_bitbang.h"

/* SBCon hardware registers layout */
struct sbcon {
	union {
		volatile uint32_t SB_CONTROLS; /* Write to set pins high */
		volatile uint32_t SB_CONTROL;  /* Read for state of pins */
	};
	volatile uint32_t SB_CONTROLC;	/* Write to set pins low */
};

/* Bits values for SCL and SDA lines in struct sbcon registers */
#define SCL BIT(0)
#define SDA BIT(1)

/* Driver config */
struct i2c_sbcon_config {
	struct sbcon *sbcon;		/* Address of hardware registers */
	uint32_t bitrate;		/* I2C bus speed in Hz */
	const struct pinctrl_dev_config *pctrl;
};

/* Driver instance data */
struct i2c_sbcon_context {
	struct i2c_bitbang bitbang;	/* Bit-bang library data */
};

static void i2c_sbcon_set_scl(void *io_context, int state)
{
	struct sbcon *sbcon = io_context;

	if (state) {
		sbcon->SB_CONTROLS = SCL;
	} else {
		sbcon->SB_CONTROLC = SCL;
	}
}

static void i2c_sbcon_set_sda(void *io_context, int state)
{
	struct sbcon *sbcon = io_context;

	if (state) {
		sbcon->SB_CONTROLS = SDA;
	} else {
		sbcon->SB_CONTROLC = SDA;
	}
}

static int i2c_sbcon_get_sda(void *io_context)
{
	struct sbcon *sbcon = io_context;

	return sbcon->SB_CONTROL & SDA;
}

static const struct i2c_bitbang_io io_fns = {
	.set_scl = &i2c_sbcon_set_scl,
	.set_sda = &i2c_sbcon_set_sda,
	.get_sda = &i2c_sbcon_get_sda,
};

static int i2c_sbcon_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_sbcon_context *context = dev->data;

	return i2c_bitbang_configure(&context->bitbang, dev_config);
}

static int i2c_sbcon_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_sbcon_context *context = dev->data;

	return i2c_bitbang_get_config(&context->bitbang, config);
}

static int i2c_sbcon_transfer(const struct device *dev, struct i2c_msg *msgs,
				uint8_t num_msgs, uint16_t slave_address)
{
	struct i2c_sbcon_context *context = dev->data;

	return i2c_bitbang_transfer(&context->bitbang, msgs, num_msgs,
							slave_address);
}

static int i2c_sbcon_recover_bus(const struct device *dev)
{
	struct i2c_sbcon_context *context = dev->data;

	return i2c_bitbang_recover_bus(&context->bitbang);
}

static DEVICE_API(i2c, api) = {
	.configure = i2c_sbcon_configure,
	.get_config = i2c_sbcon_get_config,
	.transfer = i2c_sbcon_transfer,
	.recover_bus = i2c_sbcon_recover_bus,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

static int i2c_sbcon_init(const struct device *dev)
{
	struct i2c_sbcon_context *context = dev->data;
	const struct i2c_sbcon_config *config = dev->config;
	int ret;
	ret = pinctrl_apply_state(config->pctrl, PINCTRL_STATE_DEFAULT);

	/* some pins are not available externally so,
	 * ignore if there is no entry for them
	 */
	if (ret != -ENOENT) {
		return ret;
	}
	i2c_bitbang_init(&context->bitbang, &io_fns, config->sbcon);

	ret = i2c_bitbang_configure(&context->bitbang,
				    I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate));
	if (ret != 0) {
		LOG_ERR("failed to configure I2C bitbang: %d", ret);
	}
	return ret;
}

#define DEFINE_I2C_SBCON(_num)							\
	PINCTRL_DT_INST_DEFINE(_num);						\
	static struct i2c_sbcon_context i2c_sbcon_dev_data_##_num;		\
										\
	static const struct i2c_sbcon_config i2c_sbcon_dev_cfg_##_num = {	\
		.sbcon		= (void *)DT_INST_REG_ADDR(_num),		\
		.bitrate	= DT_INST_PROP(_num, clock_frequency),		\
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(_num),			\
	};									\
										\
	I2C_DEVICE_DT_INST_DEFINE(_num,						\
		    i2c_sbcon_init,						\
		    NULL,							\
		    &i2c_sbcon_dev_data_##_num,					\
		    &i2c_sbcon_dev_cfg_##_num,					\
		    PRE_KERNEL_2, CONFIG_I2C_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_I2C_SBCON)
