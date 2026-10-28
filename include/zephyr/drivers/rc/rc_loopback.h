/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RC_RC_LOOPBACK_H_
#define ZEPHYR_INCLUDE_DRIVERS_RC_RC_LOOPBACK_H_

/**
 * @file
 * @ingroup rc_loopback
 * @brief Test helpers of the loopback remote control driver.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Loopback remote control driver test helpers
 * @defgroup rc_loopback Remote Control Loopback
 * @ingroup rc_driver_interface
 * @{
 */

/**
 * @brief Check whether reception is enabled.
 *
 * @param dev Loopback rc device.
 *
 * @retval true Reception is enabled.
 * @retval false Reception is disabled.
 */
bool rc_loopback_rx_is_enabled(const struct device *dev);

/**
 * @brief Get the receiver timeout set by the rc core.
 *
 * @param dev Loopback rc device.
 *
 * @return Timeout in us, 0 if never set.
 */
uint32_t rc_loopback_rx_timeout_us(const struct device *dev);

/**
 * @brief Inject raw words as if received.
 *
 * @param dev Loopback rc device.
 * @param words Raw words.
 * @param count Number of words.
 *
 * @retval 0 Success.
 * @retval -EAGAIN Reception is disabled.
 * @retval -ENOMEM Receive buffer full, remaining words dropped.
 */
int rc_loopback_inject(const struct device *dev, const uint32_t *words, size_t count);

/**
 * @brief Get the last transmission and the carrier set by the rc core.
 *
 * @param dev Loopback rc device.
 * @param words Filled with the words of the last transmission.
 * @param max Size of @p words.
 * @param hz Filled with the last carrier frequency set, 0 if never set.
 * @param duty_pct Filled with the last duty cycle set, 0 if never set.
 *
 * @return Number of words copied.
 */
size_t rc_loopback_tx_get(const struct device *dev, uint32_t *words, size_t max, uint32_t *hz,
			  uint8_t *duty_pct);

/**
 * @brief Get the number of transmissions so far.
 *
 * @param dev Loopback rc device.
 *
 * @return Number of transmissions.
 */
uint32_t rc_loopback_tx_total(const struct device *dev);

/**
 * @brief Hold or release transmissions.
 *
 * @param dev Loopback rc device.
 * @param hold true to hold, false to release.
 */
void rc_loopback_tx_hold(const struct device *dev, bool hold);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RC_RC_LOOPBACK_H_ */
