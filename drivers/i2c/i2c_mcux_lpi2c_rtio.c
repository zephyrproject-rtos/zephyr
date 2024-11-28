/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2019-2023, NXP
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpi2c

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <fsl_lpi2c.h>

#include <zephyr/drivers/pinctrl.h>

#ifdef CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY
#include "i2c_bitbang.h"
#include <zephyr/drivers/gpio.h>
#endif /* CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcux_lpi2c);

#include "i2c-priv.h"
/* Wait for the duration of 12 bits to detect a NAK after a bus
 * address scan.  (10 appears sufficient, 20% safety factor.)
 */
#define SCAN_DELAY_US(baudrate) (12 * USEC_PER_SEC / baudrate)

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev) \
	((const struct mcux_lpi2c_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_lpi2c_data *)(_dev)->data)

struct mcux_lpi2c_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	uint32_t bitrate;
	uint32_t bus_idle_timeout_ns;
	const struct pinctrl_dev_config *pincfg;
#ifdef CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY
	struct gpio_dt_spec scl;
	struct gpio_dt_spec sda;
#endif /* CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY */
};

struct mcux_lpi2c_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	lpi2c_master_handle_t handle;
	struct i2c_rtio *ctx;
	lpi2c_master_transfer_t transfer;
#ifdef CONFIG_I2C_TARGET
	lpi2c_slave_handle_t target_handle;
	struct i2c_target_config *target_cfg;
	bool target_attached;
	bool first_tx;
	bool read_active;
	bool send_ack;
#endif
};

static int mcux_lpi2c_configure(const struct device *dev,
				uint32_t dev_config_raw)
{
	struct i2c_rtio *const ctx = ((struct mcux_lpi2c_data *)
		dev->data)->ctx;

	return i2c_rtio_configure(ctx, dev_config_raw);
}


