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

#if defined(LUMOS_XXAA)
#define DIV_ROUND_NEAREST(a, b) (((a) + (b) / 2) / (b))

/* Formula for getting the frequency settings is following:
 * 2^12 * (2^20 / (f_PCLK / desired_frequency)) where f_PCLK is a frequency that
 * drives the TWIM.
 *
 * @param f_pclk Frequency of the clock that drives the peripheral.
 * @param baudrate Desired baudrate.
 *
 * @return Frequency setting to be written to the FREQUENCY register
 */
#define I2C_NRFX_TWIM_GET_CUSTOM_FREQUENCY(f_pclk, frequency) \
	((nrf_twim_frequency_t)((BIT(20) / DIV_ROUND_NEAREST(f_pclk, frequency)) << 12))

#define I2C_NRFX_TWIM_MIN_FREQUENCY KHZ(100)
#define I2C_NRFX_TWIM_MAX_FREQUENCY COND_CODE_1(NRF_TWIM_HAS_1000_KHZ_FREQ, KHZ(1000), KHZ(400))

#define I2C_NRFX_TWIM_CUSTOM_FREQUENCY_VALID_CHECK(frequency) \
	((frequency >= I2C_NRFX_TWIM_MIN_FREQUENCY) && (frequency <= I2C_NRFX_TWIM_MAX_FREQUENCY))

#define I2C_NRFX_TWIM_GET_CUSTOM_FREQUENCY_IF_VALID(f_pclk, frequency)  \
	(I2C_NRFX_TWIM_CUSTOM_FREQUENCY_VALID_CHECK(frequency)          \
		? I2C_NRFX_TWIM_GET_CUSTOM_FREQUENCY(f_pclk, frequency) \
		: I2C_NRFX_TWIM_INVALID_FREQUENCY)
#else
#define I2C_NRFX_TWIM_GET_CUSTOM_FREQUENCY_IF_VALID(f_pclk, frequency) \
	I2C_NRFX_TWIM_INVALID_FREQUENCY
#endif

#define I2C_NRFX_TWIM_FREQUENCY(bitrate, f_pclk)                                                   \
	(bitrate == I2C_BITRATE_STANDARD ? NRF_TWIM_FREQ_100K                                      \
	 : bitrate == 250000             ? NRF_TWIM_FREQ_250K                                      \
	 : bitrate == I2C_BITRATE_FAST                                                             \
		 ? NRF_TWIM_FREQ_400K                                                              \
		 : IF_ENABLED(NRF_TWIM_HAS_1000_KHZ_FREQ,                                          \
			      (bitrate == I2C_BITRATE_FAST_PLUS ? NRF_TWIM_FREQ_1000K :))          \
			I2C_NRFX_TWIM_GET_CUSTOM_FREQUENCY_IF_VALID(f_pclk, bitrate))

#define I2C_FREQUENCY(inst)                                                                   \
	I2C_NRFX_TWIM_FREQUENCY(DT_INST_PROP_OR(inst, clock_frequency, I2C_BITRATE_STANDARD), \
				NRF_PERIPH_GET_FREQUENCY(inst))

/* Macro determines PM actions interrupt safety level.
 *
 * Requesting/releasing TWIM device may be ISR safe, but it cannot be reliably known whether
 * managing its power domain is. It is then assumed that if power domains are used, device is
 * no longer ISR safe. This macro let's us check if we will be requesting/releasing
 * power domains and determines PM device ISR safety value.
 */
#define I2C_PM_ISR_SAFE(inst)								      \
	COND_CODE_1(									      \
		UTIL_AND(								      \
			IS_ENABLED(CONFIG_PM_DEVICE_POWER_DOMAIN),			      \
			UTIL_AND(							      \
				DT_NODE_INST_HAS_PROP(inst, power_domains),		      \
				DT_NODE_HAS_STATUS_OKAY(DT_INST_PHANDLE(inst, power_domains)) \
			)								      \
		),									      \
		(0),									      \
		(PM_DEVICE_ISR_SAFE)							      \
	)

#define CONCAT_BUF_SIZE(inst)						 \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, zephyr_concat_buf_size), \
		    (DT_INST_PROP(inst, zephyr_concat_buf_size)), (0))

#define FLASH_BUF_MAX_SIZE(inst)					    \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, zephyr_flash_buf_max_size), \
		    (DT_INST_PROP(inst, zephyr_flash_buf_max_size)), (0))

#define USES_MSG_BUF(inst)					   \
	COND_CODE_0(CONCAT_BUF_SIZE(inst),			   \
		(COND_CODE_0(FLASH_BUF_MAX_SIZE(inst), (0), (1))), \
		(1))

#define MSG_BUF_SIZE(inst) MAX(CONCAT_BUF_SIZE(inst), FLASH_BUF_MAX_SIZE(inst))

#define MAX_TRANSFER_SIZE(inst) BIT_MASK(DT_INST_PROP(inst, easydma_maxcnt_bits))

struct i2c_nrfx_twim_common_config {
	nrfx_twim_config_t twim_config;
	nrfx_twim_event_handler_t event_handler;
	uint16_t msg_buf_size;
	void (*pre_init)(void);
	const struct pinctrl_dev_config *pcfg;
	uint8_t *msg_buf;
	uint16_t max_transfer_size;
	nrfx_twim_t *twim;
	void *mem_reg;
};

int i2c_nrfx_twim_common_init(const struct device *dev);
int i2c_nrfx_twim_common_deinit(const struct device *dev);
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
