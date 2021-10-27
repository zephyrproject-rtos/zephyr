/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_b91_i2c

#include "i2c.h"
#include "clock.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_telink);

#include <drivers/i2c.h>
#include "i2c-priv.h"
#include <drivers/pinmux.h>
#include <dt-bindings/pinctrl/b91-pinctrl.h>

/* I2C configuration structure */
struct i2c_b91_cfg {
	uint32_t bitrate;
	const uint32_t *pinctrl_list;
	size_t pinctrl_list_size;
};

/* I2C data structure */
struct i2c_b91_data {
	struct k_sem mutex;
};

/* API implementation: configure */
static int i2c_b91_configure(const struct device *dev, uint32_t dev_config)
{
	ARG_UNUSED(dev);

	uint32_t i2c_speed = 0u;

	/* check address size */
	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("10-bits address is not supported");
		return -ENOTSUP;
	}

	/* check I2C Master/Slave configuration */
	if (!(dev_config & I2C_MODE_MASTER)) {
		LOG_ERR("I2C slave is not implemented");
		return -ENOTSUP;
	}

	/* check i2c speed */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		i2c_speed = 100000u;
		break;

	case I2C_SPEED_FAST:
		i2c_speed = 400000U;
		break;

	case I2C_SPEED_FAST_PLUS:
	case I2C_SPEED_HIGH:
	case I2C_SPEED_ULTRA:
	default:
		LOG_ERR("Unsupported I2C speed requested");
		return -ENOTSUP;
	}

	/* init i2c */
	i2c_master_init();
	i2c_set_master_clk((unsigned char)(sys_clk.pclk * 1000 * 1000 / (4 * i2c_speed)));

	return 0;
}

/* API implementation: transfer */
static int i2c_b91_transfer(const struct device *dev,
			    struct i2c_msg *msgs,
			    uint8_t num_msgs,
			    uint16_t addr)
{
	int status = 0;
	uint8_t send_stop = 0;
	struct i2c_b91_data *data = dev->data;

	/* get the mutex */
	k_sem_take(&data->mutex, K_FOREVER);

	/* loop through all messages */
	for (int i = 0; i < num_msgs; i++) {
		/* check addr size */
		if (msgs[i].flags & I2C_MSG_ADDR_10_BITS) {
			LOG_ERR("10-bits address is not supported");
			k_sem_give(&data->mutex);
			return -ENOTSUP;
		}

		/* config stop bit */
		send_stop = msgs[i].flags & I2C_MSG_STOP ? 1 : 0;
		i2c_master_send_stop(send_stop);

		/* transfer data */
		if (msgs[i].flags & I2C_MSG_READ) {
			status = i2c_master_read(addr, msgs[i].buf, msgs[i].len);
		} else {
			status = i2c_master_write(addr, msgs[i].buf, msgs[i].len);
		}

		/* check status */
		if (!status) {
			LOG_ERR("Failed to transfer I2C messages\n");
			k_sem_give(&data->mutex);
			return -EIO;
		}
	}

	/* release the mutex */
	k_sem_give(&data->mutex);

	return 0;
};

/* API implementation: init */
static int i2c_b91_init(const struct device *dev)
{
	int status = 0;
	const struct device *pinmux;
	const struct i2c_b91_cfg *cfg = dev->config;
	struct i2c_b91_data *data = dev->data;
	uint32_t dev_config = (I2C_MODE_MASTER | i2c_map_dt_bitrate(cfg->bitrate));

	/* init mutex */
	k_sem_init(&data->mutex, 1, 1);

	/* config i2c on startup */
	status = i2c_b91_configure(dev, dev_config);
	if (status != 0) {
		LOG_ERR("Failed to configure I2C on init");
		return status;
	}

	/* get pinmux driver */
	pinmux = DEVICE_DT_GET(DT_NODELABEL(pinmux));
	if (!device_is_ready(pinmux)) {
		return -ENODEV;
	}

	/* config pins */
	for (int i = 0; i < cfg->pinctrl_list_size; i++) {
		pinmux_pin_set(pinmux, B91_PINMUX_GET_PIN(cfg->pinctrl_list[i]),
			       B91_PINMUX_GET_FUNC(cfg->pinctrl_list[i]));
	}

	return 0;
}

/* I2C driver APIs structure */
static const struct i2c_driver_api i2c_b91_api = {
	.configure = i2c_b91_configure,
	.transfer = i2c_b91_transfer,
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "unsupported I2C instance");

/* I2C driver registration */
#define I2C_B91_INIT(inst)					  \
								  \
	static const uint32_t i2c_pins_##inst[] =		  \
		B91_PINMUX_DT_INST_GET_ARRAY(inst, 0);		  \
								  \
	static struct i2c_b91_data i2c_b91_data_##inst;		  \
								  \
	static struct i2c_b91_cfg i2c_b91_cfg_##inst = {	  \
		.bitrate = DT_INST_PROP(inst, clock_frequency),	  \
		.pinctrl_list_size = ARRAY_SIZE(i2c_pins_##inst), \
		.pinctrl_list = i2c_pins_##inst			  \
	};							  \
								  \
	I2C_DEVICE_DT_INST_DEFINE(inst, i2c_b91_init,		  \
			      NULL,				  \
			      &i2c_b91_data_##inst,		  \
			      &i2c_b91_cfg_##inst,		  \
			      POST_KERNEL,			  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			      &i2c_b91_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_B91_INIT)
