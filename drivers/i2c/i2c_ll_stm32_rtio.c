/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
#include <stm32_ll_i2c.h>
#include <stm32_ll_rcc.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ll_stm32_rtio);

#include "i2c_ll_stm32.h"
#include "i2c-priv.h"

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2)
#define DT_DRV_COMPAT st_stm32_i2c_v2
#else
#define DT_DRV_COMPAT st_stm32_i2c_v1
#endif

/* This symbol takes the value 1 if one of the device instances */
/* is configured in dts with a domain clock */
#if STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define I2C_STM32_DOMAIN_CLOCK_SUPPORT 1
#else
#define I2C_STM32_DOMAIN_CLOCK_SUPPORT 0
#endif

int i2c_stm32_runtime_configure(const struct device *dev, uint32_t config)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t i2c_clock = 0U;
	int ret;

	if (IS_ENABLED(I2C_STM32_DOMAIN_CLOCK_SUPPORT) && (cfg->pclk_len > 1)) {
		if (clock_control_get_rate(clk, (clock_control_subsys_t)&cfg->pclken[1],
					   &i2c_clock) < 0) {
			LOG_ERR("Failed call clock_control_get_rate(pclken[1])");
			return -EIO;
		}
	} else {
		if (clock_control_get_rate(clk, (clock_control_subsys_t)&cfg->pclken[0],
					   &i2c_clock) < 0) {
			LOG_ERR("Failed call clock_control_get_rate(pclken[0])");
			return -EIO;
		}
	}

	data->dev_config = config;

#ifdef CONFIG_PM_DEVICE_RUNTIME
	ret = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("Failed enabling I2C clock");
		return ret;
	}
#endif

	LL_I2C_Disable(i2c);
	ret = i2c_stm32_configure_timing(dev, i2c_clock);
	if (ret < 0) {
		LOG_ERR("Failed configuring I2C timing");
		return ret;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	ret = clock_control_off(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("Failed disabling I2C clock");
		return ret;
	}
#endif

	return ret;
}

bool i2c_stm32_start(const struct device *dev)
{
	struct i2c_stm32_data *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	struct rtio_sqe *sqe = &ctx->txn_curr->sqe;
	struct i2c_dt_spec *dt_spec = sqe->iodev->data;
	uint8_t flags = sqe->iodev_flags;
	int res = 0;

#ifdef CONFIG_I2C_STM32_V2
	struct rtio_iodev_sqe *iodev_sqe_next = rtio_txn_next(ctx->txn_curr);

	if ((iodev_sqe_next != NULL) &&
	    ((sqe->iodev_flags & I2C_MSG_STOP) == 0U) &&
	    ((iodev_sqe_next->sqe.iodev_flags & I2C_MSG_RESTART) == 0U)) {
		flags |= I2C_MSG_STM32_USE_RELOAD_MODE;
	}
#endif

	switch (sqe->op) {
	case RTIO_OP_RX:
		return i2c_stm32_msg_start(dev, I2C_MSG_READ | flags, sqe->rx.buf,
					   sqe->rx.buf_len, dt_spec->addr);
	case RTIO_OP_TINY_TX:
		return i2c_stm32_msg_start(dev, flags, sqe->tiny_tx.buf,
					   sqe->tiny_tx.buf_len, dt_spec->addr);
	case RTIO_OP_TX:
		return i2c_stm32_msg_start(dev, flags, (uint8_t *)sqe->tx.buf,
					   sqe->tx.buf_len, dt_spec->addr);
	case RTIO_OP_I2C_CONFIGURE:
		res = i2c_stm32_runtime_configure(dev, sqe->i2c_config);
		return i2c_rtio_complete(data->ctx, res);
	default:
		LOG_ERR("Invalid op code %d for submission %p\n", sqe->op, (void *)sqe);
		return i2c_rtio_complete(data->ctx, -EINVAL);
	}
}

static int i2c_stm32_configure(const struct device *dev,
				uint32_t dev_config_raw)
{
	struct i2c_stm32_data *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	return i2c_rtio_configure(ctx, dev_config_raw);
}

#define OPERATION(msg)	((msg)->flags & I2C_MSG_RW_MASK)

static int i2c_stm32_transfer(const struct device *dev, struct i2c_msg *msgs,
				   uint8_t num_msgs, uint16_t addr)
{
	struct i2c_stm32_data *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	/* Always set I2C_MSG_RESTART flag on first message in order to send start condition */
	msgs[0].flags |= I2C_MSG_RESTART;

#ifdef CONFIG_I2C_STM32_V2
	/*
	 * If a message has no STOP flag and next has no RESTART flag, set private flag
	 * I2C_MSG_STM32_USE_RELOAD_MODE in message flag to force STM32 v2 driver to enable
	 * reload mode for the message so that there is no Stop or Start conditions emited
	 * in between. This means that flags shall not be used by the generic I2C framework.
	 */
	if ((msgs[0].flags & I2C_MSG_STM32_USE_RELOAD_MODE) != 0U) {
		LOG_ERR("Unexpected bit mask 0x%02lx set in I2C message",
			I2C_MSG_STM32_USE_RELOAD_MODE);
		return -EINVAL;
	}
#endif

	for (size_t n = 1; n < num_msgs; n++) {
#ifdef CONFIG_I2C_STM32_V2
		if ((msgs[n].flags & I2C_MSG_STM32_USE_RELOAD_MODE) != 0U) {
			LOG_ERR("Unexpected bit mask 0x%02lx set in I2C message",
				I2C_MSG_STM32_USE_RELOAD_MODE);
			return -EINVAL;
		}
#endif

		if ((OPERATION(msgs + n - 1) != OPERATION(msgs + n)) &&
		    ((msgs[n].flags & I2C_MSG_RESTART) == 0U)) {
			LOG_ERR("Missing restart flag between message of different directions");
			return -EINVAL;
		}

		if ((msgs[n - 1].flags & I2C_MSG_STOP) != 0U) {
			LOG_ERR("Stop condition is only allowed on last message");
			return -EINVAL;
		}
	}

	return i2c_rtio_transfer(ctx, msgs, num_msgs, addr);
}

