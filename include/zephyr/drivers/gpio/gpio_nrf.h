/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_NRF_H
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_NRF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get pointer to GPIOTE driver instance
 * associated with specified port device node.
 *
 * @param port Pointer to port device node.
 *
 * @return Pointer to GPIOTE driver instance. NULL if not found.
 */
void *gpio_nrf_gpiote_by_port_get(const struct device *port);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_NRF_H */
