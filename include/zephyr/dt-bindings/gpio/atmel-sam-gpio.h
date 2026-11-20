/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAM-specific GPIO devicetree flags.
 * @ingroup gpio_interface_atmel_sam
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ATMEL_SAM_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ATMEL_SAM_GPIO_H_

/**
 * @defgroup gpio_interface_atmel_sam Atmel SAM-specific GPIO flags
 * @ingroup gpio_interface_ext
 * @{
 */

/**
 * @brief Enable GPIO pin debounce.
 *
 * The debounce flag is a Zephyr specific extension of the standard GPIO flags
 * specified by the Linux GPIO binding. Only applicable for Atmel SAM SoCs.
 */
#define SAM_GPIO_DEBOUNCE (1U << 8)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ATMEL_SAM_GPIO_H_ */