int i2c_stm32_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_stm32_data *data = dev->data;

	*config = data->dev_config;

	return 0;
}

static void i2c_stm32_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct i2c_stm32_data *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	/* Always set I2C_MSG_RESTART flag on first message in order to send start condition */
	iodev_sqe->sqe.iodev_flags |= RTIO_IODEV_I2C_RESTART;

	if (i2c_rtio_submit(ctx, iodev_sqe)) {
		i2c_stm32_start(dev);
	}
}

static const struct i2c_driver_api api_funcs = {
	.configure = i2c_stm32_configure,
	.transfer = i2c_stm32_transfer,
	.get_config = i2c_stm32_get_config,
	.iodev_submit = i2c_stm32_submit,
#if defined(CONFIG_I2C_TARGET)
	.target_register = i2c_stm32_target_register,
	.target_unregister = i2c_stm32_target_unregister,
#endif
};

static int i2c_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct i2c_stm32_config *cfg = dev->config;
	uint32_t bitrate_cfg;
	int ret;
	struct i2c_stm32_data *data = dev->data;

	cfg->irq_config_func(dev);

	i2c_rtio_init(data->ctx, dev);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	i2c_stm32_activate(dev);

	if (IS_ENABLED(I2C_STM32_DOMAIN_CLOCK_SUPPORT) && (cfg->pclk_len > 1)) {
		/* Enable I2C clock source */
		ret = clock_control_configure(clk,
					(clock_control_subsys_t) &cfg->pclken[1],
					NULL);
		if (ret < 0) {
			return -EIO;
		}
	}

#if defined(CONFIG_SOC_SERIES_STM32F1X)
	/*
	 * Force i2c reset for STM32F1 series.
	 * So that they can enter master mode properly.
	 * Issue described in ES096 2.14.7
	 */
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_EnableReset(i2c);
	LL_I2C_DisableReset(i2c);
#endif

	bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);

	ret = i2c_stm32_runtime_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("i2c: failure initializing");
		return ret;
	}

	(void)pm_device_runtime_enable(dev);

	return 0;
}

#define I2C_STM32_INIT(index)									\
I2C_STM32_IRQ_HANDLER_DECL(index);								\
												\
IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2),						\
	(static const uint32_t i2c_timings_##index[] =						\
		DT_INST_PROP_OR(index, timings, {});))						\
												\
PINCTRL_DT_INST_DEFINE(index);									\
												\
static const struct stm32_pclken pclken_##index[] =						\
				 STM32_DT_INST_CLOCKS(index);					\
												\
static const struct i2c_stm32_config i2c_stm32_cfg_##index = {					\
	.i2c = (I2C_TypeDef *)DT_INST_REG_ADDR(index),						\
	.pclken = pclken_##index,								\
	.pclk_len = DT_INST_NUM_CLOCKS(index),							\
	I2C_STM32_IRQ_HANDLER_FUNCTION(index)							\
	.bitrate = DT_INST_PROP(index, clock_frequency),					\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),						\
	IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2),					\
		(.timings = (const struct i2c_config_timing *) i2c_timings_##index,		\
		.n_timings =									\
			sizeof(i2c_timings_##index) / (sizeof(struct i2c_config_timing)),))	\
};												\
												\
I2C_RTIO_DEFINE(CONCAT(_i2c, index, _stm32_rtio),						\
	DT_INST_PROP_OR(index, sq_size, CONFIG_I2C_RTIO_SQ_SIZE),				\
	DT_INST_PROP_OR(index, cq_size, CONFIG_I2C_RTIO_CQ_SIZE));				\
												\
static struct i2c_stm32_data i2c_stm32_dev_data_##index = {					\
	.ctx = &CONCAT(_i2c, index, _stm32_rtio),						\
};												\
												\
PM_DEVICE_DT_INST_DEFINE(index, i2c_stm32_pm_action);						\
												\
I2C_DEVICE_DT_INST_DEFINE(index, i2c_stm32_init,						\
			 PM_DEVICE_DT_INST_GET(index),						\
			 &i2c_stm32_dev_data_##index,						\
			 &i2c_stm32_cfg_##index,						\
			 POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,					\
			 &api_funcs);								\
												\
I2C_STM32_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(I2C_STM32_INIT)
