/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Semtech SX1509B-specific GPIO devicetree flags.
 * @ingroup gpio_interface_sx1509b
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_SEMTECH_SX1509B_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_SEMTECH_SX1509B_H_

/**
 * @defgroup gpio_interface_sx1509b SX1509B-specific GPIO flags
 * @ingroup gpio_interface_ext
 * @{
 */

/**
 * @brief Enable GPIO pin debounce.
 *
 * The debounce flag is a Zephyr specific extension of the standard GPIO flags
 * specified by the Linux GPIO binding. Only applicable for Semtech SX1509B GPIO
 * controllers.
 */
#define SX1509B_GPIO_DEBOUNCE (1U << 8)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_SEMTECH_SX1509B_H_ */
