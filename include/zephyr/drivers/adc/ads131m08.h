/*
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended API for ADS131M08 ADC calibration
 *
 * This header provides per-channel gain and offset calibration functions
 * for the TI ADS131M08 8-channel delta-sigma ADC.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_ADC_ADS131M08_H_
#define ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_ADC_ADS131M08_H_

#include <zephyr/device.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADS131M08 Extended API
 * @defgroup ads131m08_interface ADS131M08
 * @ingroup adc_interface_ext
 * @{
 */

/** Number of ADC channels on the ADS131M08 */
#define ADS131M08_NUM_CHANNELS 8

/**
 * @brief Channel input MUX selection
 */
enum ads131m08_mux {
	ADS131M08_MUX_NORMAL = 0,      /**< AINxP and AINxN (default) */
	ADS131M08_MUX_SHORTED = 1,     /**< ADC inputs shorted */
	ADS131M08_MUX_POS_DC_TEST = 2, /**< Positive DC test signal */
	ADS131M08_MUX_NEG_DC_TEST = 3, /**< Negative DC test signal */
};

/**
 * @brief Per-channel calibration and configuration data
 *
 *
 * Calibrated output = (ADC_OUT - offset) * (gain / 0x800000)
 */
struct ads131m08_channel_cal {
	int32_t offset;         /**< 24-bit signed offset calibration value */
	uint32_t gain;          /**< 24-bit unsigned gain calibration value */
	int16_t phase;          /**< 10-bit signed phase delay in modulator clock cycles */
	uint8_t dcblk_dis;      /**< DC block filter disable: 0=controlled by DCBLOCK, 1=disabled */
	enum ads131m08_mux mux; /**< Channel input MUX selection */
};

/**
 * @brief Calibration data for all 8 channels
 */
struct ads131m08_calibration {
	struct ads131m08_channel_cal
		ch[ADS131M08_NUM_CHANNELS]; /**<  Per-channel calibration data */
};

/**
 * @brief Set calibration values for all 8 channels
 *
 * Writes offset and gain calibration registers for all channels in one
 * SPI transaction sequence.
 *
 * @param dev Pointer to the ADS131M08 device instance
 * @param cal Pointer to calibration data for all 8 channels
 *
 * @retval 0 on success
 * @retval error code else
 */
int ads131m08_set_calibration(const struct device *dev, const struct ads131m08_calibration *cal);

/**
 * @brief Get calibration values for all 8 channels
 *
 * Reads offset and gain calibration registers for all channels.
 *
 * @param dev Pointer to the ADS131M08 device instance
 * @param cal Pointer to store calibration data for all 8 channels
 *
 * @retval 0 on success
 * @retval error code else
 */
int ads131m08_get_calibration(const struct device *dev, struct ads131m08_calibration *cal);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_ADC_ADS131M08_H_ */
