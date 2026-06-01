/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
#include <stm32_ll_i2c.h>
#include <stm32_ll_rcc.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ll_stm32_rtio);

#include "i2c_stm32.h"
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
#if CONFIG_I2C_STM32_BUS_RECOVERY
	case RTIO_OP_I2C_RECOVER:
		res = i2c_stm32_recover_bus(dev);
		return i2c_rtio_complete(data->ctx, res);
#endif
	default:
		LOG_ERR("Invalid op code %d for submission %p\n", sqe->op, (void *)sqe);
		return i2c_rtio_complete(data->ctx, -EINVAL);
	}
}

/* Start RTIO operations until there are no more queued or an async operation is pending */
static void i2c_stm32_start_next_async(const struct device *dev)
{
	while (i2c_stm32_start(dev)) {
		;
	}
}

void i2c_stm32_rtio_complete(const struct device *dev, int status)
{
	struct i2c_stm32_data *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	if (i2c_rtio_complete(ctx, status)) {
		i2c_stm32_start_next_async(dev);
	} else {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);
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
	 * reload mode for the message so that there is no Stop or Start conditions emitted
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
		if (pm_device_runtime_get(dev) < 0) {
			(void)i2c_rtio_complete(ctx, -EINVAL);
		} else {
			/* Prevent the clocks to be stopped during the i2c transaction */
			pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

			i2c_stm32_start_next_async(dev);
		}
	}
}

#ifdef CONFIG_I2C_STM32_BUS_RECOVERY
static int i2c_rtio_stm32_recover_bus(const struct device *dev)
{
	struct i2c_stm32_data *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;
	struct rtio_iodev_sqe *head_before;
	bool had_txn;
	int err;

	k_sem_take(&ctx->lock, K_FOREVER);

	/* ISR may finish the xfer during nrfx recover; only complete if txn_head is unchanged. */
	head_before = ctx->txn_head;
	had_txn = (head_before != NULL);

	err = i2c_stm32_recover_bus(dev);

	if (had_txn && ctx->txn_head == head_before) {
		i2c_stm32_rtio_complete(dev, err != 0 ? err : -ECONNABORTED);
	}

	k_sem_give(&ctx->lock);

	/* No in-flight RTIO txn: run the RTIO recover SQE (shell / i2c_rtio_recover). */
	if (err == 0 && !had_txn) {
		err = i2c_rtio_recover(ctx);
	}

	return err;
}
#endif /* CONFIG_I2C_STM32_BUS_RECOVERY */

static DEVICE_API(i2c, api_funcs) = {
	.configure = i2c_stm32_configure,
	.transfer = i2c_stm32_transfer,
	.get_config = i2c_stm32_get_config,
	.iodev_submit = i2c_stm32_submit,
#if CONFIG_I2C_STM32_BUS_RECOVERY
	.recover_bus = i2c_rtio_stm32_recover_bus,
#endif /* CONFIG_I2C_STM32_BUS_RECOVERY */
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
	 * So that they can enter controller mode properly.
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

#ifdef CONFIG_I2C_STM32_V2_DMA

#define I2C_DMA_INIT(index, dir)								\
	.dir##_dma = {										\
		.dev_dma = COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),			\
				(DEVICE_DT_GET(STM32_DMA_CTLR(index, dir))), (NULL)),		\
		.dma_channel = COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),			\
				(DT_INST_DMAS_CELL_BY_NAME(index, dir, channel)), (-1)),	\
	},

void i2c_stm32_dma_tx_cb(const struct device *dma_dev, void *user_data,
			 uint32_t channel, int status)
{
	ARG_UNUSED(dma_dev);
	ARG_UNUSED(user_data);
	ARG_UNUSED(channel);

	/* log DMA TX error */
	if (status != 0) {
		LOG_ERR("DMA error %d", status);
	}
}

void i2c_stm32_dma_rx_cb(const struct device *dma_dev, void *user_data,
			 uint32_t channel, int status)
{
	ARG_UNUSED(dma_dev);
	ARG_UNUSED(user_data);
	ARG_UNUSED(channel);

	/* log DMA RX error */
	if (status != 0) {
		LOG_ERR("DMA error %d", status);
	}
}

