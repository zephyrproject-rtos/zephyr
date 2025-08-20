/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <stm32_ll_i2c.h>
#include <stm32_ll_rcc.h>
#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#ifdef CONFIG_I2C_STM32_BUS_RECOVERY
#include "i2c_bitbang.h"
#endif /* CONFIG_I2C_STM32_BUS_RECOVERY */

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ll_stm32);

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

int i2c_stm32_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_stm32_data *data = dev->data;

	if (!data->is_configured) {
		LOG_ERR("I2C controller not configured");
		return -EIO;
	}

	*config = data->dev_config;

#if CONFIG_I2C_STM32_V2_TIMING
	/* Print the timing parameter of device data */
	LOG_INF("I2C timing value, report to the DTS :");

	/* I2C BIT RATE */
	if (data->current_timing.i2c_speed == 100000) {
		LOG_INF("timings = <%d I2C_BITRATE_STANDARD 0x%X>;",
			data->current_timing.periph_clock,
			data->current_timing.timing_setting);
	} else if (data->current_timing.i2c_speed == 400000) {
		LOG_INF("timings = <%d I2C_BITRATE_FAST 0x%X>;",
			data->current_timing.periph_clock,
			data->current_timing.timing_setting);
	} else if (data->current_timing.i2c_speed == 1000000) {
		LOG_INF("timings = <%d I2C_SPEED_FAST_PLUS 0x%X>;",
			data->current_timing.periph_clock,
			data->current_timing.timing_setting);
	}
#endif /* CONFIG_I2C_STM32_V2_TIMING */

	return 0;
}

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

	k_sem_take(&data->bus_mutex, K_FOREVER);

#ifdef CONFIG_PM_DEVICE_RUNTIME
	ret = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("failure Enabling I2C clock");
		return ret;
	}
#endif

	LL_I2C_Disable(i2c);
#if defined(I2C_CR1_SMBUS) || defined(I2C_CR1_SMBDEN) || defined(I2C_CR1_SMBHEN)
	i2c_stm32_set_smbus_mode(dev, data->mode);
#endif
	ret = i2c_stm32_configure_timing(dev, i2c_clock);

	if (data->smbalert_active) {
		LL_I2C_Enable(i2c);
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	ret = clock_control_off(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("failure disabling I2C clock");
		return ret;
	}
#endif

	k_sem_give(&data->bus_mutex);

	return ret;
}

#define OPERATION(msg) (((struct i2c_msg *) msg)->flags & I2C_MSG_RW_MASK)

static int i2c_stm32_transfer(const struct device *dev, struct i2c_msg *msg,
			      uint8_t num_msgs, uint16_t slave)
{
	struct i2c_stm32_data *data = dev->data;
	struct i2c_msg *current;
	struct i2c_msg *next = NULL;
	int ret = 0;

	/* Check for validity of all messages, to prevent having to abort
	 * in the middle of a transfer
	 */
	current = msg;

	/*
	 * Set I2C_MSG_RESTART flag on first message in order to send start
	 * condition
	 */
	current->flags |= I2C_MSG_RESTART;

	for (uint8_t i = 1; i <= num_msgs; i++) {

		if (i < num_msgs) {
			next = current + 1;

			/*
			 * Restart condition between messages
			 * of different directions is required
			 */
			if (OPERATION(current) != OPERATION(next)) {
				if (!(next->flags & I2C_MSG_RESTART)) {
					ret = -EINVAL;
					break;
				}
			}

			/* Stop condition is only allowed on last message */
			if (current->flags & I2C_MSG_STOP) {
				ret = -EINVAL;
				break;
			}
		}

		current++;
	}

	if (ret) {
		return ret;
	}

	/* Send out messages */
	k_sem_take(&data->bus_mutex, K_FOREVER);

	/* Prevent driver from being suspended by PM until I2C transaction is complete */
	(void)pm_device_runtime_get(dev);

	/* Prevent the clocks to be stopped during the i2c transaction */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	current = msg;

	while (num_msgs > 0) {
		uint8_t *next_msg_flags = NULL;

		if (num_msgs > 1) {
			next = current + 1;
			next_msg_flags = &(next->flags);
		}
		ret = i2c_stm32_transaction(dev, *current, next_msg_flags, slave);
		if (ret < 0) {
			break;
		}
		current++;
		num_msgs--;
	}

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	(void)pm_device_runtime_put(dev);

	k_sem_give(&data->bus_mutex);

	return ret;
}

