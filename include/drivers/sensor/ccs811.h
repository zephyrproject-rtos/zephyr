/*
 * Copyright (c) 2018 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for CCS811 Indoor Air Quality Sensor
 *
 * Some capabilities and operational requirements for this sensor
 * cannot be expressed within the sensor driver abstraction.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_CCS811_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_CCS811_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>
#include <drivers/sensor.h>

/* Status register fields */
#define CCS811_STATUS_ERROR             BIT(0)
#define CCS811_STATUS_DATA_READY        BIT(3)
#define CCS811_STATUS_APP_VALID         BIT(4)
#define CCS811_STATUS_FW_MODE           BIT(7)

/* Error register fields */
#define CCS811_ERROR_WRITE_REG_INVALID  BIT(0)
#define CCS811_ERROR_READ_REG_INVALID   BIT(1)
#define CCS811_ERROR_MEASMODE_INVALID   BIT(2)
#define CCS811_ERROR_MAX_RESISTANCE     BIT(3)
#define CCS811_ERROR_HEATER_FAULT       BIT(4)
#define CCS811_ERROR_HEATER_SUPPLY      BIT(5)

/* Measurement mode constants */
#define CCS811_MODE_IDLE                0x00
#define CCS811_MODE_IAQ_1SEC            0x10
#define CCS811_MODE_IAQ_10SEC           0x20
#define CCS811_MODE_IAQ_60SEC           0x30
#define CCS811_MODE_IAQ_250MSEC         0x40
#define CCS811_MODE_MSK                 0x70

/** @brief Information collected from the sensor on each fetch. */
struct ccs811_result_type {
	/** Equivalent carbon dioxide in parts-per-million volume (ppmv). */
	uint16_t co2;

	/**
	 * Equivalent total volatile organic compounts in
	 * parts-per-billion volume.
	 */
	uint16_t voc;

	/** Raw voltage and current measured by sensor. */
	uint16_t raw;

	/** Sensor status at completion of most recent fetch. */
	uint8_t status;

	/**
	 * Sensor error flags at completion of most recent fetch.
	 *
	 * Note that errors are cleared when read.
	 */
	uint8_t error;
};

/**
 * @brief Access storage for the most recent data read from the sensor.
 *
 * This content of the object referenced is updated by
 * sensor_fetch_sample(), except for ccs811_result_type::mode which is
 * set on driver initialization.
 *
 * @param dev Pointer to the sensor device
 *
 * @return a pointer to the result information.
 */
const struct ccs811_result_type *ccs811_result(const struct device *dev);

/**
 * @brief Get information about static CCS811 state.
 *
 * This includes the configured operating mode as well as hardware and
 * firmware versions.
 */
struct ccs811_configver_type {
	uint16_t fw_boot_version;
	uint16_t fw_app_version;
	uint8_t hw_version;
	uint8_t mode;
};

/**
 * @brief Fetch operating mode and version information.
 *
 * @param dev Pointer to the sensor device
 *
 * @param ptr Pointer to where the returned information should be stored
 *
 * @return 0 on success, or a negative errno code on failure.
 */
int ccs811_configver_fetch(const struct device *dev,
			   struct ccs811_configver_type *ptr);

/**
 * @brief Fetch the current value of the BASELINE register.
 *
 * The BASELINE register encodes data used to correct sensor readings
 * based on individual device configuration and variation over time.
 *
 * For proper management of the BASELINE register see AN000370
 * "Baseline Save and Restore on CCS811".
 *
 * @param dev Pointer to the sensor device
 *
 * @return a non-negative 16-bit register value, or a negative errno
 * code on failure.
 */
int ccs811_baseline_fetch(const struct device *dev);

/**
 * @brief Update the BASELINE register.
 *
 * For proper management of the BASELINE register see AN000370
 * "Baseline Save and Restore on CCS811".
 *
 * @param dev Pointer to the sensor device
 *
 * @param baseline the value to be stored in the BASELINE register.
 *
 * @return 0 if successful, negative errno code if failure.
 */
int ccs811_baseline_update(const struct device *dev, uint16_t baseline);

/**
 * @brief Update the ENV_DATA register.
 *
 * Accurate calculation of gas levels requires accurate environment
 * data.  Measurements are only accurate to 0.5 Cel and 0.5 %RH.
 *
 * @param dev Pointer to the sensor device
 *
 * @param temperature the current temperature at the sensor
 *
 * @param humidity the current humidity at the sensor
 *
 * @return 0 if successful, negative errno code if failure.
 */
int ccs811_envdata_update(const struct device *dev,
			  const struct sensor_value *temperature,
			  const struct sensor_value *humidity);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_CCS811_H_ */
