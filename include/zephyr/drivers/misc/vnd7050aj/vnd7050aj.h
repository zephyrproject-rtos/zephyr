/*
 * Copyright (c) 2024, Technical Report Author
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_POWER_VND7050AJ_H_
#define ZEPHYR_INCLUDE_DRIVERS_POWER_VND7050AJ_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Channel identifiers for the VND7050AJ.
 */
#define VND7050AJ_CHANNEL_0 0
#define VND7050AJ_CHANNEL_1 1

/**
 * @brief Sets the state of a specific output channel.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to control (VND7050AJ_CHANNEL_0 or VND7050AJ_CHANNEL_1).
 * @param state The desired state (true for ON, false for OFF).
 * @return 0 on success, negative error code on failure.
 */
int vnd7050aj_set_output_state(const struct device *dev, uint8_t channel, bool state);

/**
 * @brief Reads the load current for a specific channel.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to measure (VND7050AJ_CHANNEL_0 or VND7050AJ_CHANNEL_1).
 * @param[out] current_ma Pointer to store the measured current in milliamperes (mA).
 * @return 0 on success, negative error code on failure.
 */
int vnd7050aj_read_load_current(const struct device *dev, uint8_t channel, int32_t *current_ma);

/**
 * @brief Reads the VCC supply voltage.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param[out] voltage_mv Pointer to store the measured voltage in millivolts (mV).
 * @return 0 on success, negative error code on failure.
 */
int vnd7050aj_read_supply_voltage(const struct device *dev, int32_t *voltage_mv);

/**
 * @brief Reads the internal chip temperature.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param[out] temp_c Pointer to store the measured temperature in degrees Celsius (°C).
 * @return 0 on success, negative error code on failure.
 */
int vnd7050aj_read_chip_temp(const struct device *dev, int32_t *temp_c);

/**
 * @brief Resets a latched fault condition.
 *
 * This function sends a low pulse to the FaultRST pin.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @return 0 on success, negative error code on failure.
 */
int vnd7050aj_reset_fault(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_POWER_VND7050AJ_H_ */
