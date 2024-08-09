/*
 * Copyright (c) 2024, Croxel Inc
 * Copyright (c) 2024, Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2C_I2C_NRFX_TWIM_COMMON_H_
#define ZEPHYR_DRIVERS_I2C_I2C_NRFX_TWIM_COMMON_H_

#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <nrfx_twim.h>

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_NRFX_TWIM_INVALID_FREQUENCY ((nrf_twim_frequency_t)-1)
#define I2C_NRFX_TWIM_FREQUENCY(bitrate)                                                           \
	(bitrate == I2C_BITRATE_STANDARD ? NRF_TWIM_FREQ_100K                                      \
	 : bitrate == 250000             ? NRF_TWIM_FREQ_250K                                      \
	 : bitrate == I2C_BITRATE_FAST                                                             \
		 ? NRF_TWIM_FREQ_400K                                                              \
		 : IF_ENABLED(NRF_TWIM_HAS_1000_KHZ_FREQ,                                          \
			      (bitrate == I2C_BITRATE_FAST_PLUS ? NRF_TWIM_FREQ_1000K :))          \
			   I2C_NRFX_TWIM_INVALID_FREQUENCY)

#define I2C(idx)                DT_NODELABEL(i2c##idx)
#define I2C_HAS_PROP(idx, prop) DT_NODE_HAS_PROP(I2C(idx), prop)
#define I2C_FREQUENCY(idx)      I2C_NRFX_TWIM_FREQUENCY(DT_PROP(I2C(idx), clock_frequency))

struct i2c_nrfx_twim_common_config {
	nrfx_twim_t twim;
	nrfx_twim_config_t twim_config;
	nrfx_twim_evt_handler_t event_handler;
	uint16_t msg_buf_size;
	void (*irq_connect)(void);
	const struct pinctrl_dev_config *pcfg;
	uint8_t *msg_buf;
	uint16_t max_transfer_size;
};

int i2c_nrfx_twim_common_init(const struct device *dev);
int i2c_nrfx_twim_configure(const struct device *dev, uint32_t i2c_config);
int i2c_nrfx_twim_recover_bus(const struct device *dev);
int i2c_nrfx_twim_msg_transfer(const struct device *dev, uint8_t flags, uint8_t *buf,
			       size_t buf_len, uint16_t i2c_addr);

#ifdef CONFIG_PM_DEVICE
int twim_nrfx_pm_action(const struct device *dev, enum pm_device_action action);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2C_I2C_NRFX_TWIM_COMMON_H_ */