#define I2C_DMA_DATA_INIT(index, dir, src, dest)						\
	IF_ENABLED(DT_INST_DMAS_HAS_NAME(index, dir),						\
		(.dma_##dir##_cfg = {								\
			.dma_slot = STM32_DMA_SLOT(index, dir, slot),				\
			.channel_direction = STM32_DMA_CONFIG_DIRECTION(			\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),				\
			.cyclic =  STM32_DMA_CONFIG_CYCLIC(					\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),				\
			.channel_priority = STM32_DMA_CONFIG_PRIORITY(				\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),				\
			.source_data_size = STM32_DMA_CONFIG_##src##_DATA_SIZE(			\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),				\
			.dest_data_size = STM32_DMA_CONFIG_##dest##_DATA_SIZE(			\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),				\
			/* single transfers (burst length = data size) */			\
			.source_burst_length = STM32_DMA_CONFIG_##src##_DATA_SIZE(		\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),				\
			.dest_burst_length = STM32_DMA_CONFIG_##dest##_DATA_SIZE(		\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),				\
			.dma_callback = i2c_stm32_dma_##dir##_cb,				\
		},))

#else /* CONFIG_I2C_STM32_V2_DMA */

#define I2C_DMA_INIT(index, dir)
#define I2C_DMA_DATA_INIT(index, dir, src, dest)

#endif /* CONFIG_I2C_STM32_V2_DMA */

#define I2C_STM32_INIT(index)									\
	I2C_STM32_IRQ_HANDLER_DECL(index);							\
												\
	IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2),					\
		   (static const uint32_t i2c_timings_##index[] =				\
			DT_INST_PROP_OR(index, timings, {});))					\
												\
	PINCTRL_DT_INST_DEFINE(index);								\
												\
	static const struct stm32_pclken pclken_##index[] = STM32_DT_INST_CLOCKS(index);	\
												\
	static const struct i2c_stm32_config i2c_stm32_cfg_##index = {				\
		.i2c = (I2C_TypeDef *)DT_INST_REG_ADDR(index),					\
		.pclken = pclken_##index,							\
		.pclk_len = DT_INST_NUM_CLOCKS(index),						\
		I2C_STM32_IRQ_HANDLER_FUNCTION(index)						\
		.bitrate = DT_INST_PROP(index, clock_frequency),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),					\
		IF_ENABLED(CONFIG_I2C_STM32_BUS_RECOVERY,					\
			   (.scl = GPIO_DT_SPEC_INST_GET_OR(index, scl_gpios, {0}),		\
			    .sda = GPIO_DT_SPEC_INST_GET_OR(index, sda_gpios, {0}),))		\
		IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2),				\
			   (.timings = (const struct i2c_config_timing *)i2c_timings_##index,	\
			    .n_timings =							\
			sizeof(i2c_timings_##index) / (sizeof(struct i2c_config_timing)),))	\
		I2C_DMA_INIT(index, tx)								\
		I2C_DMA_INIT(index, rx)								\
	};											\
												\
	I2C_RTIO_DEFINE(CONCAT(_i2c, index, _stm32_rtio),					\
			DT_INST_PROP_OR(index, sq_size, CONFIG_I2C_RTIO_SQ_SIZE),		\
			DT_INST_PROP_OR(index, cq_size, CONFIG_I2C_RTIO_CQ_SIZE));		\
												\
	static struct i2c_stm32_data i2c_stm32_dev_data_##index = {				\
		.ctx = &CONCAT(_i2c, index, _stm32_rtio),					\
		I2C_DMA_DATA_INIT(index, tx, MEMORY, PERIPHERAL)				\
		I2C_DMA_DATA_INIT(index, rx, PERIPHERAL, MEMORY)				\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(index, i2c_stm32_pm_action);					\
												\
	I2C_DEVICE_DT_INST_DEFINE(index, i2c_stm32_init,					\
				  PM_DEVICE_DT_INST_GET(index),					\
				  &i2c_stm32_dev_data_##index,					\
				  &i2c_stm32_cfg_##index,					\
				  POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,			\
				  &api_funcs);							\
												\
	I2C_STM32_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(I2C_STM32_INIT)
