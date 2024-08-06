/*
 * Copyright (c) 2024 Baylibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_i2c

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/irq.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_cc23x0);

#include <driverlib/clkctl.h>
#include <driverlib/i2c.h>

#include "i2c-priv.h"

#define I2C_MASTER_ERR_NONE 0

struct i2c_cc23x0_data {
	struct k_sem lock;
	struct k_sem complete;
	volatile uint32_t error;
};

struct i2c_cc23x0_config {
	uint32_t base;
	const struct pinctrl_dev_config *pcfg;
};

static int i2c_cc23x0_transmit(const struct device *dev, const uint8_t *buf, uint32_t len,
			       uint16_t addr)
{
	const struct i2c_cc23x0_config *config = dev->config;
	const uint32_t base = config->base;
	struct i2c_cc23x0_data *data = dev->data;

	/* Sending address without data is not supported */
	if (len == 0) {
		return -EIO;
	}

	I2CControllerSetTargetAddr(base, addr, false);

	/* Single transmission */
	if (len == 1) {
		I2CControllerPutData(base, *buf);
		I2CControllerCommand(base, I2C_CONTROLLER_CMD_SINGLE_SEND);
		k_sem_take(&data->complete, K_FOREVER);
		return data->error == I2C_MASTER_ERR_NONE ? 0 : -EIO;
	}

	/* Burst transmission */
	I2CControllerPutData(base, buf[0]);
	I2CControllerCommand(base, I2C_CONTROLLER_CMD_BURST_SEND_START);
	k_sem_take(&data->complete, K_FOREVER);

	if (data->error != I2C_MASTER_ERR_NONE) {
		goto send_error_stop;
	}

	for (int i = 1; i < len - 1; i++) {
		I2CControllerPutData(base, buf[i]);
		I2CControllerCommand(base, I2C_CONTROLLER_CMD_BURST_SEND_CONT);
		k_sem_take(&data->complete, K_FOREVER);

		if (data->error != I2C_MASTER_ERR_NONE) {
			goto send_error_stop;
		}
	}

	I2CControllerPutData(base, buf[len - 1]);
	I2CControllerCommand(base, I2C_CONTROLLER_CMD_BURST_SEND_FINISH);
	k_sem_take(&data->complete, K_FOREVER);

	if (data->error != I2C_MASTER_ERR_NONE) {
		return -EIO;
	}

	return 0;

send_error_stop:
	I2CControllerCommand(base, I2C_CONTROLLER_CMD_BURST_SEND_ERROR_STOP);
	return -EIO;
}

static int i2c_cc23x0_receive(const struct device *dev, uint8_t *buf, uint32_t len, uint16_t addr)
{
	const struct i2c_cc23x0_config *config = dev->config;
	const uint32_t base = config->base;
	struct i2c_cc23x0_data *data = dev->data;

	/* Sending address without data is not supported */
	if (len == 0) {
		return -EIO;
	}
	I2CControllerSetTargetAddr(base, addr, true);

	/* Single receive */
	if (len == 1) {
		I2CControllerCommand(base, I2C_CONTROLLER_CMD_SINGLE_RECEIVE);
		k_sem_take(&data->complete, K_FOREVER);

		if (data->error != I2C_MASTER_ERR_NONE) {
			return -EIO;
		}

		*buf = I2CControllerGetData(base);
		return 0;
	}

	/* Burst receive */
	I2CControllerCommand(base, I2C_CONTROLLER_CMD_BURST_RECEIVE_START);
	k_sem_take(&data->complete, K_FOREVER);

	if (data->error != I2C_MASTER_ERR_NONE) {
		goto recv_error_stop;
	}

	buf[0] = I2CControllerGetData(base);

	for (int i = 1; i < len - 1; i++) {
		I2CControllerCommand(base, I2C_CONTROLLER_CMD_BURST_RECEIVE_CONT);
		k_sem_take(&data->complete, K_FOREVER);

		if (data->error != I2C_MASTER_ERR_NONE) {
			goto recv_error_stop;
		}

		buf[i] = I2CControllerGetData(base);
	}

	I2CControllerCommand(base, I2C_CONTROLLER_CMD_BURST_RECEIVE_FINISH);
	k_sem_take(&data->complete, K_FOREVER);

	if (data->error != I2C_MASTER_ERR_NONE) {
		return -EIO;
	}

	buf[len - 1] = I2CControllerGetData(base);

	return 0;

recv_error_stop:
	I2CControllerCommand(base, I2C_CONTROLLER_CMD_BURST_RECEIVE_ERROR_STOP);
	return -EIO;
}

