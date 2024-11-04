/*
 * Copyright (c) 2024, Croxel Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2C_I2C_NRFX_TWI_COMMON_H_
#define ZEPHYR_DRIVERS_I2C_I2C_NRFX_TWI_COMMON_H_

#include <zephyr/pm/device.h>
#include <nrfx_twi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_NRFX_TWI_INVALID_FREQUENCY  ((nrf_twi_frequency_t)-1)
#define I2C_NRFX_TWI_FREQUENCY(bitrate)					       \
	 (bitrate == I2C_BITRATE_STANDARD ? NRF_TWI_FREQ_100K		       \
	: bitrate == 250000               ? NRF_TWI_FREQ_250K		       \
	: bitrate == I2C_BITRATE_FAST     ? NRF_TWI_FREQ_400K		       \
					  : I2C_NRFX_TWI_INVALID_FREQUENCY)
#define I2C(idx) DT_NODELABEL(i2c##idx)
#define I2C_FREQUENCY(idx)						       \
	I2C_NRFX_TWI_FREQUENCY(DT_PROP_OR(I2C(idx), clock_frequency,	       \
					  I2C_BITRATE_STANDARD))

struct i2c_nrfx_twi_common_data {
	uint32_t dev_config;
};

struct i2c_nrfx_twi_config {
	nrfx_twi_t twi;
	nrfx_twi_config_t config;
	nrfx_twi_evt_handler_t event_handler;
	const struct pinctrl_dev_config *pcfg;
};

static inline nrfx_err_t i2c_nrfx_twi_get_evt_result(nrfx_twi_evt_t const *p_event)
{
	switch (p_event->type) {
	case NRFX_TWI_EVT_DONE:
		return NRFX_SUCCESS;
	case NRFX_TWI_EVT_ADDRESS_NACK:
		return NRFX_ERROR_DRV_TWI_ERR_ANACK;
	case NRFX_TWI_EVT_DATA_NACK:
		return NRFX_ERROR_DRV_TWI_ERR_DNACK;
	default:
		return NRFX_ERROR_INTERNAL;
	}
}

int i2c_nrfx_twi_init(const struct device *dev);
int i2c_nrfx_twi_configure(const struct device *dev, uint32_t dev_config);
int i2c_nrfx_twi_recover_bus(const struct device *dev);
int i2c_nrfx_twi_msg_transfer(const struct device *dev, uint8_t flags,
			      uint8_t *buf, size_t buf_len,
			      uint16_t i2c_addr, bool more_msgs);

#ifdef CONFIG_PM_DEVICE
int twi_nrfx_pm_action(const struct device *dev, enum pm_device_action action);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2C_I2C_NRFX_TWI_COMMON_H_ */
