/*
 * Copyright 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UART_UART_BRIDGE_H_
#define ZEPHYR_INCLUDE_DRIVERS_UART_UART_BRIDGE_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Update the hardware port settings on a uart bridge
 *
 * If dev is part bridge_dev, then the dev uart configuration are applied to
 * the other device in the uart bridge. This allows propagating the settings
 * from a USB CDC-ACM port to a hardware UART.
 *
 * If dev is not part of bridge_dev then the function is a no-op.
 */
void uart_bridge_settings_update(const struct device *dev,
				 const struct device *bridge_dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_UART_UART_BRIDGE_H_ */
