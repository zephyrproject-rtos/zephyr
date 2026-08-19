/*
 * Copyright (c) 2024, Orgatex GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the BQ35100
 */

#ifndef ZEPHYR_DRIVERS_FUEL_GAUGE_BQ35100_PROPS_H_
#define ZEPHYR_DRIVERS_FUEL_GAUGE_BQ35100_PROPS_H_

#include <zephyr/drivers/fuel_gauge.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum fuel_gauge_prop_type_bq35100
 * @brief Enumeration of custom property types for BQ35100 fuel gauge.
 */
enum fuel_gauge_prop_type_bq35100 {
	/**
	 * @brief Initialize a new battery.
	 * @note Requires fuel_gauge_prop_val: design_cap to be set.
	 */
	FUEL_GAUGE_BQ35100_NEW_BATTERY = FUEL_GAUGE_CUSTOM_BEGIN,

	/**
	 * @brief Reset the fuel gauge.
	 * @note Does not require any fuel_gauge_prop_val.
	 */
	FUEL_GAUGE_BQ35100_RESET,

	/**
	 * @brief Start the fuel gauge.
	 * @note Does not require any fuel_gauge_prop_val.
	 */
	FUEL_GAUGE_BQ35100_START,

	/**
	 * @brief Stop the fuel gauge.
	 * @note Does not require any fuel_gauge_prop_val.
	 */
	FUEL_GAUGE_BQ35100_STOP,

	/**
	 * @brief Set the threshold when battery alert low voltage schould be triggered
	 * @note Requires fuel_gauge_prop_val: design_volt_mv as the threshold to be set.
	 */
	FUEL_GAUGE_BQ35100_ALERT_CONF,

	/**
	 * @brief Get the reason for a battery alert
	 * @note This property will read the BatteryAlert and BatteryStatus register
	 * @warning Reading the BatteryAlert() register will cause the alert pin to reset
	 */
	FUEL_GAUGE_BQ35100_BATTERY_STATUS_ALERT,

	/**
	 * @brief Set security mode sealed
	 *
	 */
	FUEL_GAUGE_BQ35100_SEC_MODE_SEALED,

	/**
	 * @brief Set security mode unsealed
	 *
	 */
	FUEL_GAUGE_BQ35100_SEC_MODE_UNSEALED,
};

/**
 * @brief Calibrate voltage of BQ35100
 *
 * @param dev Bq35100 device
 * @param known_volt Voltage applied while calibrating  (in mA)
 * @return 0 on success and a negative error code otherwise
 */
int bq35100_calibrate_gauge_volt(const struct device *dev, const uint16_t known_volt);

/**
 * @brief Calibrate current of BQ35100
 *
 * @param dev BQ35100 device
 * @param known_current Current applied while calibrating (in mA)
 * @return 0 on success, negative error code otherwise
 */
int bq35100_calibrate_gauge_current(const struct device *dev, const uint16_t known_current);

/**
 * @brief Perform CC offset calibration
 *
 * @note This calibrates the coulomb counter (CC) offset of the BQ35100.
 *
 * @param dev BQ35100 device
 * @return 0 on success, negative error code otherwise
 */
int bq35100_perform_cc_offset(const struct device *dev);

/**
 * @brief Perform board offset calibration
 *
 * @note This calibrates the system-level offset of the BQ35100.
 *
 * @param dev BQ35100 device
 * @return 0 on success, negative error code otherwise
 */
int bq35100_perform_board_offset(const struct device *dev);

/**
 * @brief Store voltage gain calibration value
 *
 * @note Writes the voltage gain calibration value to BQ35100 data flash (DF).
 *
 * @param dev BQ35100 device
 * @param voltage_gain Calibrated voltage gain value
 * @return 0 on success, negative error code otherwise
 */
int bq35100_store_calibration_volt(const struct device *dev, uint16_t voltage_gain);

/**
 * @brief Store current gain and delta calibration value
 *
 * @note Writes the current gain and delta calibration value to BQ35100 DF.
 *
 * @param dev BQ35100 device
 * @param cc_gain Calibrated current gain value
 * @return 0 on success, negative error code otherwise
 */
int bq35100_store_calibration_cc_gain_delta(const struct device *dev, float cc_gain);

/**
 * @brief Store coulomb counter (CC) offset
 *
 * @note Writes the CC offset value to BQ35100 DF.
 *
 * @param dev BQ35100 device
 * @param cc_offset Calibrated CC offset
 * @return 0 on success, negative error code otherwise
 */
int bq35100_store_calibration_cc_offset(const struct device *dev, int16_t cc_offset);

/**
 * @brief Store board offset value
 *
 * @note Writes the board offset value to BQ35100 DF.
 *
 * @param dev BQ35100 device
 * @param board_offset Calibrated board offset
 * @return 0 on success, negative error code otherwise
 */
int bq35100_store_calibration_board_offset(const struct device *dev, int8_t board_offset);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_FUEL_GAUGE_BQ35100_PROPS_H_ */
