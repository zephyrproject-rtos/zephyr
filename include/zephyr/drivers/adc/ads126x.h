/*
 * Copyright 2026 Sacra Systems Private Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_ADS126X_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_ADS126X_H_

/**
 * @file
 * @brief Texas Instruments ADS126X ADC driver API
 *
 * This file contains the API for the ADS126X 32-bit, 10-channel, low-power,
 * Delta-Sigma ADC with integrated PGA, VREF, SPI interface, and two IDACs.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADS126X ADC driver API
 * @defgroup ads126x_interface ADS126X ADC driver API
 * @ingroup adc_interface
 * @{
 */

/**
 * @brief ADS126X analog multiplexer input selection.
 */
enum ads126x_mux_input {
	/** Analog input AIN0 */
	ADS126X_MUX_AIN0 = 0x00,

	/** Analog input AIN1 */
	ADS126X_MUX_AIN1 = 0x01,

	/** Analog input AIN2 */
	ADS126X_MUX_AIN2 = 0x02,

	/** Analog input AIN3 */
	ADS126X_MUX_AIN3 = 0x03,

	/** Analog input AIN4 */
	ADS126X_MUX_AIN4 = 0x04,

	/** Analog input AIN5 */
	ADS126X_MUX_AIN5 = 0x05,

	/** Analog input AIN6 */
	ADS126X_MUX_AIN6 = 0x06,

	/** Analog input AIN7 */
	ADS126X_MUX_AIN7 = 0x07,

	/** Analog input AIN8 */
	ADS126X_MUX_AIN8 = 0x08,

	/** Analog input AIN9 */
	ADS126X_MUX_AIN9 = 0x09,

	/** Common analog input */
	ADS126X_MUX_AINCOM = 0x0A,

	/** Internal temperature sensor as input */
	ADS126X_MUX_TEMP = 0x0B,

	/** AVDD supply monitor as input */
	ADS126X_MUX_AVDD = 0x0C,

	/** DVDD supply monitor as input */
	ADS126X_MUX_DVDD = 0x0D,

	/** TDAC output as input */
	ADS126X_MUX_TDAC = 0x0E,

	/** Open input selection */
	ADS126X_MUX_OPEN = 0x0F,
};

/**
 * @brief Configure ADS126x ADC inputs at runtime.
 *
 * Single-ended:
 *
 *     input_positive = ADS126X_MUX_AIN3
 *     differential   = false
 *
 *     Measures AIN3 with respect to AINCOM.
 *
 * Differential:
 *
 *     input_positive = ADS126X_MUX_AIN3
 *     input_negative = ADS126X_MUX_AIN4
 *     differential   = true
 *
 *     Measures AIN3 - AIN4.
 *
 * Example:
 *
 *     struct adc_channel_cfg cfg = {
 *         .channel_id = 0,
 *         .gain = ADC_GAIN_1,
 *         .reference = ADC_REF_INTERNAL,
 *         .acquisition_time = ADC_ACQ_TIME_DEFAULT,
 *         .differential = false,
 *         .input_positive = ADS126X_MUX_AIN3,
 *     };
 *
 *     adc_channel_setup(dev, &cfg);
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_ADS126X_H_ */
