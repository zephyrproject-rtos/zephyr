/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <stm32_ll_i2c.h>
#include <stm32_ll_rcc.h>
#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ll_stm32_rtio);

#include "i2c_ll_stm32.h"
#include "i2c-priv.h"

#define DT_DRV_COMPAT st_stm32_i2c_v2


struct i2c_stm32_data_rtio {
	struct i2c_rtio *ctx;
	uint32_t dev_config;
	uint8_t *xfer_buf;
	uint8_t xfer_len;
	uint8_t xfer_flags;
};

static void i2c_stm32_disable_transfer_interrupts(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_DisableIT_TX(i2c);
	LL_I2C_DisableIT_RX(i2c);
	LL_I2C_DisableIT_STOP(i2c);
	LL_I2C_DisableIT_NACK(i2c);
	LL_I2C_DisableIT_TC(i2c);
}

static void i2c_stm32_enable_transfer_interrupts(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_EnableIT_STOP(i2c);
	LL_I2C_EnableIT_NACK(i2c);
	LL_I2C_EnableIT_TC(i2c);
	LL_I2C_EnableIT_ERR(i2c);
}

static void i2c_stm32_master_mode_end(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	i2c_stm32_disable_transfer_interrupts(dev);

	if (LL_I2C_IsEnabledReloadMode(i2c)) {
		LL_I2C_DisableReloadMode(i2c);
	}
}


static bool i2c_stm32_start(const struct device *dev);

void i2c_stm32_event(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	I2C_TypeDef *i2c = cfg->i2c;
	int ret = 0;

	if (data->xfer_len) {
		/* Send next byte */
		if (LL_I2C_IsActiveFlag_TXIS(i2c)) {
			LL_I2C_TransmitData8(i2c, *data->xfer_buf);
		}

		/* Receive next byte */
		if (LL_I2C_IsActiveFlag_RXNE(i2c)) {
			*data->xfer_buf = LL_I2C_ReceiveData8(i2c);
		}

		data->xfer_buf++;
		data->xfer_len--;
	}

	/* NACK received */
	if (LL_I2C_IsActiveFlag_NACK(i2c)) {
		LL_I2C_ClearFlag_NACK(i2c);
		/*
		 * AutoEndMode is always disabled in master mode,
		 * so send a stop condition manually
		 */
		LL_I2C_GenerateStopCondition(i2c);
		ret = -EIO;
	}

	/* STOP received */
	if (LL_I2C_IsActiveFlag_STOP(i2c)) {
		LL_I2C_ClearFlag_STOP(i2c);
		LL_I2C_DisableReloadMode(i2c);
		i2c_stm32_master_mode_end(dev);
	}

	/* TODO handle the reload separately from complete */
	/* Transfer Complete or Transfer Complete Reload */
	if (LL_I2C_IsActiveFlag_TC(i2c) ||
	    LL_I2C_IsActiveFlag_TCR(i2c)) {
		/* Issue stop condition if necessary */
		/* TODO look at current sqe flags */
		if (data->xfer_flags & I2C_MSG_STOP) {
			LL_I2C_GenerateStopCondition(i2c);
		} else {
			i2c_stm32_disable_transfer_interrupts(dev);
		}

		if (data->xfer_len == 0 && i2c_rtio_complete(ctx, ret)) {
			i2c_stm32_start(dev);
		}
	}
}

int i2c_stm32_error(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	I2C_TypeDef *i2c = cfg->i2c;
	int ret;

	if (LL_I2C_IsActiveFlag_ARLO(i2c)) {
		LL_I2C_ClearFlag_ARLO(i2c);
		ret = -EIO;
	}

	if (LL_I2C_IsActiveFlag_BERR(i2c)) {
		LL_I2C_ClearFlag_BERR(i2c);
		ret = -EIO;
	}

	if (ret) {
		i2c_stm32_master_mode_end(dev);
		if (i2c_rtio_complete(ctx, ret)) {
			i2c_stm32_start(dev);
		}
	}

	return ret;
}

