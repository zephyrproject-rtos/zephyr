/*
 * Copyright (c) 2018 Diego Sueiro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_i2c

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <em_cmu.h>
#include <em_i2c.h>
#include <em_gpio.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_gecko);

#include "i2c-priv.h"

#define DEV_BASE(dev) \
	((I2C_TypeDef *)((const struct i2c_gecko_config * const)(dev)->config)->base)

struct i2c_gecko_config {
	I2C_TypeDef *base;
	CMU_Clock_TypeDef clock;
	uint32_t bitrate;
	struct soc_gpio_pin pin_sda;
	struct soc_gpio_pin pin_scl;
#ifdef CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION
	uint8_t loc_sda;
	uint8_t loc_scl;
#else
	uint8_t loc;
#endif
};

struct i2c_gecko_data {
	struct k_sem device_sync_sem;
	uint32_t dev_config;
};

void i2c_gecko_config_pins(const struct device *dev,
			   const struct soc_gpio_pin *pin_sda,
			   const struct soc_gpio_pin *pin_scl)
{
	I2C_TypeDef *base = DEV_BASE(dev);
	const struct i2c_gecko_config *config = dev->config;

	GPIO_PinModeSet(pin_scl->port, pin_scl->pin, pin_scl->mode, pin_scl->out);
	GPIO_PinModeSet(pin_sda->port, pin_sda->pin, pin_sda->mode, pin_sda->out);

#ifdef CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION
	base->ROUTEPEN = I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN;
	base->ROUTELOC0 = (config->loc_sda << _I2C_ROUTELOC0_SDALOC_SHIFT) |
			  (config->loc_scl << _I2C_ROUTELOC0_SCLLOC_SHIFT);
#elif defined(GPIO_I2C_ROUTEEN_SCLPEN) && defined(GPIO_I2C_ROUTEEN_SDAPEN)
	GPIO->I2CROUTE[I2C_NUM(base)].ROUTEEN = GPIO_I2C_ROUTEEN_SCLPEN |
		GPIO_I2C_ROUTEEN_SDAPEN;
	GPIO->I2CROUTE[I2C_NUM(base)].SCLROUTE =
		(config->pin_scl.pin << _GPIO_I2C_SCLROUTE_PIN_SHIFT) |
		(config->pin_scl.port << _GPIO_I2C_SCLROUTE_PORT_SHIFT);
	GPIO->I2CROUTE[I2C_NUM(base)].SDAROUTE =
		(config->pin_sda.pin << _GPIO_I2C_SDAROUTE_PIN_SHIFT) |
		(config->pin_sda.port << _GPIO_I2C_SDAROUTE_PORT_SHIFT);
#else
	base->ROUTE = I2C_ROUTE_SDAPEN | I2C_ROUTE_SCLPEN | (config->loc << 8);
#endif
}

static int i2c_gecko_configure(const struct device *dev,
			       uint32_t dev_config_raw)
{
	I2C_TypeDef *base = DEV_BASE(dev);
	struct i2c_gecko_data *data = dev->data;
	I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;
	uint32_t baudrate;

	if (!(I2C_MODE_CONTROLLER & dev_config_raw)) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		baudrate = KHZ(100);
		break;
	case I2C_SPEED_FAST:
		baudrate = KHZ(400);
		break;
	case I2C_SPEED_FAST_PLUS:
		baudrate = MHZ(1);
		break;
	default:
		return -EINVAL;
	}

	data->dev_config = dev_config_raw;
	i2cInit.freq = baudrate;

	I2C_Init(base, &i2cInit);

	return 0;
}

static int i2c_gecko_transfer(const struct device *dev, struct i2c_msg *msgs,
			      uint8_t num_msgs, uint16_t addr)
{
	I2C_TypeDef *base = DEV_BASE(dev);
	struct i2c_gecko_data *data = dev->data;
	I2C_TransferSeq_TypeDef seq;
	I2C_TransferReturn_TypeDef ret = -EIO;
	uint32_t timeout = 300000U;

	if (!num_msgs) {
		return 0;
	}

	seq.addr = addr << 1;

	do {
		seq.buf[0].data = msgs->buf;
		seq.buf[0].len	= msgs->len;

		if ((msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			seq.flags = I2C_FLAG_READ;
		} else {
			seq.flags = I2C_FLAG_WRITE;
			if (num_msgs > 1) {
				/* Next message */
				msgs++;
				num_msgs--;
				if ((msgs->flags & I2C_MSG_RW_MASK)
				    == I2C_MSG_READ) {
					seq.flags = I2C_FLAG_WRITE_READ;
				} else {
					seq.flags = I2C_FLAG_WRITE_WRITE;
				}
				seq.buf[1].data = msgs->buf;
				seq.buf[1].len	= msgs->len;
			}
		}

		if (data->dev_config & I2C_ADDR_10_BITS) {
			seq.flags |= I2C_FLAG_10BIT_ADDR;
		}

		/* Do a polled transfer */
		ret = I2C_TransferInit(base, &seq);
		while (ret == i2cTransferInProgress && timeout--) {
			ret = I2C_Transfer(base);
		}

		if (ret != i2cTransferDone) {
			goto finish;
		}

		/* Next message */
		msgs++;
		num_msgs--;
	} while (num_msgs);

