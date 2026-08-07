/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Realtek Bee GPIO Controller specific definitions
 *
 * This file contains the GPIO configuration flags specific to the Realtek Bee
 * series SoCs, used in Device Tree bindings.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_BEE_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_BEE_GPIO_H_

/**
 * @name GPIO Debounce Configuration
 * @{
 */

/** @brief Bit position for debounce time (in milliseconds) */
#define BEE_GPIO_INPUT_DEBOUNCE_MS_POS 8

/** @brief Bit mask for debounce time configuration */
#define BEE_GPIO_INPUT_DEBOUNCE_MS_MASK (0xff << BEE_GPIO_INPUT_DEBOUNCE_MS_POS)

/** @} */

/**
 * @brief Enable GPIO pin debounce.
 *
 * The debounce flag is a Zephyr specific extension of the standard GPIO flags
 * specified by the Linux GPIO binding. Only applicable for Realtek bee SoCs.
 *
 * @param ms Debounce time in milliseconds (0-255).
 */
#define BEE_GPIO_INPUT_DEBOUNCE_MS(ms) ((0xff & (ms)) << BEE_GPIO_INPUT_DEBOUNCE_MS_POS)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_BEE_GPIO_H_ */
