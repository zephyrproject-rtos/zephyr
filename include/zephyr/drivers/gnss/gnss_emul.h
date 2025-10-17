/*
 * Copyright (c) 2025 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GNSS_GNSS_EMUL_H_
#define ZEPHYR_DRIVERS_GNSS_GNSS_EMUL_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gnss.h>

/**
 * @brief Clear all internal GNSS data of the emulator
 *
 * @param dev GNSS emulator device
 */
void gnss_emul_clear_data(const struct device *dev);

/**
 * @brief Set the internal GNSS data of the emulator
 *
 * @param dev GNSS emulator device
 * @param nav Updated navigation state
 * @param info Updated GNSS fix information
 * @param boot_realtime_ms Unix timestamp associated with system boot
 */
void gnss_emul_set_data(const struct device *dev, const struct navigation_data *nav,
			const struct gnss_info *info, int64_t boot_realtime_ms);

/**
 * @brief Retrieve the last configured fix rate, regardless of PM state
 *
 * @param dev GNSS emulator device
 * @param fix_interval_ms Output fix interval
 *
 * @retval 0 On success
 */
int gnss_emul_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms);

/**
 * @brief Retrieve the last configured navigation mode, regardless of PM state
 *
 * @param dev GNSS emulator device
 * @param mode Output navigation mode
 *
 * @retval 0 On success
 */
int gnss_emul_get_navigation_mode(const struct device *dev, enum gnss_navigation_mode *mode);

/**
 * @brief Retrieve the last configured systems, regardless of PM state
 *
 * @param dev GNSS emulator device
 * @param systems Output GNSS systems
 *
 * @retval 0 On success
 */
int gnss_emul_get_enabled_systems(const struct device *dev, gnss_systems_t *systems);

#endif /* ZEPHYR_DRIVERS_GNSS_GNSS_EMUL_H_ */
