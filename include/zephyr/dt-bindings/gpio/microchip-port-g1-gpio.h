/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_MICROCHIP_PORT_G1_GPIO_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_MICROCHIP_PORT_G1_GPIO_H_

/**
 * @brief Enable GPIO pin debounce.
 *
 * The debounce flag is a Zephyr specific extension of the standard GPIO flags
 * specified by the Linux GPIO binding. Only applicable for Microchip SoCs.
 */
#define MCHP_GPIO_DEBOUNCE (1U << 8)

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_MICROCHIP_PORT_G1_GPIO_H_ */
