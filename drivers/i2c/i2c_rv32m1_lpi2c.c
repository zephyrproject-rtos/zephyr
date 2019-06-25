/*
 * Copyright (c) 2019, Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * Based on the i2c_mcux_lpi2c.c driver, which is:
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include <drivers/clock_control.h>
#include <fsl_lpi2c.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(rv32m1_lpi2c);

#include "i2c-priv.h"

struct rv32m1_lpi2c_config {
	LPI2C_Type *base;
	char *clock_controller;
	clock_control_subsys_t clock_subsys;
	clock_ip_name_t clock_ip_name;
	u32_t clock_ip_src;
	u32_t bitrate;
	void (*irq_config_func)(struct device *dev);
};

struct rv32m1_lpi2c_data {
	lpi2c_master_handle_t handle;
	struct k_sem transfer_sync;
	struct k_sem completion_sync;
	status_t completion_status;
};

static int rv32m1_lpi2c_configure(struct device *dev, u32_t dev_config)
{
	const struct rv32m1_lpi2c_config *config = dev->config->config_info;
	struct device *clk;
	u32_t baudrate;
	u32_t clk_freq;
	int err;

	if (!(I2C_MODE_MASTER & dev_config)) {
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

	clk = device_get_binding(config->clock_controller);
	if (!clk) {
		LOG_ERR("Could not get clock controller '%s'",
			config->clock_controller);
		return -EINVAL;
	}

	err = clock_control_get_rate(clk, config->clock_subsys, &clk_freq);
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
	struct device *dev = userData;
	struct rv32m1_lpi2c_data *data = dev->driver_data;

	ARG_UNUSED(base);
	ARG_UNUSED(handle);

	data->completion_status = completionStatus;
	k_sem_give(&data->completion_sync);
}

static u32_t rv32m1_lpi2c_convert_flags(int msg_flags)
{
	u32_t flags = 0U;

	if (!(msg_flags & I2C_MSG_STOP)) {
		flags |= kLPI2C_TransferNoStopFlag;
	}

	if (msg_flags & I2C_MSG_RESTART) {
		flags |= kLPI2C_TransferRepeatedStartFlag;
	}

	return flags;
}

static int rv32m1_lpi2c_transfer(struct device *dev, struct i2c_msg *msgs,
				 u8_t num_msgs, u16_t addr)
{
	const struct rv32m1_lpi2c_config *config = dev->config->config_info;
	struct rv32m1_lpi2c_data *data = dev->driver_data;
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

static void rv32m1_lpi2c_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct rv32m1_lpi2c_config *config = dev->config->config_info;
	struct rv32m1_lpi2c_data *data = dev->driver_data;

	LPI2C_MasterTransferHandleIRQ(config->base, &data->handle);
}

static int rv32m1_lpi2c_init(struct device *dev)
{
	const struct rv32m1_lpi2c_config *config = dev->config->config_info;
	struct rv32m1_lpi2c_data *data = dev->driver_data;
	lpi2c_master_config_t master_config;
	struct device *clk;
	u32_t clk_freq, dev_cfg;
	int err;

	CLOCK_SetIpSrc(config->clock_ip_name, config->clock_ip_src);

	clk = device_get_binding(config->clock_controller);
	if (!clk) {
		LOG_ERR("Could not get clock controller '%s'",
			config->clock_controller);
		return -EINVAL;
	}

	err = clock_control_on(clk, config->clock_subsys);
	if (err) {
		LOG_ERR("Could not turn on clock (err %d)", err);
		return -EINVAL;
	}

	err = clock_control_get_rate(clk, config->clock_subsys, &clk_freq);
	if (err) {
		LOG_ERR("Could not get clock frequency (err %d)", err);
		return -EINVAL;
	}

	LPI2C_MasterGetDefaultConfig(&master_config);
	LPI2C_MasterInit(config->base, &master_config, clk_freq);
	LPI2C_MasterTransferCreateHandle(config->base, &data->handle,
					 rv32m1_lpi2c_master_transfer_callback,
					 dev);

	dev_cfg = i2c_map_dt_bitrate(config->bitrate);
	err = rv32m1_lpi2c_configure(dev, dev_cfg | I2C_MODE_MASTER);
	if (err) {
		LOG_ERR("Could not configure controller (err %d)", err);
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
	static void rv32m1_lpi2c_irq_config_func_##id(struct device *dev);     \
	static const struct rv32m1_lpi2c_config rv32m1_lpi2c_##id##_config = { \
		.base =                                                        \
		(LPI2C_Type *)DT_OPENISA_RV32M1_LPI2C_I2C_##id##_BASE_ADDRESS, \
		.clock_controller =                                            \
			DT_OPENISA_RV32M1_LPI2C_I2C_##id##_CLOCK_CONTROLLER,   \
		.clock_subsys =                                                \
			(clock_control_subsys_t)                               \
			DT_OPENISA_RV32M1_LPI2C_I2C_##id##_CLOCK_NAME,         \
		.clock_ip_name = kCLOCK_Lpi2c##id,                             \
		.clock_ip_src  = kCLOCK_IpSrcFircAsync,                        \
		.bitrate = DT_OPENISA_RV32M1_LPI2C_I2C_##id##_CLOCK_FREQUENCY, \
		.irq_config_func = rv32m1_lpi2c_irq_config_func_##id,          \
	};                                                                     \
	static struct rv32m1_lpi2c_data rv32m1_lpi2c_##id##_data = {           \
		.transfer_sync = Z_SEM_INITIALIZER(                            \
			rv32m1_lpi2c_##id##_data.transfer_sync, 1, 1),         \
		.completion_sync = Z_SEM_INITIALIZER(                          \
			rv32m1_lpi2c_##id##_data.completion_sync, 0, 1),       \
	};                                                                     \
	DEVICE_AND_API_INIT(rv32m1_lpi2c_##id,                                 \
			    DT_OPENISA_RV32M1_LPI2C_I2C_##id##_LABEL,          \
			    &rv32m1_lpi2c_init,                                \
			    &rv32m1_lpi2c_##id##_data,                         \
			    &rv32m1_lpi2c_##id##_config,                       \
			    POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,             \
			    &rv32m1_lpi2c_driver_api);	                       \
	static void rv32m1_lpi2c_irq_config_func_##id(struct device *dev)      \
	{                                                                      \
		IRQ_CONNECT(DT_OPENISA_RV32M1_LPI2C_I2C_##id##_IRQ_0,          \
			    0,						       \
			    rv32m1_lpi2c_isr, DEVICE_GET(rv32m1_lpi2c_##id),   \
			    0);                                                \
		irq_enable(DT_OPENISA_RV32M1_LPI2C_I2C_##id##_IRQ_0);          \
	}                                                                      \

#ifdef CONFIG_I2C_0
RV32M1_LPI2C_DEVICE(0)
#endif

#ifdef CONFIG_I2C_1
RV32M1_LPI2C_DEVICE(1)
#endif

#ifdef CONFIG_I2C_2
RV32M1_LPI2C_DEVICE(2)
#endif

#ifdef CONFIG_I2C_3
RV32M1_LPI2C_DEVICE(3)
#endif
