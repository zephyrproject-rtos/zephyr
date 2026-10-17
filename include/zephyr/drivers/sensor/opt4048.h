/*
 * Copyright (c) 2026 Darshan Maru <darshanmaru1411@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public header for TI OPT4048 XYZ tristimulus color sensor.
 * @ingroup sensor_interface
 *
 * Defines the sensor channels, attributes and value enumerations specific
 * to the OPT4048 for use with the generic Zephyr sensor API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_OPT4048_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_OPT4048_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name OPT4048 status register flags
 *
 * Bit masks of the flags returned by opt4048_get_flags().
 * @{
 */
/** Measurement below the low threshold. */
#define OPT4048_FLAG_L                0x01
/** Measurement above the high threshold. */
#define OPT4048_FLAG_H                0x02
/** Conversion completed. */
#define OPT4048_FLAG_CONVERSION_READY 0x04
/** Signal exceeded the full-scale range. */
#define OPT4048_FLAG_OVERLOAD         0x08
/** @} */

/**
 * @brief Full-scale light range of the OPT4048.
 *
 * Use with SENSOR_ATTR_OPT4048_RANGE. A fixed range trades measurable
 * illuminance against resolution; OPT4048_RANGE_AUTO lets the device pick
 * the range for every conversion.
 */
enum opt4048_range {
	/** 2.2 klux full-scale range. */
	OPT4048_RANGE_2K_LUX = 0,

	/** 4.5 klux full-scale range. */
	OPT4048_RANGE_4K_LUX = 1,

	/** 9 klux full-scale range. */
	OPT4048_RANGE_9K_LUX = 2,

	/** 18 klux full-scale range. */
	OPT4048_RANGE_18K_LUX = 3,

	/** 36 klux full-scale range. */
	OPT4048_RANGE_36K_LUX = 4,

	/** 72 klux full-scale range. */
	OPT4048_RANGE_72K_LUX = 5,

	/** 144 klux full-scale range. */
	OPT4048_RANGE_144K_LUX = 6,

	/** Automatic range selection. */
	OPT4048_RANGE_AUTO = 12,
};

/**
 * @brief Conversion time per channel of the OPT4048.
 *
 * Use with SENSOR_ATTR_OPT4048_CONVERSION_TIME.
 *
 * @note A full measurement cycle converts all four channels, so it takes
 * four times the selected conversion time.
 */
enum opt4048_conversion_time {
	/** 600 microseconds. */
	OPT4048_CONVERSION_TIME_600US = 0,

	/** 1 millisecond. */
	OPT4048_CONVERSION_TIME_1MS = 1,

	/** 1.8 milliseconds. */
	OPT4048_CONVERSION_TIME_1_8MS = 2,

	/** 3.4 milliseconds. */
	OPT4048_CONVERSION_TIME_3_4MS = 3,

	/** 6.5 milliseconds. */
	OPT4048_CONVERSION_TIME_6_5MS = 4,

	/** 12.7 milliseconds. */
	OPT4048_CONVERSION_TIME_12_7MS = 5,

	/** 25 milliseconds. */
	OPT4048_CONVERSION_TIME_25MS = 6,

	/** 50 milliseconds. */
	OPT4048_CONVERSION_TIME_50MS = 7,

	/** 100 milliseconds. */
	OPT4048_CONVERSION_TIME_100MS = 8,

	/** 200 milliseconds. */
	OPT4048_CONVERSION_TIME_200MS = 9,

	/** 400 milliseconds. */
	OPT4048_CONVERSION_TIME_400MS = 10,

	/** 800 milliseconds. */
	OPT4048_CONVERSION_TIME_800MS = 11,
};

/**
 * @brief Operating mode of the OPT4048.
 *
 * Use with SENSOR_ATTR_OPT4048_MODE.
 *
 * @note In the one-shot modes the driver starts a conversion on every
 * sensor_sample_fetch() call and waits for it to finish. In continuous
 * mode the latest result registers are read.
 */
enum opt4048_mode {
	/** Power-down, no measurements. */
	OPT4048_MODE_POWERDOWN = 0,

	/** Forced auto-range one-shot. */
	OPT4048_MODE_AUTO_ONESHOT = 1,

	/** One-shot. */
	OPT4048_MODE_ONESHOT = 2,

	/** Continuous. */
	OPT4048_MODE_CONTINUOUS = 3,
};

/**
 * @brief Number of consecutive faults required to set the threshold flags.
 *
 * Use with SENSOR_ATTR_OPT4048_FAULT_COUNT.
 */
enum opt4048_fault_count {
	/** 1 fault. */
	OPT4048_FAULT_COUNT_1 = 0,

	/** 2 consecutive faults. */
	OPT4048_FAULT_COUNT_2 = 1,

	/** 4 consecutive faults. */
	OPT4048_FAULT_COUNT_4 = 2,

	/** 8 consecutive faults. */
	OPT4048_FAULT_COUNT_8 = 3,
};

/**
 * @brief Function of the INT pin of the OPT4048.
 *
 * Use with SENSOR_ATTR_OPT4048_INT_CONFIG.
 */
