/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 * Copyright (c) 2024 Intel Corporation
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ll_stm32_common);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2)
#define DT_DRV_COMPAT st_stm32_i2c_v2
#else
#define DT_DRV_COMPAT st_stm32_i2c_v1
#endif

#include "i2c_stm32.h"

#ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
void i2c_stm32_combined_isr(void *arg)
{
	const struct device *dev = (const struct device *) arg;

	if (i2c_stm32_error(dev)) {
		return;
	}
	i2c_stm32_event(dev);
}
#else
void i2c_stm32_event_isr(void *arg)
{
	const struct device *dev = (const struct device *) arg;

	i2c_stm32_event(dev);
}

void i2c_stm32_error_isr(void *arg)
{
	const struct device *dev = (const struct device *) arg;

	(void)i2c_stm32_error(dev);
}
#endif

int i2c_stm32_activate(const struct device *dev)
{
	int ret;
	const struct i2c_stm32_config *cfg = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	/* Move pins to active/default state */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("I2C pinctrl setup failed (%d)", ret);
		return ret;
	}

	/* Enable device clock. */
	if (clock_control_on(clk,
			     (clock_control_subsys_t) &cfg->pclken[0]) != 0) {
		LOG_ERR("i2c: failure enabling clock");
		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
int i2c_stm32_suspend(const struct device *dev)
{
	int ret;
	const struct i2c_stm32_config *cfg = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	/* Disable device clock. */
	ret = clock_control_off(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("failure disabling I2C clock");
		return ret;
	}

	/* Move pins to sleep state */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_SLEEP);
	if (ret == -ENOENT) {
		/* Warn but don't block suspend */
		LOG_WRN("I2C pinctrl sleep state not available ");
	} else if (ret < 0) {
		return ret;
	}

	return 0;
}

int i2c_stm32_pm_action(const struct device *dev, enum pm_device_action action)
{
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		err = i2c_stm32_activate(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		err = i2c_stm32_suspend(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return err;
}
#endif

int i2c_stm32_pm_get(const struct device *dev)
{
	int ret;

	ret = pm_device_runtime_get(dev);
	if (ret == 0) {
		/* Prevent the clocks to be stopped during the i2c transaction */
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		if (IS_ENABLED(CONFIG_PM_S2RAM)) {
			pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
		}
	}

	return ret;
}

void i2c_stm32_pm_put(const struct device *dev)
{
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	if (IS_ENABLED(CONFIG_PM_S2RAM)) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	}
	(void)pm_device_runtime_put(dev);
}

/* I2C instances definitions */

#ifdef CONFIG_I2C_STM32_V2_DMA

#define I2C_DMA_INIT(index, dir)								\
	.dir##_dma = {										\
		.dev_dma = COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),			\
				(DEVICE_DT_GET(STM32_DMA_CTLR(index, dir))), (NULL)),		\
		.dma_channel = COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),			\
				(DT_INST_DMAS_CELL_BY_NAME(index, dir, channel)), (-1)),	\
	},

void i2c_stm32_dma_tx_cb(const struct device *dma_dev __unused, void *user_data __unused,
			 uint32_t channel __unused, int status)
{
	/* log DMA TX error */
	if (status != 0) {
		LOG_ERR("DMA error %d", status);
	}
}

void i2c_stm32_dma_rx_cb(const struct device *dma_dev __unused, void *user_data __unused,
			 uint32_t channel __unused, int status)
{
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
	BUILD_ASSERT(!IS_ENABLED(CONFIG_I2C_STM32_V2_DMA) ||					\
		     (DT_INST_DMAS_HAS_NAME(index, tx) == DT_INST_DMAS_HAS_NAME(index, rx)),	\
		     "STM32 I2C requires either none or both of rx and tx DMAs are used");	\
												\
	IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2), (				\
	static const uint32_t i2c_timings_##index[] = DT_INST_PROP_OR(index, timings, {});	\
	))											\
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
		IF_ENABLED(CONFIG_I2C_STM32_BUS_RECOVERY, (					\
		.scl = GPIO_DT_SPEC_INST_GET_OR(index, scl_gpios, {0}),				\
		.sda = GPIO_DT_SPEC_INST_GET_OR(index, sda_gpios, {0}),				\
		))										\
		IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2), (			\
		.timings = (const struct i2c_config_timing *)i2c_timings_##index,		\
		.n_timings = sizeof(i2c_timings_##index) / (sizeof(struct i2c_config_timing)),	\
		))										\
		I2C_DMA_INIT(index, tx)								\
		I2C_DMA_INIT(index, rx)								\
	};											\
												\
	IF_ENABLED(CONFIG_I2C_RTIO, (								\
		I2C_RTIO_DEFINE(CONCAT(_i2c, index, _stm32_rtio),				\
				DT_INST_PROP_OR(index, sq_size, CONFIG_I2C_RTIO_SQ_SIZE),	\
				DT_INST_PROP_OR(index, cq_size, CONFIG_I2C_RTIO_CQ_SIZE));	\
	))											\
												\
	static struct i2c_stm32_data i2c_stm32_dev_data_##index = {				\
		IF_ENABLED(CONFIG_I2C_RTIO, (							\
		.ctx = &CONCAT(_i2c, index, _stm32_rtio),					\
		))										\
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
				  &i2c_stm32_driver_api);					\
												\
	I2C_STM32_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(I2C_STM32_INIT)
