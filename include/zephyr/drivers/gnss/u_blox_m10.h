/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended API of the u-blox M10 GNSS driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GNSS_U_BLOX_M10_H_
#define ZEPHYR_INCLUDE_DRIVERS_GNSS_U_BLOX_M10_H_

#include <stdbool.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Drive the receiver's EXTINT input
 *
 * Sets the level of the GPIO given by the @c extint-gpios devicetree property. The level
 * stays until the next call. The driver drives EXTINT active during initialization.
 *
 * With @c extint-wake, the receiver stays awake while EXTINT is active (high). With
 * @c extint-backup, it enters backup mode while EXTINT is inactive (low), and wakes up when
 * EXTINT is set active again. In any configuration, an edge on EXTINT wakes the receiver
 * from software backup mode entered on device suspend.
 *
 * @param dev u-blox M10 device.
 * @param active True to drive EXTINT active (high), false to drive it inactive (low).
 *
 * @retval 0 Success.
 * @retval -ENOTSUP The device has no @c extint-gpios property.
 * @retval -errno Negative errno code returned by the GPIO driver.
 */
int gnss_u_blox_m10_set_extint(const struct device *dev, bool active);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GNSS_U_BLOX_M10_H_ */