enum opt4048_int_cfg {
	/** Threshold interrupt / SMBus alert. */
	OPT4048_INT_CFG_SMBUS_ALERT = 0,

	/** Pulse after every channel conversion. */
	OPT4048_INT_CFG_DATA_READY_NEXT = 1,

	/** Pulse after all four channels were converted. */
	OPT4048_INT_CFG_DATA_READY_ALL = 3,
};

/**
 * @brief Driver-specific sensor channel identifiers for the OPT4048.
 *
 * Exposes the raw ADC codes of the four device channels and values derived
 * from them (CIE tristimulus, chromaticity and CCT). SENSOR_CHAN_LIGHT
 * returns the illuminance in lux.
 *
 * @note Every channel is returned by sensor_channel_get() as a struct
 * sensor_value (val1 integer part, val2 in 1e-6 units). The derived channels
 * return -ENODATA when there is no light and -ERANGE when the result is
 * outside the valid range.
 *
 * Use these identifiers with sensor_channel_get() after a successful
 * sensor_sample_fetch() call.
 */
enum sensor_channel_opt4048 {
	/** CH0 (X filter, peak 600 nm), linearised ADC codes. */
	SENSOR_CHAN_OPT4048_RAW_CH0 = SENSOR_CHAN_PRIV_START,

	/** CH1 (Y filter, peak 550 nm), linearised ADC codes. */
	SENSOR_CHAN_OPT4048_RAW_CH1,

	/** CH2 (Z filter, peak 460 nm), linearised ADC codes. */
	SENSOR_CHAN_OPT4048_RAW_CH2,

	/** CH3 (W wide band channel), linearised ADC codes. */
	SENSOR_CHAN_OPT4048_RAW_CH3,

	/** CIE tristimulus X, computed with the conversion matrix. */
	SENSOR_CHAN_OPT4048_X,

	/** CIE tristimulus Y, computed with the conversion matrix. */
	SENSOR_CHAN_OPT4048_Y,

	/** CIE tristimulus Z, computed with the conversion matrix. */
	SENSOR_CHAN_OPT4048_Z,

	/** CIE 1931 chromaticity coordinate x (0 to 1). */
	SENSOR_CHAN_OPT4048_CIE_X,

	/** CIE 1931 chromaticity coordinate y (0 to 1). */
	SENSOR_CHAN_OPT4048_CIE_Y,

	/** Correlated color temperature (CCT), in Kelvin. */
	SENSOR_CHAN_OPT4048_CCT,
};

/**
 * @brief Driver-specific sensor attribute identifiers for the OPT4048.
 *
 * The attribute value is passed in sensor_value.val1.
 *
 * @note THRESHOLDS: SENSOR_ATTR_LOWER_THRESH and SENSOR_ATTR_UPPER_THRESH
 * take linear ADC codes, the same unit as SENSOR_CHAN_OPT4048_RAW_CHx. The
 * channel they are compared against is selected with
 * SENSOR_ATTR_OPT4048_THRESHOLD_CHANNEL.
 *
 * Use these identifiers with sensor_attr_set() and sensor_attr_get().
 */
enum sensor_attribute_opt4048 {
	/** Full-scale range, see @ref opt4048_range. */
	SENSOR_ATTR_OPT4048_RANGE = SENSOR_ATTR_PRIV_START,

	/** Conversion time per channel, see @ref opt4048_conversion_time. */
	SENSOR_ATTR_OPT4048_CONVERSION_TIME,

	/** Operating mode, see @ref opt4048_mode. */
	SENSOR_ATTR_OPT4048_MODE,

	/** Consecutive fault count, see @ref opt4048_fault_count. */
	SENSOR_ATTR_OPT4048_FAULT_COUNT,

	/** Interrupt latch: 1 = latched window mode, 0 = transparent hysteresis. */
	SENSOR_ATTR_OPT4048_INT_LATCH,

	/** INT pin polarity: 0 = active low, 1 = active high. */
	SENSOR_ATTR_OPT4048_INT_POLARITY,

	/** INT pin direction: 1 = output, 0 = input (hardware one-shot trigger). */
	SENSOR_ATTR_OPT4048_INT_DIRECTION,

	/** INT pin function, see @ref opt4048_int_cfg. */
	SENSOR_ATTR_OPT4048_INT_CONFIG,

	/** Channel (0 to 3) used by the threshold logic. */
	SENSOR_ATTR_OPT4048_THRESHOLD_CHANNEL,

	/** Quick wake-up from standby in one-shot modes: 1 = enabled. */
	SENSOR_ATTR_OPT4048_QUICK_WAKE,
};

/**
 * @brief Read the status flags of the OPT4048.
 *
 * Reads the STATUS register of the device and returns the flag bits as a
 * bitmask of OPT4048_FLAG_L, OPT4048_FLAG_H, OPT4048_FLAG_CONVERSION_READY
 * and OPT4048_FLAG_OVERLOAD.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[out] flags Pointer to store the status flags bitmask.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p flags is NULL.
 * @retval -errno Negative error code from the I2C API.
 */
int opt4048_get_flags(const struct device *dev, uint8_t *flags);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_OPT4048_H_ */
