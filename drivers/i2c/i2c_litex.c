/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_i2c

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_litex, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"
#include "i2c_bitbang.h"

#include <soc.h>

#define SCL_BIT_POS                0
#define SDA_DIR_BIT_POS            1
#define SDA_BIT_W_POS              2
#define SDA_BIT_R_POS              0

#define SDA_DIR_OUTPUT             1
#define SDA_DIR_INPUT              0

#define HIGH_STATE_ON_I2C_LINES    0x7

struct i2c_litex_cfg {
	uint32_t write_addr;
	uint32_t read_addr;
	uint32_t bitrate;
};

#define GET_I2C_CFG(dev)						     \
	((const struct i2c_litex_cfg *) dev->config)

#define GET_I2C_BITBANG(dev)						     \
	((struct i2c_bitbang *) dev->data)

static inline void set_bit(uint32_t addr, uint32_t bit, uint32_t val)
{
	uint32_t mask = BIT(bit);

	if (val) {
		litex_write8(litex_read8(addr) | mask, addr);
	} else {
		litex_write8(litex_read8(addr) & ~mask, addr);
	}
}

static inline int get_bit(uint32_t addr, uint32_t bit)
{
	uint32_t mask = BIT(bit);

	return !!(litex_read8(addr) & mask);
}

static void i2c_litex_bitbang_set_scl(void *context, int state)
{
	const struct i2c_litex_cfg *config =
		(const struct i2c_litex_cfg *) context;

	set_bit(config->write_addr, SCL_BIT_POS, state);
}

static void i2c_litex_bitbang_set_sda(void *context, int state)
{
	const struct i2c_litex_cfg *config =
		(const struct i2c_litex_cfg *) context;

	set_bit(config->write_addr, SDA_DIR_BIT_POS, SDA_DIR_OUTPUT);
	set_bit(config->write_addr, SDA_BIT_W_POS, state);
}

static int i2c_litex_bitbang_get_sda(void *context)
{
	const struct i2c_litex_cfg *config =
		(const struct i2c_litex_cfg *) context;

	set_bit(config->write_addr, SDA_DIR_BIT_POS, SDA_DIR_INPUT);
	return get_bit(config->read_addr, SDA_BIT_R_POS);
}

static const struct i2c_bitbang_io i2c_litex_bitbang_io = {
	.set_scl = i2c_litex_bitbang_set_scl,
	.set_sda = i2c_litex_bitbang_set_sda,
	.get_sda = i2c_litex_bitbang_get_sda,
};

static int i2c_litex_init(const struct device *dev)
{
	const struct i2c_litex_cfg *config = GET_I2C_CFG(dev);
	struct i2c_bitbang *bitbang = GET_I2C_BITBANG(dev);
	int ret;

	litex_write8(litex_read8(config->write_addr) | HIGH_STATE_ON_I2C_LINES, config->write_addr);
	i2c_bitbang_init(bitbang, &i2c_litex_bitbang_io, (void *)config);

	ret = i2c_bitbang_configure(bitbang,
				    I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate));
	if (ret != 0) {
		LOG_ERR("failed to configure I2C bitbang: %d", ret);
	}

	return ret;
}

static int i2c_litex_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_bitbang *bitbang = GET_I2C_BITBANG(dev);

	return i2c_bitbang_configure(bitbang, dev_config);
}

static int i2c_litex_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_bitbang *bitbang = GET_I2C_BITBANG(dev);

	return i2c_bitbang_get_config(bitbang, config);
}

static int i2c_litex_transfer(const struct device *dev,  struct i2c_msg *msgs,
			      uint8_t num_msgs, uint16_t addr)
{
	struct i2c_bitbang *bitbang = GET_I2C_BITBANG(dev);

	return i2c_bitbang_transfer(bitbang, msgs, num_msgs, addr);
}

static int i2c_litex_recover_bus(const struct device *dev)
{
	struct i2c_bitbang *bitbang = GET_I2C_BITBANG(dev);

	return i2c_bitbang_recover_bus(bitbang);
}

static DEVICE_API(i2c, i2c_litex_driver_api) = {
	.configure = i2c_litex_configure,
	.get_config = i2c_litex_get_config,
	.transfer = i2c_litex_transfer,
	.recover_bus = i2c_litex_recover_bus,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

/* Device Instantiation */

#define I2C_LITEX_INIT(n)						       \
	static const struct i2c_litex_cfg i2c_litex_cfg_##n = {		       \
		.write_addr = DT_INST_REG_ADDR_BY_NAME(n, write),	       \
		.read_addr = DT_INST_REG_ADDR_BY_NAME(n, read),		       \
		.bitrate = DT_INST_PROP(n, clock_frequency),                   \
	};								       \
									       \
	static struct i2c_bitbang i2c_bitbang_##n;			       \
									       \
	I2C_DEVICE_DT_INST_DEFINE(n,					       \
			   i2c_litex_init,				       \
			   NULL,					       \
			   &i2c_bitbang_##n,	                               \
			   &i2c_litex_cfg_##n,				       \
			   POST_KERNEL,					       \
			   CONFIG_I2C_INIT_PRIORITY,			       \
			   &i2c_litex_driver_api			       \
			   );

DT_INST_FOREACH_STATUS_OKAY(I2C_LITEX_INIT)
