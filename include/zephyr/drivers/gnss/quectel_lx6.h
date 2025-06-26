/*
 * Copyright (c) 2025 Nick Ward <nix.ward@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GNSS_QUECTEL_LX6_H_
#define ZEPHYR_DRIVERS_GNSS_QUECTEL_LX6_H_

#include <zephyr/drivers/gnss.h>

/**
 * @brief Perform a full cold restart of the Quectel LX6 GNSS module.
 *
 * This function sends the PMTK104 command to trigger a full cold start.
 * A full cold start clears all saved GNSS data, including ephemeris,
 * almanac, time, position, and configuration. The module will perform
 * a complete re-acquisition of satellite data upon reboot.
 *
 * @note This function must only be called when the driver is active and not
 * suspended.
 *
 * After calling this function, the GNSS module may be temporarily unavailable
 * for several hundred milliseconds during its reboot.
 *
 * @param dev Pointer to the Quectel LX6 device
 *
 * @retval 0 on success
 * @retval -errno code otherwise
 */
int quectel_lx6_cold_start(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_GNSS_QUECTEL_LX6_H_ */
