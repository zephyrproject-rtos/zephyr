/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NPM1300_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NPM1300_GPIO_H_

/**
 * @brief nPM1300-specific GPIO Flags
 * @defgroup gpio_interface_npm1300 nPM1300-specific GPIO Flags
 *
 * The drive flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as
 * follows:
 *
 * - Bit 8: Drive strength (0=1mA, 1=6mA)
 * - Bit 9: Debounce (0=OFF, 1=ON)
 *
 * @ingroup gpio_interface
 * @{
 */

/**
 * @name nPM1300 GPIO drive strength flags
 * @brief nPM1300 GPIO drive strength flags
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/** Drive mode field mask */
#define NPM1300_GPIO_DRIVE_MSK 0x0100U
/** @endcond */

/** 1mA drive */
#define NPM1300_GPIO_DRIVE_1MA (0U << 8U)
/** 6mA drive */
#define NPM1300_GPIO_DRIVE_6MA (1U << 8U)

/** @} */

/**
 * @name nPM1300 GPIO debounce flags
 * @brief nPM1300 GPIO debounce flags
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/** Debounce field mask */
#define NPM1300_GPIO_DEBOUNCE_MSK 0x0200U
/** @endcond */

/** Normal drive */
#define NPM1300_GPIO_DEBOUNCE_OFF (0U << 9U)
/** High drive */
#define NPM1300_GPIO_DEBOUNCE_ON  (1U << 9U)

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NPM1300_GPIO_H_ */