static int  mcux_lpi2c_do_configure(const struct device *dev, uint32_t dev_config_raw)
{
	const struct mcux_lpi2c_config *config = dev->config;
	LPI2C_Type *base = (LPI2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint32_t clock_freq;
	uint32_t baudrate;

	if (!(I2C_MODE_CONTROLLER & dev_config_raw)) {
		return -EINVAL;
	}

	if (I2C_ADDR_10_BITS & dev_config_raw) {
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

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	LPI2C_MasterSetBaudRate(base, clock_freq, baudrate);

	return 0;
}


static uint32_t mcux_lpi2c_convert_flags(int msg_flags)
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

static bool mcux_lpi2c_msg_start(const struct device *dev, uint8_t flags,
				 uint8_t *buf, size_t buf_len, uint16_t i2c_addr)
{
	struct mcux_lpi2c_data *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	LPI2C_Type *base = (LPI2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	lpi2c_master_transfer_t *transfer = &data->transfer;
	status_t status;

	if (I2C_MSG_ADDR_10_BITS & flags) {
		return i2c_rtio_complete(ctx, -ENOTSUP);
	}

	/* Initialize the transfer descriptor */
	transfer->flags = mcux_lpi2c_convert_flags(flags);

	/* Prevent the controller to send a start condition between
	 * messages, except if explicitly requested.
	 */
	if (ctx->txn_curr != ctx->txn_head && !(flags & I2C_MSG_RESTART)) {
		transfer->flags |= kLPI2C_TransferNoStartFlag;
	}

	transfer->slaveAddress = i2c_addr;
	transfer->direction = (flags & I2C_MSG_READ)
		? kLPI2C_Read : kLPI2C_Write;
	transfer->subaddress = 0;
	transfer->subaddressSize = 0;
	transfer->data = buf;
	transfer->dataSize = buf_len;

	/* Start the transfer */
	status = LPI2C_MasterTransferNonBlocking(base,
			&data->handle, transfer);

	/* Return an error if the transfer didn't start successfully
	 * e.g., if the bus was busy
	 */
	if (status != kStatus_Success) {
		LPI2C_MasterTransferAbort(base, &data->handle);
		return i2c_rtio_complete(ctx, -EIO);
	}

	return false;
}

static void mcux_lpi2c_complete(const struct device *dev, int status);

static bool mcux_lpi2c_start(const struct device *dev)
{
	struct mcux_lpi2c_data *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	struct rtio_sqe *sqe = &ctx->txn_curr->sqe;
	struct i2c_dt_spec *dt_spec = sqe->iodev->data;

	int res = 0;

	switch (sqe->op) {
	case RTIO_OP_RX:
		return mcux_lpi2c_msg_start(dev, I2C_MSG_READ | sqe->iodev_flags,
					    sqe->rx.buf, sqe->rx.buf_len, dt_spec->addr);
	case RTIO_OP_TINY_TX:
		return mcux_lpi2c_msg_start(dev, I2C_MSG_WRITE | sqe->iodev_flags,
					    (uint8_t *)sqe->tiny_tx.buf, sqe->tiny_tx.buf_len,
					    dt_spec->addr);
	case RTIO_OP_TX:
		return mcux_lpi2c_msg_start(dev, I2C_MSG_WRITE | sqe->iodev_flags,
					    (uint8_t *)sqe->tx.buf, sqe->tx.buf_len,
					    dt_spec->addr);
	case RTIO_OP_I2C_CONFIGURE:
		res = mcux_lpi2c_do_configure(dev, sqe->i2c_config);
		return i2c_rtio_complete(data->ctx, res);
	default:
		LOG_ERR("Invalid op code %d for submission %p\n", sqe->op, (void *)sqe);
		return i2c_rtio_complete(data->ctx, -EINVAL);
	}
}

static void mcux_lpi2c_complete(const struct device *dev, status_t status)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = (LPI2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct i2c_rtio *const ctx = data->ctx;

	int ret = 0;

	/* Return an error if the transfer didn't complete
	 * successfully. e.g., nak, timeout, lost arbitration
	 */
	if (status != kStatus_Success) {
		LPI2C_MasterTransferAbort(base, &data->handle);
		ret = -EIO;
		goto out;
	}

	if (data->transfer.dataSize == 0) {
		k_busy_wait(SCAN_DELAY_US(config->bitrate));
		if (0 != (base->MSR & LPI2C_MSR_NDF_MASK)) {
			LPI2C_MasterTransferAbort(base, &data->handle);
			ret = -EIO;
			goto out;
		}
	}

out:
	if (i2c_rtio_complete(ctx, ret)) {
		mcux_lpi2c_start(dev);
	}
}

static void mcux_lpi2c_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct mcux_lpi2c_data *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	if (i2c_rtio_submit(ctx, iodev_sqe)) {
		mcux_lpi2c_start(dev);
	}
}


static void mcux_lpi2c_master_transfer_callback(LPI2C_Type *base,
						lpi2c_master_handle_t *handle,
						status_t status, void *userData)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(base);

	const struct device *dev = userData;

	mcux_lpi2c_complete(dev, status);
}

static int mcux_lpi2c_transfer(const struct device *dev, struct i2c_msg *msgs,
				   uint8_t num_msgs, uint16_t addr)
{
	struct i2c_rtio *const ctx = ((struct mcux_lpi2c_data *)
		dev->data)->ctx;

	return i2c_rtio_transfer(ctx, msgs, num_msgs, addr);
}

static void mcux_lpi2c_isr(const struct device *dev)
{
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = (LPI2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	LPI2C_MasterTransferHandleIRQ(base, &data->handle);
}

static int mcux_lpi2c_init(const struct device *dev)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base;
	uint32_t clock_freq, bitrate_cfg;
	lpi2c_master_config_t master_config;
	int error;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	base = (LPI2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	error = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (error) {
		return error;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	LPI2C_MasterGetDefaultConfig(&master_config);
	master_config.busIdleTimeout_ns = config->bus_idle_timeout_ns;
	LPI2C_MasterInit(base, &master_config, clock_freq);
	LPI2C_MasterTransferCreateHandle(base, &data->handle,
					 mcux_lpi2c_master_transfer_callback,
					 (void *)dev);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

	error = mcux_lpi2c_do_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (error) {
		return error;
	}

	config->irq_config_func(dev);

	i2c_rtio_init(data->ctx, dev);

	return 0;
}

static DEVICE_API(i2c, mcux_lpi2c_driver_api) = {
	.configure = mcux_lpi2c_configure,
	.transfer = mcux_lpi2c_transfer,
	.iodev_submit = mcux_lpi2c_submit,
};

#if CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY
#define I2C_MCUX_LPI2C_SCL_INIT(n) .scl = GPIO_DT_SPEC_INST_GET_OR(n, scl_gpios, {0}),
#define I2C_MCUX_LPI2C_SDA_INIT(n) .sda = GPIO_DT_SPEC_INST_GET_OR(n, sda_gpios, {0}),
#else
#define I2C_MCUX_LPI2C_SCL_INIT(n)
#define I2C_MCUX_LPI2C_SDA_INIT(n)
#endif /* CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY */

#define I2C_MCUX_LPI2C_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static void mcux_lpi2c_config_func_##n(const struct device *dev); \
									\
	static const struct mcux_lpi2c_config mcux_lpi2c_config_##n = {	\
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),	\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys =						\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
		.irq_config_func = mcux_lpi2c_config_func_##n,		\
		.bitrate = DT_INST_PROP(n, clock_frequency),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		I2C_MCUX_LPI2C_SCL_INIT(n)				\
		I2C_MCUX_LPI2C_SDA_INIT(n)				\
		.bus_idle_timeout_ns =					\
			UTIL_AND(DT_INST_NODE_HAS_PROP(n, bus_idle_timeout),\
				 DT_INST_PROP(n, bus_idle_timeout)),	\
	};								\
									\
	I2C_RTIO_DEFINE(_i2c##n##_lpi2c_rtio,				\
		DT_INST_PROP_OR(n, sq_size, CONFIG_I2C_RTIO_SQ_SIZE),	\
		DT_INST_PROP_OR(n, cq_size, CONFIG_I2C_RTIO_CQ_SIZE));	\
									\
	static struct mcux_lpi2c_data mcux_lpi2c_data_##n = {		\
		.ctx = &CONCAT(_i2c, n, _lpi2c_rtio),			\
	};								\
									\
	I2C_DEVICE_DT_INST_DEFINE(n, mcux_lpi2c_init, NULL,		\
				&mcux_lpi2c_data_##n,			\
				&mcux_lpi2c_config_##n, POST_KERNEL,	\
				CONFIG_I2C_INIT_PRIORITY,		\
				&mcux_lpi2c_driver_api);		\
									\
	static void mcux_lpi2c_config_func_##n(const struct device *dev)\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
				DT_INST_IRQ(n, priority),		\
				mcux_lpi2c_isr,				\
				DEVICE_DT_INST_GET(n), 0);		\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_MCUX_LPI2C_INIT)
