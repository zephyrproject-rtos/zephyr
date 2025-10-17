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
 * @param timestamp_ms Timestamp associated with the GNSS fix
 */
void gnss_emul_set_data(const struct device *dev, const struct navigation_data *nav,
			const struct gnss_info *info, int64_t timestamp_ms);

#endif /* ZEPHYR_DRIVERS_GNSS_GNSS_EMUL_H_ */
