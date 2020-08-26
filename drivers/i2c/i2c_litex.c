/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_i2c

#include <device.h>
#include <drivers/i2c.h>
#include "i2c_bitbang.h"

#define SCL_BIT_POS                0
#define SDA_DIR_BIT_POS            1
#define SDA_BIT_W_POS              2
#define SDA_BIT_R_POS              0

#define SDA_DIR_OUTPUT             1
#define SDA_DIR_INPUT              0

#define HIGH_STATE_ON_I2C_LINES    0x7

struct i2c_litex_cfg {
	volatile uint32_t *w_reg;
	volatile const uint32_t *r_reg;
};

#define GET_I2C_CFG(dev)						     \
	((const struct i2c_litex_cfg *) dev->config)

#define GET_I2C_BITBANG(dev)						     \
	((struct i2c_bitbang *) dev->data)

static inline void set_bit(volatile uint32_t *reg, uint32_t bit, uint32_t val)
{
	uint32_t mask = BIT(bit);

	if (val) {
		*reg |= mask;
	} else {
		*reg &= ~mask;
	}
}

static inline int get_bit(volatile const uint32_t *reg, uint32_t bit)
{
	uint32_t mask = BIT(bit);

	return !!((*reg) & mask);
}

static void i2c_litex_bitbang_set_scl(void *context, int state)
{
	const struct i2c_litex_cfg *config =
		(const struct i2c_litex_cfg *) context;

	set_bit(config->w_reg, SCL_BIT_POS, state);
}

static void i2c_litex_bitbang_set_sda(void *context, int state)
{
	const struct i2c_litex_cfg *config =
		(const struct i2c_litex_cfg *) context;

	set_bit(config->w_reg, SDA_DIR_BIT_POS, SDA_DIR_OUTPUT);
	set_bit(config->w_reg, SDA_BIT_W_POS, state);
}

static int i2c_litex_bitbang_get_sda(void *context)
{
	const struct i2c_litex_cfg *config =
		(const struct i2c_litex_cfg *) context;

	set_bit(config->w_reg, SDA_DIR_BIT_POS, SDA_DIR_INPUT);
	return get_bit(config->r_reg, SDA_BIT_R_POS);
}

static const struct i2c_bitbang_io i2c_litex_bitbang_io = {
	.set_scl = i2c_litex_bitbang_set_scl,
	.set_sda = i2c_litex_bitbang_set_sda,
	.get_sda = i2c_litex_bitbang_get_sda,
};

static int i2c_litex_init(struct device *dev)
{
	const struct i2c_litex_cfg *config = GET_I2C_CFG(dev);
	struct i2c_bitbang *bitbang = GET_I2C_BITBANG(dev);

	*(config->w_reg) |= HIGH_STATE_ON_I2C_LINES;
	i2c_bitbang_init(bitbang, &i2c_litex_bitbang_io, (void *)config);

	return 0;
}

static int i2c_litex_configure(struct device *dev, uint32_t dev_config)
{
	struct i2c_bitbang *bitbang = GET_I2C_BITBANG(dev);

	return i2c_bitbang_configure(bitbang, dev_config);
}

static int i2c_litex_transfer(struct device *dev,  struct i2c_msg *msgs,
		       uint8_t num_msgs, uint16_t addr)
{
	struct i2c_bitbang *bitbang = GET_I2C_BITBANG(dev);

	return i2c_bitbang_transfer(bitbang, msgs, num_msgs, addr);
}

static const struct i2c_driver_api i2c_litex_driver_api = {
	.configure         = i2c_litex_configure,
	.transfer          = i2c_litex_transfer,
	.slave_register    = NULL,
	.slave_unregister  = NULL,
};

/* Device Instantiation */

#define I2C_LITEX_INIT(n)						       \
	static const struct i2c_litex_cfg i2c_litex_cfg_##n = {		       \
		.w_reg =						       \
		 (volatile uint32_t *) DT_INST_REG_ADDR_BY_NAME(n, write),\
		.r_reg =						       \
		 (volatile uint32_t *) DT_INST_REG_ADDR_BY_NAME(n, read), \
	};								       \
									       \
	static struct i2c_bitbang i2c_bitbang_##n;			       \
									       \
	DEVICE_AND_API_INIT(litex_i2c_##n,				       \
			   DT_INST_LABEL(n),		       \
			   i2c_litex_init,				       \
			   &i2c_bitbang_##n,	                               \
			   &i2c_litex_cfg_##n,				       \
			   POST_KERNEL,					       \
			   CONFIG_I2C_INIT_PRIORITY,			       \
			   &i2c_litex_driver_api			       \
			   );

DT_INST_FOREACH_STATUS_OKAY(I2C_LITEX_INIT)
