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
#include <zephyr/drivers/i2c/stm32.h>
#include <zephyr/drivers/pinctrl.h>
#include <stm32_ll_i2c.h>

#ifdef CONFIG_I2C_STM32_BUS_RECOVERY
#include "i2c_bitbang.h"
#include "i2c-priv.h"
#endif /* CONFIG_I2C_STM32_BUS_RECOVERY */

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ll_stm32_common);

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

int i2c_stm32_recover_bus(const struct device *dev)
{
	__maybe_unused struct i2c_stm32_data *data = dev->data;
	const struct i2c_stm32_config *config = dev->config;
	struct i2c_bitbang bitbang_ctx;
	struct i2c_bitbang_io bitbang_io = {
		.set_scl = i2c_stm32_bitbang_set_scl,
		.set_sda = i2c_stm32_bitbang_set_sda,
		.get_sda = i2c_stm32_bitbang_get_sda,
	};
	uint32_t bitrate_cfg = i2c_map_dt_bitrate(config->bitrate) | I2C_MODE_CONTROLLER;
	int error = 0;

	LOG_ERR("attempting to recover bus");

	if (config->scl.port == NULL || config->sda.port == NULL) {
		LOG_ERR("No SCL or SDA GPIO deefined");
		return -ENOSYS;
	}

	if (!gpio_is_ready_dt(&config->scl)) {
		LOG_ERR("SCL GPIO device not ready");
		return -EIO;
	}

	if (!gpio_is_ready_dt(&config->sda)) {
		LOG_ERR("SDA GPIO device not ready");
		return -EIO;
	}

#ifndef CONFIG_I2C_RTIO
	k_sem_take(&data->bus_mutex, K_FOREVER);
#endif

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

	/* Re-initialize the I2C peripheral after GPIO-based bus recovery.
	 * pinctrl_apply_state() restores the pin configuration, but the
	 * peripheral registers remain in a faulted state. Re-running
	 * runtime_configure() restores the peripheral to a working state.
	 */
	if (i2c_stm32_runtime_configure(dev, bitrate_cfg) != 0) {
		LOG_ERR("failed to restore I2C peripheral after bus recovery");
	}

#ifndef CONFIG_I2C_RTIO
	k_sem_give(&data->bus_mutex);
#endif

	return error;
}
#endif /* CONFIG_I2C_STM32_BUS_RECOVERY */

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
