/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of LTC4286 sensor
 * @ingroup ltc4286_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_LTC4286_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_LTC4286_H_

/**
 * @brief Analog Devices LTC4286 Hot Swap controller and power monitor
 * @defgroup ltc4286_interface LTC4286
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/drivers/sensor.h>

/**
 * @name Status Word bit flags
 * @{
 */
/** Output voltage fault or warning */
#define LTC4286_STATUS_WORD_VOUT         BIT(15)
/** Output current fault or warning */
#define LTC4286_STATUS_WORD_IOUT         BIT(14)
/** Input voltage fault or warning */
#define LTC4286_STATUS_WORD_INPUT        BIT(13)
/** Manufacturer-specific fault or warning */
#define LTC4286_STATUS_WORD_MFRSPEC      BIT(12)
/** FET undervoltage threshold fault */
#define LTC4286_STATUS_WORD_FET_UV_THRES BIT(11)
/** Other fault or warning */
#define LTC4286_STATUS_WORD_OTHER        BIT(9)
/** Unknown fault or warning */
#define LTC4286_STATUS_WORD_UNKNOWN      BIT(8)
/** Device is busy processing command */
#define LTC4286_STATUS_WORD_BUSY         BIT(7)
/** Device is in off state */
#define LTC4286_STATUS_WORD_OFF          BIT(6)
/** Output current overcurrent threshold fault */
#define LTC4286_STATUS_WORD_IOUT_OTHRES  BIT(4)
/** Input voltage undervoltage threshold fault */
#define LTC4286_STATUS_WORD_VIN_UTHRES   BIT(3)
/** Temperature fault or warning */
#define LTC4286_STATUS_WORD_TEMP         BIT(2)
/** Communication fault */
#define LTC4286_STATUS_WORD_COMLINE      BIT(1)
/** Not applicable */
#define LTC4286_STATUS_WORD_NA           BIT(0)
/** @} */

/**
 * @brief Custom sensor channels for LTC4286
 */
enum ltc4286_sensor_chan {
	/** Input voltage (VIN) channel */
	SENSOR_CHAN_LTC4286_VIN = SENSOR_CHAN_PRIV_START,
	/** Device status word channel */
	SENSOR_CHAN_LTC4286_STATUS,
	/** Power good status channel */
	SENSOR_CHAN_LTC4286_POWER_GOOD,
	/** Comparator output channel */
	SENSOR_CHAN_LTC4286_COMPARATOR,
	/** Alert status channel */
	SENSOR_CHAN_LTC4286_ALERT,
};

/**
 * @brief Alert mask types for triggering alerts
 */
enum ltc4286_trig_alert_mask {
	/** Status byte low mask (0xD0) */
	LTC4286_ALERT_STATUS_BYTE_LOW_MASK = 0xD0,
	/** Status byte high mask */
	LTC4286_ALERT_STATUS_BYTE_HIGH_MASK,
	/** Output voltage status mask */
	LTC4286_ALERT_STATUS_VOUT_MASK,
	/** Output current status mask */
	LTC4286_ALERT_STATUS_IOUT_MASK,
	/** Input voltage/power status mask */
	LTC4286_ALERT_STATUS_INPUT_MASK,
	/** Temperature status mask */
	LTC4286_ALERT_STATUS_TEMP_MASK,
	/** Communication status mask */
	LTC4286_ALERT_STATUS_COMMS_MASK,
	/** Manufacturer-specific status mask (0xD8) */
	LTC4286_ALERT_STATUS_SPEC_MASK = 0xD8,
	/** System status 1 mask */
	LTC4286_ALERT_STATUS_SYS1_MASK,
	/** System status 2 mask */
	LTC4286_ALERT_STATUS_SYS2_MASK,
};

/**
 * @brief Enable alert mask for LTC4286
 *
 * Enables the specified alert mask to trigger alerts when the corresponding
 * status condition occurs.
 *
 * @param dev Pointer to the LTC4286 device structure
 * @param alert Alert mask type to enable
 *
 * @retval 0 on success
 * @retval -errno negative errno code on failure
 */
int ltc4286_enable_alert_mask(const struct device *dev, enum ltc4286_trig_alert_mask alert);

/**
 * @brief Disable alert mask for LTC4286
 *
 * Disables the specified alert mask to prevent alerts from being triggered
 * when the corresponding status condition occurs.
 *
 * @param dev Pointer to the LTC4286 device structure
 * @param alert Alert mask type to disable
 *
 * @retval 0 on success
 * @retval -errno negative errno code on failure
 */
int ltc4286_disable_alert_mask(const struct device *dev, enum ltc4286_trig_alert_mask alert);

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_SENSOR_LTC4286_LTC4286_H_ */
