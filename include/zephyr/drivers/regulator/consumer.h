/**
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Regulator consumer API based on linux regulator API
 *
 * This API extends the Zephyr regulator API by implementing voltage
 * and current level control for supported regulators. Currently, the only
 * supported device is the NXP PCA9420 PMIC.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PMIC_CONSUMER_H_
#define ZEPHYR_INCLUDE_DRIVERS_PMIC_CONSUMER_H_

#include <zephyr/types.h>
#include <zephyr/drivers/regulator.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return the number of supported voltage levels
 * Returns the number of selectors, or negative errno. Selectors are
 * numbered starting at zero, and typically correspond to bitfields
 * in hardware registers.
 *
 * @param dev: Regulator device to count voltage levels for.
 * @return number of selectors, or negative errno.
 */
int regulator_count_voltages(const struct device *dev);

/**
 * @brief Return the number of supported regulator modes
 * Returns the number of supported regulator modes. Many regulators will only
 * support one mode. Regulator modes can be set and selected with
 * regulator_set_mode
 *
 * @param dev: Regulator device to count supported regulator modes for
 * @return number of supported modes
 */
int regulator_count_modes(const struct device *dev);

/**
 * @brief Return supported voltage
 * Returns a voltage that can be passed to @ref regulator_set_voltage(), zero
 * if the selector code can't be used, or a negative errno.
 *
 * @param dev: Regulator device to get voltage for.
 * @param selector: voltage selector code.
 * @return voltage level in uV, or zero if selector code can't be used.
 */
int regulator_list_voltages(const struct device *dev, unsigned int selector);

/**
 * @brief Check if a voltage range can be supported.
 *
 * @param dev: Regulator to check range against.
 * @param min_uV: Minimum voltage in microvolts
 * @param max_uV: maximum voltage in microvolts
 * @returns boolean or negative error code.
 */
int regulator_is_supported_voltage(const struct device *dev, int min_uV, int max_uV);

/**
 * @brief Set regulator output voltage.
 * Sets a regulator to the closest supported output voltage.
 * @param dev: Regulator to set voltage
 * @param min_uV: Minimum acceptable voltage in microvolts
 * @param max_uV: Maximum acceptable voltage in microvolts
 */
int regulator_set_voltage(const struct device *dev, int min_uV, int max_uV);

/**
 * @brief Get regulator output voltage.
 * Returns the current regulator voltage in microvolts
 *
 * @param dev: Regulator to query
 * @return voltage level in uV
 */
int regulator_get_voltage(const struct device *dev);

/**
 * @brief Set regulator output current limit
 * Sets current sink to desired output current.
 * @param dev: Regulator to set output current level
 * @param min_uA: minimum microamps
 * @param max_uA: maximum microamps
 * @return 0 on success, or errno on error
 */
int regulator_set_current_limit(const struct device *dev, int min_uA, int max_uA);

/**
 * @brief Get regulator output current.
 * Note the current limit must have been set for this call to succeed.
 * @param dev: Regulator to query
 * @return current limit in uA, or errno
 */
int regulator_get_current_limit(const struct device *dev);

/**
 * @brief Select mode of regulator
 * Regulators can support multiple modes in order to permit different voltage
 * configuration or better power savings. This API will apply a mode for
 * the regulator.
 * @param dev: regulator to switch mode for
 * @param mode: Mode to select for this regulator. Only modes present
 * in the regulator-allowed-modes property are permitted.
 * @return 0 on success, or errno on error
 */
int regulator_set_mode(const struct device *dev, uint32_t mode);

/**
 * @brief Set target voltage for regulator mode
 * Part of the extended regulator consumer API.
 * sets the target voltage for a given regulator mode. This mode does
 * not need to be the active mode. This API can be used to configure
 * voltages for a mode, then the regulator can be switched to that mode
 * with the regulator_set_mode api.
 * @param dev: regulator to set voltage for
 * @param mode: target mode to configure voltage for
 * @param min_uV: minimum voltage acceptable, in uV
 * @param max_uV: maximum voltage acceptable, in uV
 * @return 0 on success, or errno on error
 */
int regulator_set_mode_voltage(const struct device *dev, uint32_t mode,
	uint32_t min_uV, uint32_t max_uV);

/**
 * @brief Get target voltage for regulator mode
 * Part of the extended regulator consumer API.
 * gets the target voltage for a given regulator mode. This mode does
 * not need to be the active mode. This API can be used to read voltages
 * from a regulator mode other than the default.
 * @param dev: regulator to query voltage from
 * @param mode: target mode to query voltage from
 * @return voltage level in uV
 */
int regulator_get_mode_voltage(const struct device *dev, uint32_t mode);

/**
 * @brief Disable regulator for a given mode
 * Part of the extended regulator consumer API.
 * Disables the regulator in a given mode. Does not implement the
 * onoff service, as this is incompatible with multiple mode operation
 * @param dev: regulator to disable
 * @param mode: mode to change regulator state in
 */
int regulator_mode_disable(const struct device *dev, uint32_t mode);

/**
 * @brief Enable regulator for a given mode
 * Part of the extended regulator consumer API.
 * Enables the regulator in a given mode. Does not implement the
 * onoff service, as this is incompatible with multiple mode operation
 * @param dev: regulator to enable
 * @param mode: mode to change regulator state in
 */
int regulator_mode_enable(const struct device *dev, uint32_t mode);

#ifdef __cplusplus
}
#endif

#endif