finish:
	if (ret != i2cTransferDone) {
		ret = -EIO;
	}
	return ret;
}

static int i2c_gecko_init(const struct device *dev)
{
	const struct i2c_gecko_config *config = dev->config;
	uint32_t bitrate_cfg;
	int error;

	CMU_ClockEnable(config->clock, true);

	i2c_gecko_config_pins(dev, &config->pin_sda, &config->pin_scl);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

	error = i2c_gecko_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (error) {
		return error;
	}

	return 0;
}

static const struct i2c_driver_api i2c_gecko_driver_api = {
	.configure = i2c_gecko_configure,
	.transfer = i2c_gecko_transfer,
};

#ifdef CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION
#define I2C_LOC_DATA(idx) \
	.loc_sda = DT_INST_PROP_BY_IDX(idx, location_sda, 0), \
	.loc_scl = DT_INST_PROP_BY_IDX(idx, location_scl, 0)
#define I2C_VALIDATE_LOC(idx) BUILD_ASSERT(true, "")
#else
#define I2C_VALIDATE_LOC(idx) \
	BUILD_ASSERT(DT_INST_PROP_BY_IDX(idx, location_sda, 0) \
		     == DT_INST_PROP_BY_IDX(idx, location_scl, 0), \
		     "DTS location-* properties must be equal")
#define I2C_LOC_DATA(idx) \
	.loc = DT_INST_PROP_BY_IDX(idx, location_scl, 0)
#endif

#define I2C_INIT(idx) \
I2C_VALIDATE_LOC(idx); \
static const struct i2c_gecko_config i2c_gecko_config_##idx = { \
	.base = (I2C_TypeDef *)DT_INST_REG_ADDR(idx), \
	.clock = cmuClock_I2C##idx, \
	.pin_sda = {DT_INST_PROP_BY_IDX(idx, location_sda, 1), \
		DT_INST_PROP_BY_IDX(idx, location_sda, 2), gpioModeWiredAnd, 1}, \
	.pin_scl = {DT_INST_PROP_BY_IDX(idx, location_scl, 1), \
		DT_INST_PROP_BY_IDX(idx, location_scl, 2), gpioModeWiredAnd, 1}, \
	I2C_LOC_DATA(idx), \
	.bitrate = DT_INST_PROP(idx, clock_frequency), \
}; \
\
static struct i2c_gecko_data i2c_gecko_data_##idx; \
\
I2C_DEVICE_DT_INST_DEFINE(idx, i2c_gecko_init, \
		 NULL, \
		 &i2c_gecko_data_##idx, &i2c_gecko_config_##idx, \
		 POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, \
		 &i2c_gecko_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_INIT)