#if CONFIG_I2C_STM32_BUS_RECOVERY
static void i2c_stm32_bitbang_set_scl(void *io_context, int state)
{
	const struct i2c_stm32_config *config = io_context;

	gpio_pin_set_dt(&config->scl, state);
}

static void i2c_stm32_bitbang_set_sda(void *io_context, int state)
{
	const struct i2c_stm32_config *config = io_context;

	gpio_pin_set_dt(&config->sda, state);
}

static int i2c_stm32_bitbang_get_sda(void *io_context)
{
	const struct i2c_stm32_config *config = io_context;

	return gpio_pin_get_dt(&config->sda) == 0 ? 0 : 1;
}

static int i2c_stm32_recover_bus(const struct device *dev)
{
	const struct i2c_stm32_config *config = dev->config;
	struct i2c_stm32_data *data = dev->data;
	struct i2c_bitbang bitbang_ctx;
	struct i2c_bitbang_io bitbang_io = {
		.set_scl = i2c_stm32_bitbang_set_scl,
		.set_sda = i2c_stm32_bitbang_set_sda,
		.get_sda = i2c_stm32_bitbang_get_sda,
	};
	uint32_t bitrate_cfg;
	int error = 0;

	LOG_ERR("attempting to recover bus");

	if (!gpio_is_ready_dt(&config->scl)) {
		LOG_ERR("SCL GPIO device not ready");
		return -EIO;
	}

	if (!gpio_is_ready_dt(&config->sda)) {
		LOG_ERR("SDA GPIO device not ready");
		return -EIO;
	}

	k_sem_take(&data->bus_mutex, K_FOREVER);

	error = gpio_pin_configure_dt(&config->scl, GPIO_OUTPUT_HIGH);
	if (error != 0) {
		LOG_ERR("failed to configure SCL GPIO (err %d)", error);
		goto restore;
	}

	error = gpio_pin_configure_dt(&config->sda, GPIO_OUTPUT_HIGH);
	if (error != 0) {
		LOG_ERR("failed to configure SDA GPIO (err %d)", error);
		goto restore;
	}

	i2c_bitbang_init(&bitbang_ctx, &bitbang_io, (void *)config);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate) | I2C_MODE_CONTROLLER;
	error = i2c_bitbang_configure(&bitbang_ctx, bitrate_cfg);
	if (error != 0) {
		LOG_ERR("failed to configure I2C bitbang (err %d)", error);
		goto restore;
	}

	error = i2c_bitbang_recover_bus(&bitbang_ctx);
	if (error != 0) {
		LOG_ERR("failed to recover bus (err %d)", error);
	}

restore:
	(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	k_sem_give(&data->bus_mutex);

	return error;
}
#endif /* CONFIG_I2C_STM32_BUS_RECOVERY */

static DEVICE_API(i2c, api_funcs) = {
	.configure = i2c_stm32_runtime_configure,
	.transfer = i2c_stm32_transfer,
	.get_config = i2c_stm32_get_config,
#if CONFIG_I2C_STM32_BUS_RECOVERY
	.recover_bus = i2c_stm32_recover_bus,
#endif /* CONFIG_I2C_STM32_BUS_RECOVERY */
#if defined(CONFIG_I2C_TARGET)
	.target_register = i2c_stm32_target_register,
	.target_unregister = i2c_stm32_target_unregister,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

static int i2c_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct i2c_stm32_config *cfg = dev->config;
	uint32_t bitrate_cfg;
	int ret;
	struct i2c_stm32_data *data = dev->data;
#ifdef CONFIG_I2C_STM32_INTERRUPT
	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);
	cfg->irq_config_func(dev);
#endif

	data->is_configured = false;
	data->mode = I2CSTM32MODE_I2C;

	/*
	 * initialize mutex used when multiple transfers
	 * are taking place to guarantee that each one is
	 * atomic and has exclusive access to the I2C bus.
	 */
	k_sem_init(&data->bus_mutex, 1, 1);

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

	data->is_configured = true;

	return 0;
}

