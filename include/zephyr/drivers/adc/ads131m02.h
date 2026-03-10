/*
 * Copyright (c) 2024 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended API of ADS131M02 ADC
 * @ingroup ads131m02_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_ADS131M02_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_ADS131M02_H_

#include <zephyr/device.h>

/**
 * @brief Texas Instruments 2 channels SPI ADC
 * @defgroup ads131m02_interface ADS131M02
 * @ingroup adc_interface_ext
 * @{
 */

/**
 * @brief ADS131M02 ADC mode.
 */
enum ads131m02_adc_mode {
	ADS131M02_CONTINUOUS_MODE,	/**< Continuous conversion mode */
	ADS131M02_GLOBAL_CHOP_MODE	/**< Global-chop mode */
};

/**
 * @brief ADS131M02 power mode.
 */
enum ads131m02_adc_power_mode {
	ADS131M02_VLP,	/**< Very Low Power */
	ADS131M02_LP,	/**< Low Power */
	ADS131M02_HR	/**< High Resolution */
};

/**
 * @brief ADS131M02 global-chop delay.
 *
 * Delay inserted after the chopping switches toggle in global-chop mode to allow for external input
 * circuitry settling.
 */
enum ads131m02_gc_delay {
	/** 2 sample delay */
	ADS131M02_GC_DELAY_2,
	/** 4 sample delay */
	ADS131M02_GC_DELAY_4,
	/** 8 sample delay */
	ADS131M02_GC_DELAY_8,
	/** 16 sample delay */
	ADS131M02_GC_DELAY_16,
	/** 32 sample delay */
	ADS131M02_GC_DELAY_32,
	/** 64 sample delay */
	ADS131M02_GC_DELAY_64,
	/** 128 sample delay */
	ADS131M02_GC_DELAY_128,
	/** 256 sample delay */
	ADS131M02_GC_DELAY_256,
	/** 512 sample delay */
	ADS131M02_GC_DELAY_512,
	/** 1024 sample delay */
	ADS131M02_GC_DELAY_1024,
	/** 2048 sample delay */
	ADS131M02_GC_DELAY_2048,
	/** 4096 sample delay */
	ADS131M02_GC_DELAY_4096,
	/** 8192 sample delay */
	ADS131M02_GC_DELAY_8192,
	/** 16384 sample delay */
	ADS131M02_GC_DELAY_16384,
	/** 32768 sample delay */
	ADS131M02_GC_DELAY_32768,
	/** 65536 sample delay */
	ADS131M02_GC_DELAY_65536
};

/**
 * @brief Set the ADC mode of an ADS131M02 ADC.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mode ADC mode to set.
 * @param gc_delay Global chop delay to set (if @p mode is @ref ADS131M02_GLOBAL_CHOP_MODE).
 *
 * @retval 0 success.
 * @retval -errno negative errno on failure.
 */
int ads131m02_set_adc_mode(const struct device *dev, enum ads131m02_adc_mode mode,
			   enum ads131m02_gc_delay gc_delay);

/**
 * @brief Set the power mode of an ADS131M02 ADC.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mode Power mode to set.
 *
 * @retval 0 success.
 * @retval -errno negative errno on failure.
 */
int ads131m02_set_power_mode(const struct device *dev,
			     enum ads131m02_adc_power_mode mode);

/** @} */

#endif
