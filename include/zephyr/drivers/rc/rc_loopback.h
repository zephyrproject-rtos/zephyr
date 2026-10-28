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

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RC_RC_LOOPBACK_H_ */
