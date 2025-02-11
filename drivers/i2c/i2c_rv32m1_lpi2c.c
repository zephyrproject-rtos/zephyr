/*
 * Copyright (c) 2019, Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * Based on the i2c_mcux_lpi2c.c driver, which is:
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT openisa_rv32m1_lpi2c

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <fsl_lpi2c.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <zephyr/drivers/pinctrl.h>

LOG_MODULE_REGISTER(rv32m1_lpi2c);

#include "i2c-priv.h"

struct rv32m1_lpi2c_config {
	LPI2C_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	clock_ip_name_t clock_ip_name;
	uint32_t clock_ip_src;
	uint32_t bitrate;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
};

struct rv32m1_lpi2c_data {
	lpi2c_master_handle_t handle;
	struct k_sem transfer_sync;
	struct k_sem completion_sync;
	status_t completion_status;
};

static int rv32m1_lpi2c_configure(const struct device *dev,
				  uint32_t dev_config)
{
	const struct rv32m1_lpi2c_config *config = dev->config;
	uint32_t baudrate;
	uint32_t clk_freq;
	int err;

	if (!(I2C_MODE_CONTROLLER & dev_config)) {
		/* Slave mode not supported - yet */
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if (I2C_ADDR_10_BITS & dev_config) {
		/* FSL LPI2C driver only supports 7-bit addressing */
		LOG_ERR("10 bit addressing not supported");
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		baudrate = KHZ(100);
		break;
	case I2C_SPEED_FAST:
		baudrate = KHZ(400);
		break;
	case I2C_SPEED_FAST_PLUS:
		baudrate = MHZ(1);
		break;
	/* TODO: only if SCL pin implements current source pull-up */
	/* case I2C_SPEED_HIGH: */
	/*      baudrate = KHZ(3400); */
	/*      break; */
	/* TODO: ultra-fast requires pin_config setting */
	/* case I2C_SPEED_ULTRA: */
	/*      baudrate = MHZ(5); */
	/*      break; */
	default:
		LOG_ERR("Unsupported speed");
		return -ENOTSUP;
	}

	err = clock_control_get_rate(config->clock_dev, config->clock_subsys, &clk_freq);
	if (err) {
		LOG_ERR("Could not get clock frequency (err %d)", err);
		return -EINVAL;
	}

	LPI2C_MasterSetBaudRate(config->base, clk_freq, baudrate);

	return 0;
}

static void rv32m1_lpi2c_master_transfer_callback(LPI2C_Type *base,
						  lpi2c_master_handle_t *handle,
						  status_t completionStatus,
						  void *userData)
{
	struct rv32m1_lpi2c_data *data = userData;

	ARG_UNUSED(base);
	ARG_UNUSED(handle);

	data->completion_status = completionStatus;
	k_sem_give(&data->completion_sync);
}

static uint32_t rv32m1_lpi2c_convert_flags(int msg_flags)
{
	uint32_t flags = 0U;

	if (!(msg_flags & I2C_MSG_STOP)) {
		flags |= kLPI2C_TransferNoStopFlag;
	}

	if (msg_flags & I2C_MSG_RESTART) {
		flags |= kLPI2C_TransferRepeatedStartFlag;
	}

	return flags;
}

static int rv32m1_lpi2c_transfer(const struct device *dev,
				 struct i2c_msg *msgs,
				 uint8_t num_msgs, uint16_t addr)
{
	const struct rv32m1_lpi2c_config *config = dev->config;
	struct rv32m1_lpi2c_data *data = dev->data;
	lpi2c_master_transfer_t transfer;
	status_t status;
	int ret = 0;

	k_sem_take(&data->transfer_sync, K_FOREVER);

	/* Iterate over all the messages */
	for (int i = 0; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs->flags) {
			ret = -ENOTSUP;
			goto out;
		}

		/* Initialize the transfer descriptor */
		transfer.flags = rv32m1_lpi2c_convert_flags(msgs->flags);

		/* Prevent the controller to send a start condition between
		 * messages, except if explicitly requested.
		 */
		if (i != 0 && !(msgs->flags & I2C_MSG_RESTART)) {
			transfer.flags |= kLPI2C_TransferNoStartFlag;
		}

		transfer.slaveAddress = addr;
		transfer.direction = (msgs->flags & I2C_MSG_READ)
			? kLPI2C_Read : kLPI2C_Write;
		transfer.subaddress = 0;
		transfer.subaddressSize = 0;
		transfer.data = msgs->buf;
		transfer.dataSize = msgs->len;

		/* Start the transfer */
		status = LPI2C_MasterTransferNonBlocking(config->base,
							 &data->handle,
							 &transfer);

		/* Return an error if the transfer didn't start successfully
		 * e.g., if the bus was busy
		 */
		if (status != kStatus_Success) {
			LOG_DBG("Could not start transfer (status %d)", status);
			ret = -EIO;
			goto out;
		}

		/* Wait for the transfer to complete */
		k_sem_take(&data->completion_sync, K_FOREVER);

		/* Return an error if the transfer didn't complete
		 * successfully. e.g., nak, timeout, lost arbitration
		 */
		if (data->completion_status != kStatus_Success) {
			LOG_DBG("Transfer failed (status %d)",
				data->completion_status);
			LPI2C_MasterTransferAbort(config->base, &data->handle);
			ret = -EIO;
			goto out;
		}

		/* Move to the next message */
		msgs++;
	}

