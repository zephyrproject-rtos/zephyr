/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_i2c

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include <am_mcu_apollo.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>

LOG_MODULE_REGISTER(ambiq_i2c, CONFIG_I2C_LOG_LEVEL);

typedef int (*ambiq_i2c_pwr_func_t)(void);

#define PWRCTRL_MAX_WAIT_US 5

#include "i2c-priv.h"

struct i2c_ambiq_config {
	uint32_t base;
	int size;
	uint32_t bitrate;
	const struct pinctrl_dev_config *pcfg;
	ambiq_i2c_pwr_func_t pwr_func;
};

struct i2c_ambiq_data {
	am_hal_iom_config_t iom_cfg;
	void *IOMHandle;
};

static int i2c_ambiq_read(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	struct i2c_ambiq_data *data = dev->data;

	int ret = 0;

	am_hal_iom_transfer_t trans = {0};

	trans.ui8Priority = 1;
	trans.eDirection = AM_HAL_IOM_RX;
	trans.uPeerInfo.ui32I2CDevAddr = addr;
	trans.ui32NumBytes = msg->len;
	trans.pui32RxBuffer = (uint32_t *)msg->buf;

	ret = am_hal_iom_blocking_transfer(data->IOMHandle, &trans);

	return ret;
}

static int i2c_ambiq_write(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	struct i2c_ambiq_data *data = dev->data;

	int ret = 0;

	am_hal_iom_transfer_t trans = {0};

	trans.ui8Priority = 1;
	trans.eDirection = AM_HAL_IOM_TX;
	trans.uPeerInfo.ui32I2CDevAddr = addr;
	trans.ui32NumBytes = msg->len;
	trans.pui32TxBuffer = (uint32_t *)msg->buf;

	ret = am_hal_iom_blocking_transfer(data->IOMHandle, &trans);

	return ret;
}

static int i2c_ambiq_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_ambiq_data *data = dev->data;

	if (!(I2C_MODE_CONTROLLER & dev_config)) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		data->iom_cfg.ui32ClockFreq = AM_HAL_IOM_100KHZ;
		break;
	case I2C_SPEED_FAST:
		data->iom_cfg.ui32ClockFreq = AM_HAL_IOM_400KHZ;
		break;
	case I2C_SPEED_FAST_PLUS:
		data->iom_cfg.ui32ClockFreq = AM_HAL_IOM_1MHZ;
		break;
	default:
		return -EINVAL;
	}

	am_hal_iom_configure(data->IOMHandle, &data->iom_cfg);

	return 0;
}

static int i2c_ambiq_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			      uint16_t addr)
{
	int ret = 0;

	if (!num_msgs) {
		return 0;
	}

	for (int i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			ret = i2c_ambiq_read(dev, &(msgs[i]), addr);
		} else {
			ret = i2c_ambiq_write(dev, &(msgs[i]), addr);
		}

		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static int i2c_ambiq_init(const struct device *dev)
{
	struct i2c_ambiq_data *data = dev->data;
	const struct i2c_ambiq_config *config = dev->config;
	uint32_t bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);
	int ret = 0;

	data->iom_cfg.eInterfaceMode = AM_HAL_IOM_I2C_MODE;

	ret = am_hal_iom_initialize((config->base - REG_IOM_BASEADDR) / config->size,
				    &data->IOMHandle);

	ret = config->pwr_func();

	ret = i2c_ambiq_configure(dev, I2C_MODE_CONTROLLER | I2C_SPEED_SET(bitrate_cfg));

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		return ret;
	}

	ret = am_hal_iom_enable(data->IOMHandle);

	return ret;
}

static struct i2c_driver_api i2c_ambiq_driver_api = {
	.configure = i2c_ambiq_configure,
	.transfer = i2c_ambiq_transfer,
};

#define AMBIQ_I2C_DEFINE(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static int pwr_on_ambiq_i2c_##n(void)                                                      \
	{                                                                                          \
		uint32_t addr = DT_REG_ADDR(DT_INST_PHANDLE(n, ambiq_pwrcfg)) +                    \
				DT_INST_PHA(n, ambiq_pwrcfg, offset);                              \
		sys_write32((sys_read32(addr) | DT_INST_PHA(n, ambiq_pwrcfg, mask)), addr);        \
		k_busy_wait(PWRCTRL_MAX_WAIT_US);                                                  \
		return 0;                                                                          \
	}                                                                                          \
	static struct i2c_ambiq_data i2c_ambiq_data##n;                                            \
	static const struct i2c_ambiq_config i2c_ambiq_config##n = {                               \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.size = DT_INST_REG_SIZE(n),                                                       \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.pwr_func = pwr_on_ambiq_i2c_##n};                                                 \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_ambiq_init, NULL, &i2c_ambiq_data##n,                     \
				  &i2c_ambiq_config##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,     \
				  &i2c_ambiq_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_I2C_DEFINE)