static int i2c_cc23x0_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	struct i2c_cc23x0_data *data = dev->data;
	int ret = 0;

	if (num_msgs == 0) {
		return 0;
	}

	k_sem_take(&data->lock, K_FOREVER);

	for (int i = 0; i < num_msgs; i++) {
		/* Not supported by hardware */
		if (msgs[i].flags & I2C_MSG_ADDR_10_BITS) {
			ret = -EIO;
			break;
		}

		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = i2c_cc23x0_transmit(dev, msgs[i].buf, msgs[i].len, addr);
		} else {
			ret = i2c_cc23x0_receive(dev, msgs[i].buf, msgs[i].len, addr);
		}

		if (ret) {
			break;
		}
	}
	k_sem_give(&data->lock);

	return ret;
}

#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)
static int i2c_cc23x0_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_cc23x0_config *config = dev->config;
	bool fast;

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		fast = false;
		break;
	case I2C_SPEED_FAST:
		fast = true;
		break;
	default:
		LOG_ERR("Unsupported speed");
		return -EIO;
	}

	/* Support for slave mode has not been implemented */
	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Slave mode is not supported");
		return -EIO;
	}

	/* This is deprecated and could be ignored in the future */
	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("10-bit addressing mode is not supported");
		return -EIO;
	}

	/* Enables and configures I2C master */
	I2CControllerInitExpClk(config->base, fast);

	CLKCTLEnable(CLKCTL_BASE, CLKCTL_I2C0);

	return 0;
}

static void i2c_cc23x0_isr(const struct device *dev)
{
	const struct i2c_cc23x0_config *config = dev->config;
	struct i2c_cc23x0_data *data = dev->data;
	const uint32_t base = config->base;

	if (I2CControllerIntStatus(base, true)) {
		I2CControllerClearInt(base);

		data->error = I2CControllerError(base);

		k_sem_give(&data->complete);
	}
}

static const struct i2c_driver_api i2c_cc23x0_driver_api = {.configure = i2c_cc23x0_configure,
							    .transfer = i2c_cc23x0_transfer};

#define I2C_CC23X0_INIT_FUNC(id)                                                                   \
	static int i2c_cc23x0_init##id(const struct device *dev)                                   \
	{                                                                                          \
		const struct i2c_cc23x0_config *config = dev->config;                              \
		uint32_t cfg;                                                                      \
		int err;                                                                           \
                                                                                                   \
		CLKCTLEnable(CLKCTL_BASE, CLKCTL_I2C0);                                            \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), i2c_cc23x0_isr,           \
			    DEVICE_DT_INST_GET(id), 0);                                            \
                                                                                                   \
		irq_enable(DT_INST_IRQN(id));                                                      \
                                                                                                   \
		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);                    \
		if (err < 0) {                                                                     \
			LOG_ERR("Failed to configure pinctrl state\n");                            \
			return err;                                                                \
		}                                                                                  \
                                                                                                   \
		cfg = i2c_map_dt_bitrate(DT_INST_PROP(id, clock_frequency));                       \
		err = i2c_cc23x0_configure(dev, cfg | I2C_MODE_CONTROLLER);                        \
		if (err) {                                                                         \
			LOG_ERR("Failed to configure\n");                                          \
			return err;                                                                \
		}                                                                                  \
                                                                                                   \
		I2CControllerEnableInt(config->base);                                              \
		return 0;                                                                          \
	}

#define CC23X0_I2C(id)                                                                             \
	I2C_CC23X0_INIT_FUNC(id);                                                                  \
	PINCTRL_DT_INST_DEFINE(id);                                                                \
                                                                                                   \
	static const struct i2c_cc23x0_config i2c_cc23x0_##id##_config = {                         \
		.base = DT_INST_REG_ADDR(id),                                                      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),                                        \
	};                                                                                         \
                                                                                                   \
	static struct i2c_cc23x0_data i2c_cc23x0_##id##_data = {                                   \
		.lock = Z_SEM_INITIALIZER(i2c_cc23x0_##id##_data.lock, 1, 1),                      \
		.complete = Z_SEM_INITIALIZER(i2c_cc23x0_##id##_data.complete, 0, 1),              \
		.error = I2C_MASTER_ERR_NONE};                                                     \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(id, i2c_cc23x0_init##id, NULL, &i2c_cc23x0_##id##_data,          \
				  &i2c_cc23x0_##id##_config, POST_KERNEL,                          \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_cc23x0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CC23X0_I2C);
