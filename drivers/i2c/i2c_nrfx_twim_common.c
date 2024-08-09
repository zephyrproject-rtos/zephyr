/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device_runtime.h>

#include "i2c_nrfx_twim_common.h"

LOG_MODULE_DECLARE(i2c_nrfx_twim);

int i2c_nrfx_twim_recover_bus(const struct device *dev)
{
	const struct i2c_nrfx_twim_common_config *config = dev->config;
	enum pm_device_state state;
	uint32_t scl_pin;
	uint32_t sda_pin;
	nrfx_err_t err;

	scl_pin = nrf_twim_scl_pin_get(config->twim.p_twim);
	sda_pin = nrf_twim_sda_pin_get(config->twim.p_twim);

	/* disable peripheral if active (required to release SCL/SDA lines) */
	(void)pm_device_state_get(dev, &state);
	if (state == PM_DEVICE_STATE_ACTIVE) {
		nrfx_twim_disable(&config->twim);
	}

	err = nrfx_twim_bus_recover(scl_pin, sda_pin);

	/* restore peripheral if it was active before */
	if (state == PM_DEVICE_STATE_ACTIVE) {
		(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		nrfx_twim_enable(&config->twim);
	}

	return (err == NRFX_SUCCESS ? 0 : -EBUSY);
}

int i2c_nrfx_twim_configure(const struct device *dev, uint32_t i2c_config)
{
	const struct i2c_nrfx_twim_common_config *config = dev->config;

	if (I2C_ADDR_10_BITS & i2c_config) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(i2c_config)) {
	case I2C_SPEED_STANDARD:
		nrf_twim_frequency_set(config->twim.p_twim, NRF_TWIM_FREQ_100K);
		break;
	case I2C_SPEED_FAST:
		nrf_twim_frequency_set(config->twim.p_twim, NRF_TWIM_FREQ_400K);
		break;
#if NRF_TWIM_HAS_1000_KHZ_FREQ
	case I2C_SPEED_FAST_PLUS:
		nrf_twim_frequency_set(config->twim.p_twim, NRF_TWIM_FREQ_1000K);
		break;
#endif
	default:
		LOG_ERR("unsupported speed");
		return -EINVAL;
	}

	return 0;
}

int i2c_nrfx_twim_msg_transfer(const struct device *dev, uint8_t flags, uint8_t *buf,
			       size_t buf_len, uint16_t i2c_addr)
{
	const struct i2c_nrfx_twim_common_config *config = dev->config;
	nrfx_twim_xfer_desc_t cur_xfer = {
		.address = i2c_addr,
		.type = (flags & I2C_MSG_READ) ? NRFX_TWIM_XFER_RX : NRFX_TWIM_XFER_TX,
		.p_primary_buf = buf,
		.primary_length = buf_len,
	};
	nrfx_err_t res;
	int ret = 0;

	if (buf_len > config->max_transfer_size) {
		LOG_ERR("Trying to transfer more than the maximum size "
			"for this device: %d > %d",
			buf_len, config->max_transfer_size);
		return -ENOSPC;
	}

	res = nrfx_twim_xfer(&config->twim, &cur_xfer,
			     (flags & I2C_MSG_STOP) ? 0 : NRFX_TWIM_FLAG_TX_NO_STOP);
	if (res != NRFX_SUCCESS) {
		if (res == NRFX_ERROR_BUSY) {
			ret = -EBUSY;
		} else {
			ret = -EIO;
		}
	}
	return ret;
}

int twim_nrfx_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct i2c_nrfx_twim_common_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		nrfx_twim_enable(&config->twim);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		nrfx_twim_disable(&config->twim);
		(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

int i2c_nrfx_twim_common_init(const struct device *dev)
{
	const struct i2c_nrfx_twim_common_config *config = dev->config;

	config->irq_connect();

	(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);

	if (nrfx_twim_init(&config->twim, &config->twim_config, config->event_handler, data) !=
	    NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s", dev->name);
		return -EIO;
	}

	return pm_device_driver_init(dev, twim_nrfx_pm_action);
}
