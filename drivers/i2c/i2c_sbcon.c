/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for ARM's SBCon 2-wire serial bus interface
 *
 * SBCon is a simple device which allows directly setting and getting the
 * hardware state of two-bit serial interfaces like I2C.
 */

#include <board.h>
#include <device.h>
#include <errno.h>
#include <i2c.h>
#include "i2c_bitbang.h"

/* SBCon hardware registers layout */
struct sbcon {
	union {
		volatile u32_t SB_CONTROLS; /* Write to set pins high */
		volatile u32_t SB_CONTROL;  /* Read for state of pins */
	};
	volatile u32_t SB_CONTROLC;	/* Write to set pins low */
};

/* Bits values for SCL and SDA lines in struct sbcon registers */
#define SCL BIT(0)
#define SDA BIT(1)

/* Driver config */
struct i2c_sbcon_config {
	struct sbcon *sbcon;		/* Address of hardware registers */
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

static int i2c_sbcon_configure(struct device *dev, u32_t dev_config)
{
	struct i2c_sbcon_context *context = dev->driver_data;

	return i2c_bitbang_configure(&context->bitbang, dev_config);
}

static int i2c_sbcon_transfer(struct device *dev, struct i2c_msg *msgs,
				u8_t num_msgs, u16_t slave_address)
{
	struct i2c_sbcon_context *context = dev->driver_data;

	return i2c_bitbang_transfer(&context->bitbang, msgs, num_msgs,
							slave_address);
}

static struct i2c_driver_api api = {
	.configure = i2c_sbcon_configure,
	.transfer = i2c_sbcon_transfer,
};

static int i2c_sbcon_init(struct device *dev)
{
	struct i2c_sbcon_context *context = dev->driver_data;
	const struct i2c_sbcon_config *config = dev->config->config_info;

	i2c_bitbang_init(&context->bitbang, &io_fns, config->sbcon);

	dev->driver_api = &api;

	return 0;
}

#define	DEFINE_I2C_SBCON(_num)						\
									\
static struct i2c_sbcon_context i2c_sbcon_dev_data_##_num;		\
									\
static const struct i2c_sbcon_config i2c_sbcon_dev_cfg_##_num = {	\
	.sbcon		= (void *)I2C_SBCON_##_num##_BASE_ADDR,		\
};									\
									\
DEVICE_INIT(i2c_sbcon_##_num, CONFIG_I2C_SBCON_##_num##_NAME,		\
	    i2c_sbcon_init,						\
	    &i2c_sbcon_dev_data_##_num,					\
	    &i2c_sbcon_dev_cfg_##_num,					\
	    PRE_KERNEL_2, CONFIG_I2C_INIT_PRIORITY)

#ifdef CONFIG_I2C_SBCON_0
DEFINE_I2C_SBCON(0);
#endif

#ifdef CONFIG_I2C_SBCON_1
DEFINE_I2C_SBCON(1);
#endif

#ifdef CONFIG_I2C_SBCON_2
DEFINE_I2C_SBCON(2);
#endif

#ifdef CONFIG_I2C_SBCON_3
DEFINE_I2C_SBCON(3);
#endif