out:
	k_sem_give(&data->transfer_sync);
	return ret;
}

static void rv32m1_lpi2c_isr(const struct device *dev)
{
	const struct rv32m1_lpi2c_config *config = dev->config;
	struct rv32m1_lpi2c_data *data = dev->data;

	LPI2C_MasterTransferHandleIRQ(config->base, &data->handle);
}

static int rv32m1_lpi2c_init(const struct device *dev)
{
	const struct rv32m1_lpi2c_config *config = dev->config;
	struct rv32m1_lpi2c_data *data = dev->data;
	lpi2c_master_config_t master_config;
	uint32_t clk_freq, dev_cfg;
	int err;

	CLOCK_SetIpSrc(config->clock_ip_name, config->clock_ip_src);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err) {
		LOG_ERR("Could not turn on clock (err %d)", err);
		return -EINVAL;
	}

	err = clock_control_get_rate(config->clock_dev, config->clock_subsys, &clk_freq);
	if (err) {
		LOG_ERR("Could not get clock frequency (err %d)", err);
		return -EINVAL;
	}

	LPI2C_MasterGetDefaultConfig(&master_config);
	LPI2C_MasterInit(config->base, &master_config, clk_freq);
	LPI2C_MasterTransferCreateHandle(config->base, &data->handle,
					 rv32m1_lpi2c_master_transfer_callback,
					 data);

	dev_cfg = i2c_map_dt_bitrate(config->bitrate);
	err = rv32m1_lpi2c_configure(dev, dev_cfg | I2C_MODE_CONTROLLER);
	if (err) {
		LOG_ERR("Could not configure controller (err %d)", err);
		return err;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	config->irq_config_func(dev);

	return 0;
}

static const struct i2c_driver_api rv32m1_lpi2c_driver_api = {
	.configure = rv32m1_lpi2c_configure,
	.transfer  = rv32m1_lpi2c_transfer,
};

#define RV32M1_LPI2C_DEVICE(id)                                                \
	PINCTRL_DT_INST_DEFINE(id);					       \
	static void rv32m1_lpi2c_irq_config_func_##id(const struct device *dev);     \
	static const struct rv32m1_lpi2c_config rv32m1_lpi2c_##id##_config = { \
		.base =                                                        \
		(LPI2C_Type *)DT_INST_REG_ADDR(id),                            \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),	       \
		.clock_subsys =                                                \
			(clock_control_subsys_t) DT_INST_CLOCKS_CELL(id, name),\
		.clock_ip_name = INST_DT_CLOCK_IP_NAME(id),                    \
		.clock_ip_src  = kCLOCK_IpSrcFircAsync,                        \
		.bitrate = DT_INST_PROP(id, clock_frequency),                  \
		.irq_config_func = rv32m1_lpi2c_irq_config_func_##id,          \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),		       \
	};                                                                     \
	static struct rv32m1_lpi2c_data rv32m1_lpi2c_##id##_data = {           \
		.transfer_sync = Z_SEM_INITIALIZER(                            \
			rv32m1_lpi2c_##id##_data.transfer_sync, 1, 1),         \
		.completion_sync = Z_SEM_INITIALIZER(                          \
			rv32m1_lpi2c_##id##_data.completion_sync, 0, 1),       \
	};                                                                     \
	I2C_DEVICE_DT_INST_DEFINE(id,                                          \
			    rv32m1_lpi2c_init,                                 \
			    NULL,                                              \
			    &rv32m1_lpi2c_##id##_data,                         \
			    &rv32m1_lpi2c_##id##_config,                       \
			    POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,             \
			    &rv32m1_lpi2c_driver_api);	                       \
	static void rv32m1_lpi2c_irq_config_func_##id(const struct device *dev)      \
	{                                                                      \
		IRQ_CONNECT(DT_INST_IRQN(id),                                  \
			    0,						       \
			    rv32m1_lpi2c_isr, DEVICE_DT_INST_GET(id),	       \
			    0);                                                \
		irq_enable(DT_INST_IRQN(id));                                  \
	}                                                                      \

DT_INST_FOREACH_STATUS_OKAY(RV32M1_LPI2C_DEVICE)