static int i2c_stm32_msg_start(const struct device *dev, uint8_t flags,
	uint8_t *buf, size_t buf_len, uint16_t i2c_addr)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t transfer;

	data->xfer_buf = buf;
	data->xfer_len = buf_len;
	data->xfer_flags = flags;

	if (flags & I2C_MSG_READ) {
		transfer = LL_I2C_REQUEST_READ;
	} else {
		transfer = LL_I2C_REQUEST_WRITE;
	}

	/* TODO deal with larger than 255 byte transfers correctly */
	if (buf_len > UINT8_MAX) {
		/* TODO LL_I2C_EnableReloadMode(i2c); */
		return -EINVAL;
	}

	if (I2C_ADDR_10_BITS & data->dev_config) {
		LL_I2C_SetMasterAddressingMode(i2c,
				LL_I2C_ADDRESSING_MODE_10BIT);
		LL_I2C_SetSlaveAddr(i2c, (uint32_t) i2c_addr);
	} else {
		LL_I2C_SetMasterAddressingMode(i2c,
			LL_I2C_ADDRESSING_MODE_7BIT);
		LL_I2C_SetSlaveAddr(i2c, (uint32_t) i2c_addr << 1);
	}

	LL_I2C_DisableAutoEndMode(i2c);
	LL_I2C_SetTransferRequest(i2c, transfer);
	LL_I2C_SetTransferSize(i2c, buf_len);

	LL_I2C_Enable(i2c);

	LL_I2C_GenerateStartCondition(i2c);

	i2c_stm32_enable_transfer_interrupts(dev);
	if (flags & I2C_MSG_READ) {
		LL_I2C_EnableIT_RX(i2c);
	} else {
		LL_I2C_EnableIT_TX(i2c);
	}

	return 0;
}

int i2c_stm32_configure_timing(const struct device *dev, uint32_t clock)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t i2c_hold_time_min, i2c_setup_time_min;
	uint32_t i2c_h_min_time, i2c_l_min_time;
	uint32_t presc = 1U;
	uint32_t timing = 0U;

	/*  Look for an adequate preset timing value */
	for (uint32_t i = 0; i < cfg->n_timings; i++) {
		const struct i2c_config_timing *preset = &cfg->timings[i];
		uint32_t speed = i2c_map_dt_bitrate(preset->i2c_speed);

		if ((I2C_SPEED_GET(speed) == I2C_SPEED_GET(data->dev_config))
		   && (preset->periph_clock == clock)) {
			/*  Found a matching periph clock and i2c speed */
			LL_I2C_SetTiming(i2c, preset->timing_setting);
			return 0;
		}
	}

	/* No preset timing was provided, let's dynamically configure */
	switch (I2C_SPEED_GET(data->dev_config)) {
	case I2C_SPEED_STANDARD:
		i2c_h_min_time = 4000U;
		i2c_l_min_time = 4700U;
		i2c_hold_time_min = 500U;
		i2c_setup_time_min = 1250U;
		break;
	case I2C_SPEED_FAST:
		i2c_h_min_time = 600U;
		i2c_l_min_time = 1300U;
		i2c_hold_time_min = 375U;
		i2c_setup_time_min = 500U;
		break;
	default:
		LOG_ERR("i2c: speed above \"fast\" requires manual timing configuration, "
				"see \"timings\" property of st,stm32-i2c-v2 devicetree binding");
		return -EINVAL;
	}

	/* Calculate period until prescaler matches */
	do {
		uint32_t t_presc = clock / presc;
		uint32_t ns_presc = NSEC_PER_SEC / t_presc;
		uint32_t sclh = i2c_h_min_time / ns_presc;
		uint32_t scll = i2c_l_min_time / ns_presc;
		uint32_t sdadel = i2c_hold_time_min / ns_presc;
		uint32_t scldel = i2c_setup_time_min / ns_presc;

		if ((sclh - 1) > 255 ||  (scll - 1) > 255) {
			++presc;
			continue;
		}

		if (sdadel > 15 || (scldel - 1) > 15) {
			++presc;
			continue;
		}

		timing = __LL_I2C_CONVERT_TIMINGS(presc - 1,
					scldel - 1, sdadel, sclh - 1, scll - 1);
		break;
	} while (presc < 16);

	if (presc >= 16U) {
		LOG_DBG("I2C:failed to find prescaler value");
		return -EINVAL;
	}

	LL_I2C_SetTiming(i2c, timing);

	return 0;
}

static int i2c_stm32_do_configure(const struct device *dev, uint32_t config)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t i2c_clock = 0U;
	int ret;

	if (IS_ENABLED(STM32_I2C_DOMAIN_CLOCK_SUPPORT) && (cfg->pclk_len > 1)) {
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
		LOG_ERR("failure Enabling I2C clock");
		return ret;
	}
#endif

	LL_I2C_Disable(i2c);
	ret = i2c_stm32_configure_timing(dev, i2c_clock);

#ifdef CONFIG_PM_DEVICE_RUNTIME
	ret = clock_control_off(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("failure disabling I2C clock");
		return ret;
	}
#endif

	return ret;
}


