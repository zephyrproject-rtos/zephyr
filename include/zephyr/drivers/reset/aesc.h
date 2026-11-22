/*
 * SPDX-FileCopyrightText: 2026 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Aesc Silicon reset controller specific API
 * @ingroup reset_controller_aesc
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RESET_AESC_H_
#define ZEPHYR_INCLUDE_DRIVERS_RESET_AESC_H_

#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Aesc Silicon reset controller specific API
 * @defgroup reset_controller_aesc Aesc Silicon reset controller
 * @ingroup reset_controller_interface
 * @{
 */

/**
 * @brief Reset several reset domains of an Aesc Silicon reset controller at once.
 *
 * All domains in @p mask are triggered with a single register write, so they
 * enter reset in the same cycle. The function returns after the request and
 * does not wait for the domains to leave reset, e.g. to reset the domain of
 * the calling CPU.
 *
 * @param dev Reset controller device.
 * @param mask Bit mask of reset domain identifiers.
 *
 * @retval 0 Reset triggered.
 * @retval -EINVAL Empty mask or mask contains a domain the controller does not have.
 */
int reset_aesc_toggle_mask(const struct device *dev, uint32_t mask);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RESET_AESC_H_ */
