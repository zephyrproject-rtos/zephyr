/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
#include <stm32_ll_i2c.h>
#include <stm32_ll_rcc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ll_stm32_v2_rtio);

#include "i2c_ll_stm32.h"
#include "i2c-priv.h"

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

void i2c_stm32_event(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	I2C_TypeDef *i2c = cfg->i2c;
	int ret = 0;

	if (data->xfer_len != 0) {
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
		if ((data->xfer_flags & I2C_MSG_STOP) != 0) {
			LL_I2C_GenerateStopCondition(i2c);
		} else {
			i2c_stm32_disable_transfer_interrupts(dev);
		}

		if ((data->xfer_len == 0) && i2c_rtio_complete(ctx, ret)) {
			i2c_stm32_start(dev);
		}
	}
}

int i2c_stm32_error(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	I2C_TypeDef *i2c = cfg->i2c;
	int ret = 0;

	if (LL_I2C_IsActiveFlag_ARLO(i2c)) {
		LL_I2C_ClearFlag_ARLO(i2c);
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

int i2c_stm32_msg_start(const struct device *dev, uint8_t flags,
	uint8_t *buf, size_t buf_len, uint16_t i2c_addr)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t transfer;

	data->xfer_buf = buf;
	data->xfer_len = buf_len;
	data->xfer_flags = flags;

	if ((flags & I2C_MSG_READ) != 0) {
		transfer = LL_I2C_REQUEST_READ;
	} else {
		transfer = LL_I2C_REQUEST_WRITE;
	}

	/* TODO deal with larger than 255 byte transfers correctly */
	if (buf_len > UINT8_MAX) {
		/* TODO LL_I2C_EnableReloadMode(i2c); */
		return -EINVAL;
	}

	if ((I2C_MSG_ADDR_10_BITS & flags) != 0) {
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
	if ((flags & I2C_MSG_READ) != 0) {
		LL_I2C_EnableIT_RX(i2c);
	} else {
		LL_I2C_EnableIT_TX(i2c);
	}

	return 0;
}

int i2c_stm32_configure_timing(const struct device *dev, uint32_t clock)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
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