#ifdef CONFIG_SMBUS_STM32_SMBALERT
void i2c_stm32_smbalert_set_callback(const struct device *dev, i2c_stm32_smbalert_cb_func_t func,
				     const struct device *cb_dev)
{
	struct i2c_stm32_data *data = dev->data;

	data->smbalert_cb_func = func;
	data->smbalert_cb_dev = cb_dev;
}
#endif /* CONFIG_SMBUS_STM32_SMBALERT */

#if defined(I2C_CR1_SMBUS) || defined(I2C_CR1_SMBDEN) || defined(I2C_CR1_SMBHEN)
void i2c_stm32_set_smbus_mode(const struct device *dev, enum i2c_stm32_mode mode)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	data->mode = mode;

	switch (mode) {
	case I2CSTM32MODE_I2C:
		LL_I2C_SetMode(i2c, LL_I2C_MODE_I2C);
		return;
#ifdef CONFIG_SMBUS_STM32
	case I2CSTM32MODE_SMBUSHOST:
		LL_I2C_SetMode(i2c, LL_I2C_MODE_SMBUS_HOST);
		return;
	case I2CSTM32MODE_SMBUSDEVICE:
		LL_I2C_SetMode(i2c, LL_I2C_MODE_SMBUS_DEVICE);
		return;
	case I2CSTM32MODE_SMBUSDEVICEARP:
		LL_I2C_SetMode(i2c, LL_I2C_MODE_SMBUS_DEVICE_ARP);
		return;
#endif
	default:
		LOG_ERR("%s: invalid mode %i", dev->name, mode);
		return;
	}
}
#endif

#ifdef CONFIG_SMBUS_STM32
void i2c_stm32_smbalert_enable(const struct device *dev)
{
	struct i2c_stm32_data *data = dev->data;
	const struct i2c_stm32_config *cfg = dev->config;

	data->smbalert_active = true;
	LL_I2C_EnableSMBusAlert(cfg->i2c);
	LL_I2C_EnableIT_ERR(cfg->i2c);
	LL_I2C_Enable(cfg->i2c);
}

void i2c_stm32_smbalert_disable(const struct device *dev)
{
	struct i2c_stm32_data *data = dev->data;
	const struct i2c_stm32_config *cfg = dev->config;

	data->smbalert_active = false;
	LL_I2C_DisableSMBusAlert(cfg->i2c);
	LL_I2C_DisableIT_ERR(cfg->i2c);
	LL_I2C_Disable(cfg->i2c);
}
#endif /* CONFIG_SMBUS_STM32 */

/* Macros for I2C instance declaration */

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
		.dma_slot = STM32_DMA_SLOT(index, dir, slot),					\
		.channel_direction = STM32_DMA_CONFIG_DIRECTION(				\
					STM32_DMA_CHANNEL_CONFIG(index, dir)),			\
		.cyclic =  STM32_DMA_CONFIG_CYCLIC(						\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),				\
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(					\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),				\
		.source_data_size = STM32_DMA_CONFIG_##src##_DATA_SIZE(				\
					STM32_DMA_CHANNEL_CONFIG(index, dir)),			\
		.dest_data_size = STM32_DMA_CONFIG_##dest##_DATA_SIZE(				\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),				\
		.source_burst_length = 1,							\
		.dest_burst_length = 1,								\
		.dma_callback = i2c_stm32_dma_##dir##_cb,					\
	},))

#else

#define I2C_DMA_INIT(index, dir)
#define I2C_DMA_DATA_INIT(index, dir, src, dest)

#endif /* CONFIG_I2C_STM32_V2_DMA */

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
	IF_ENABLED(CONFIG_I2C_STM32_BUS_RECOVERY,						\
		(.scl =	GPIO_DT_SPEC_INST_GET_OR(index, scl_gpios, {0}),			\
		 .sda = GPIO_DT_SPEC_INST_GET_OR(index, sda_gpios, {0}),))			\
	IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2),					\
		(.timings = (const struct i2c_config_timing *) i2c_timings_##index,		\
		 .n_timings =									\
			sizeof(i2c_timings_##index) / (sizeof(struct i2c_config_timing)),))	\
	I2C_DMA_INIT(index, tx)									\
	I2C_DMA_INIT(index, rx)									\
};												\
												\
static struct i2c_stm32_data i2c_stm32_dev_data_##index = {					\
	I2C_DMA_DATA_INIT(index, tx, MEMORY, PERIPHERAL)					\
	I2C_DMA_DATA_INIT(index, rx, PERIPHERAL, MEMORY)					\
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
