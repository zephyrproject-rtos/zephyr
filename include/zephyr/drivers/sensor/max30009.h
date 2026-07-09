/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file max30009.h
 * @brief Header file for extended sensor API of MAX30009 sensor
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30009_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30009_H_

#include <zephyr/drivers/sensor.h>

/**
 * @brief MAX30009-specific sensor channels
 *
 * These channels extend the standard sensor_channel enum
 * to expose the MAX30009 BioZ in-phase and quadrature data.
 */
enum max30009_sensor_channel {
	/** BioZ in-phase (I) channel */
	SENSOR_CHAN_BIOZ_I = SENSOR_CHAN_PRIV_START,
	/** BioZ quadrature (Q) channel */
	SENSOR_CHAN_BIOZ_Q,
};

/**
 * @brief MAX30009-specific sensor triggers
 *
 * These triggers extend the standard sensor_trigger_type enum
 * to support the MAX30009 DC lead-off and range status events.
 */
enum sensor_trigger_max30009 {
	/** DC lead-off: BIOZN (BIN) voltage is below the low threshold */
	SENSOR_TRIG_MAX30009_DC_LOFF_NL = SENSOR_TRIG_PRIV_START,
	/** DC lead-off: BIOZN (BIN) voltage is above the high threshold */
	SENSOR_TRIG_MAX30009_DC_LOFF_NH,
	/** DC lead-off: BIOZP (BIP) voltage is below the low threshold */
	SENSOR_TRIG_MAX30009_DC_LOFF_PL,
	/** DC lead-off: BIOZP (BIP) voltage is above the high threshold */
	SENSOR_TRIG_MAX30009_DC_LOFF_PH,
	/** BioZ current generator out of range (DRVN peaks out of compliance, leads off) */
	SENSOR_TRIG_MAX30009_DRV_OOR,
	/** BioZ under range: absolute BioZ ADC reading is below the BioZ low threshold */
	SENSOR_TRIG_MAX30009_BIOZ_UNDR,
	/** BioZ over range: absolute BioZ ADC reading exceeds the BioZ high threshold */
	SENSOR_TRIG_MAX30009_BIOZ_OVER,
	/** DC leads-on detected: a BioZ lead-on condition is present */
	SENSOR_TRIG_MAX30009_LON,
};

/**
 * @brief MAX30009-specific sensor attributes
 *
 * These attributes extend the standard sensor_attribute enum
 * to support MAX30009-specific BioZ configuration options.
 */
enum sensor_attribute_max30009 {
	/** BioZ transmit drive mode (BIOZ_DRV_MODE) */
	SENSOR_ATTR_MAX30009_DRV_MODE = SENSOR_ATTR_PRIV_START,
	/** BioZ internal current-range resistor (BIOZ_IDRV_RGE) */
	SENSOR_ATTR_MAX30009_IDRV_RGE,
	/** BioZ voltage-drive magnitude (BIOZ_VDRV_MAG) */
	SENSOR_ATTR_MAX30009_VDRV_MAG,
	/** BioZ receive-channel gain (BIOZ_GAIN) */
	SENSOR_ATTR_MAX30009_GAIN,
	/** BioZ analog high-pass filter selection (BIOZ_AHPF) */
	SENSOR_ATTR_MAX30009_AHPF,
	/** BioZ digital high-pass filter cutoff (BIOZ_DHPF) */
	SENSOR_ATTR_MAX30009_DHPF,
	/** BioZ digital low-pass filter cutoff (BIOZ_DLPF) */
	SENSOR_ATTR_MAX30009_DLPF,
	/** BioZ amplifier range (BIOZ_AMP_RGE) */
	SENSOR_ATTR_MAX30009_AMP_RGE,
	/** BioZ amplifier bandwidth (BIOZ_AMP_BW) */
	SENSOR_ATTR_MAX30009_AMP_BW,
	/** BioZ threshold comparator channel (BIOZ_CMP) */
	SENSOR_ATTR_MAX30009_CMP,
	/** BioZ ADC oversampling ratio, natural units 8..1024 (BIOZ_ADC_OSR) */
	SENSOR_ATTR_MAX30009_ADC_OSR,
	/** BioZ DAC oversampling ratio, natural units 32..256 (BIOZ_DAC_OSR) */
	SENSOR_ATTR_MAX30009_DAC_OSR,
	/** BioZ high threshold (BIOZ_HIGH_THRESHOLD) */
	SENSOR_ATTR_MAX30009_HIGH_THRESHOLD,
	/** BioZ low threshold (BIOZ_LOW_THRESHOLD) */
	SENSOR_ATTR_MAX30009_LOW_THRESHOLD,
	/** Enable BioZ in-phase (I) channel (BIOZ_I_EN) */
	SENSOR_ATTR_MAX30009_I_EN,
	/** Enable BioZ quadrature (Q) channel (BIOZ_Q_EN) */
	SENSOR_ATTR_MAX30009_Q_EN,
	/** Enable BioZ bandgap bias (BIOZ_BG_EN) */
	SENSOR_ATTR_MAX30009_BG_EN,
};
#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30009_H_ */
