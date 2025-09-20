/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_i2c

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "bf0_hal_i2c.h"

LOG_MODULE_REGISTER(i2c_sf32lb, CONFIG_I2C_LOG_LEVEL);

struct i2c_sf32lb_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
};

struct i2c_sf32lb_data {
	struct k_sem lock;
	I2C_HandleTypeDef hi2c;
	uint32_t i2c_speed;
};

static int i2c_sf32lb_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	HAL_StatusTypeDef status;

	if (!(I2C_MODE_CONTROLLER & dev_config)) {
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		data->i2c_speed = 100000u;
		break;

	case I2C_SPEED_FAST:
		data->i2c_speed = 400000U;
		break;

	case I2C_SPEED_FAST_PLUS:
	case I2C_SPEED_HIGH:
	case I2C_SPEED_ULTRA:
	default:
		LOG_ERR("Unsupported I2C speed requested");
		return -ENOTSUP;
	}

	/* Configure I2C */
	data->hi2c.Instance = (I2C_TypeDef *)cfg->base;
	data->hi2c.Init.ClockSpeed = data->i2c_speed;
	data->hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	data->hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	data->hi2c.Init.OwnAddress1 = 0;
	data->hi2c.Init.Timing = 0;

	status = HAL_I2C_Init(&data->hi2c);
	if (status != HAL_OK) {
		LOG_ERR("Failed to initialize I2C");
		return -EIO;
	}

	return 0;
}

static int i2c_sf32lb_transfer(const struct device *dev, struct i2c_msg *msgs,
			       uint8_t num_msgs, uint16_t addr)
{
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	HAL_StatusTypeDef status;
	int ret = 0;

	if (!num_msgs) {
		return 0;
	}

	k_sem_take(&data->lock, K_FOREVER);

	for (int i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_STOP) {
			/* Last message */
			if (msgs[i].flags & I2C_MSG_READ) {
				status = HAL_I2C_Master_Receive(&data->hi2c, addr << 1,
							      msgs[i].buf, msgs[i].len,
							      HAL_MAX_DELAY);
			} else {
				status = HAL_I2C_Master_Transmit(&data->hi2c, addr << 1,
							       msgs[i].buf, msgs[i].len,
							       HAL_MAX_DELAY);
			}
		} else {
			/* Not the last message */
			if (msgs[i].flags & I2C_MSG_READ) {
				status = HAL_I2C_Master_Receive(&data->hi2c, addr << 1,
							      msgs[i].buf, msgs[i].len,
							      HAL_MAX_DELAY);
			} else {
				status = HAL_I2C_Master_Transmit(&data->hi2c, addr << 1,
							       msgs[i].buf, msgs[i].len,
							       HAL_MAX_DELAY);
			}
		}

		if (status != HAL_OK) {
			ret = -EIO;
			break;
		}
	}

	k_sem_give(&data->lock);
	return ret;
}

static const struct i2c_driver_api i2c_sf32lb_driver_api = {
	.configure = i2c_sf32lb_configure,
	.transfer = i2c_sf32lb_transfer,
};

static int i2c_sf32lb_init(const struct device *dev)
{
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	int err;

	k_sem_init(&data->lock, 1, 1);

	/* Configure pins */
	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	/* Enable clock */
	err = clock_control_on(cfg->clock_dev, (clock_control_subsys_t)cfg->clock_subsys);
	if (err) {
		return err;
	}

	/* Initialize I2C */
	data->hi2c.Instance = (I2C_TypeDef *)cfg->base;
	data->hi2c.Init.ClockSpeed = 100000; /* Default to standard speed */
	data->hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	data->hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	data->hi2c.Init.OwnAddress1 = 0;
	data->hi2c.Init.Timing = 0;

	err = HAL_I2C_Init(&data->hi2c);
	if (err != HAL_OK) {
		LOG_ERR("Failed to initialize I2C");
		return -EIO;
	}

	return 0;
}

#define I2C_SF32LB_INIT(n) \
	PINCTRL_DT_INST_DEFINE(n); \
	static struct i2c_sf32lb_data i2c_sf32lb_data_##n; \
	static const struct i2c_sf32lb_config i2c_sf32lb_config_##n = { \
		.base = DT_INST_REG_ADDR(n), \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n), \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)), \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, id), \
	}; \
	DEVICE_DT_INST_DEFINE(n, \
		&i2c_sf32lb_init, \
		NULL, \
		&i2c_sf32lb_data_##n, \
		&i2c_sf32lb_config_##n, \
		POST_KERNEL, \
		CONFIG_I2C_INIT_PRIORITY, \
		&i2c_sf32lb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_SF32LB_INIT)
