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

#ifdef __cplusplus
}
#endif

#endif