static bool i2c_stm32_start(const struct device *dev)
{
	struct i2c_stm32_data_rtio *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	struct rtio_sqe *sqe = &ctx->txn_curr->sqe;
	struct i2c_dt_spec *dt_spec = sqe->iodev->data;

	int res = 0;

	switch (sqe->op) {
	case RTIO_OP_RX:
		return i2c_stm32_msg_start(dev, I2C_MSG_READ | sqe->iodev_flags,
				      sqe->buf, sqe->buf_len, dt_spec->addr);
	case RTIO_OP_TINY_TX:
		return i2c_stm32_msg_start(dev,  sqe->iodev_flags,
				      sqe->tiny_buf, sqe->tiny_buf_len, dt_spec->addr);
	case RTIO_OP_TX:
		return i2c_stm32_msg_start(dev, sqe->iodev_flags,
				      sqe->buf, sqe->buf_len, dt_spec->addr);
	case RTIO_OP_I2C_CONFIGURE:
		res = i2c_stm32_do_configure(dev, sqe->i2c_config);
		return i2c_rtio_complete(data->ctx, res);
	default:
		LOG_ERR("Invalid op code %d for submission %p\n", sqe->op, (void *)sqe);
		return i2c_rtio_complete(data->ctx, -EINVAL);
	}
}


static int i2c_stm32_configure(const struct device *dev,
				uint32_t dev_config_raw)
{
	struct i2c_stm32_data_rtio *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	return i2c_rtio_configure(ctx, dev_config_raw);
}

static int i2c_stm32_transfer(const struct device *dev, struct i2c_msg *msgs,
				   uint8_t num_msgs, uint16_t addr)
{
	struct i2c_stm32_data_rtio *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	return i2c_rtio_transfer(ctx, msgs, num_msgs, addr);
}


int i2c_stm32_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_stm32_data_rtio *data = dev->data;

	*config = data->dev_config;

	return 0;
}

static void i2c_stm32_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct i2c_stm32_data_rtio *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	if (i2c_rtio_submit(ctx, iodev_sqe)) {
		i2c_stm32_start(dev);
	}
}

static const struct i2c_driver_api api_funcs = {
	.configure = i2c_stm32_configure,
	.transfer = i2c_stm32_transfer,
	.get_config = i2c_stm32_get_config,
	.iodev_submit = i2c_stm32_submit,
};


static int i2c_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct i2c_stm32_config *cfg = dev->config;
	uint32_t bitrate_cfg;
	int ret;
	struct i2c_stm32_data_rtio *data = dev->data;

	cfg->irq_config_func(dev);

	i2c_rtio_init(data->ctx, dev);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	i2c_stm32_activate(dev);

	if (IS_ENABLED(STM32_I2C_DOMAIN_CLOCK_SUPPORT) && (cfg->pclk_len > 1)) {
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

	ret = i2c_stm32_do_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("i2c: failure initializing");
		return ret;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	(void)pm_device_runtime_enable(dev);
#endif

	return 0;
}

#define STM32_I2C_INIT(index)						\
STM32_I2C_IRQ_HANDLER_DECL(index);					\
									\
IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2),			\
	(static const uint32_t i2c_timings_##index[] =			\
		DT_INST_PROP_OR(index, timings, {});))			\
									\
PINCTRL_DT_INST_DEFINE(index);						\
									\
static const struct stm32_pclken pclken_##index[] =			\
				 STM32_DT_INST_CLOCKS(index);		\
									\
static const struct i2c_stm32_config i2c_stm32_cfg_##index = {		\
	.i2c = (I2C_TypeDef *)DT_INST_REG_ADDR(index),			\
	.pclken = pclken_##index,					\
	.pclk_len = DT_INST_NUM_CLOCKS(index),				\
	STM32_I2C_IRQ_HANDLER_FUNCTION(index)				\
	.bitrate = DT_INST_PROP(index, clock_frequency),		\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),			\
	IF_ENABLED(CONFIG_I2C_STM32_BUS_RECOVERY,			\
		(.scl =	GPIO_DT_SPEC_INST_GET_OR(index, scl_gpios, {0}),\
		 .sda = GPIO_DT_SPEC_INST_GET_OR(index, sda_gpios, {0}),))\
	IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2),		\
		(.timings = (const struct i2c_config_timing *) i2c_timings_##index,\
		 .n_timings = ARRAY_SIZE(i2c_timings_##index),))	\
};									\
									\
I2C_RTIO_DEFINE(CONCAT(_i2c, index, _stm32_rtio),			\
	DT_INST_PROP_OR(index, sq_size, CONFIG_I2C_RTIO_SQ_SIZE),	\
	DT_INST_PROP_OR(index, cq_size, CONFIG_I2C_RTIO_CQ_SIZE));	\
									\
static struct i2c_stm32_data_rtio i2c_stm32_dev_data_##index = {	\
	.ctx = &CONCAT(_i2c, index, _stm32_rtio),			\
};									\
									\
PM_DEVICE_DT_INST_DEFINE(index, i2c_stm32_pm_action);			\
									\
I2C_DEVICE_DT_INST_DEFINE(index, i2c_stm32_init,			\
			 PM_DEVICE_DT_INST_GET(index),			\
			 &i2c_stm32_dev_data_##index,			\
			 &i2c_stm32_cfg_##index,			\
			 POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,		\
			 &api_funcs);					\
									\
STM32_I2C_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(STM32_I2C_INIT)
