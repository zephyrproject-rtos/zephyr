/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NPM10XX_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NPM10XX_GPIO_H_

/**
 * @file nordic-npm10xx-gpio.h
 * @brief nPM10xx-specific GPIO Flags
 * @defgroup gpio_interface_npm10xx nPM10xx-specific GPIO Flags
 *
 * The flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as follows:
 *
 * - Bit 8: Drive strength (0=Normal, 1=High)
 * - Bit 9: Debounce (0=OFF, 1=ON)
 *
 * @ingroup gpio_interface_ext
 * @{
 */

/**
 * @name nPM10xx GPIO drive strength flags
 * @{
 */
/** @cond INTERNAL_HIDDEN */
/** Drive mode field mask */
#define NPM10XX_GPIO_DRIVE_MSK 0x0100U
/** @endcond */
/** Normal (2mA) drive */
#define NPM10XX_GPIO_DRIVE_NORMAL (0U << 8U)
/** High (4mA) drive */
#define NPM10XX_GPIO_DRIVE_HIGH   (1U << 8U)
/** @} */

/**
 * @name nPM10xx GPIO debounce flags
 * @{
 */
/** @cond INTERNAL_HIDDEN */
/** Debounce field mask */
#define NPM10XX_GPIO_DEBOUNCE_MSK 0x0200U
/** @endcond */
/** Debounce off */
#define NPM10XX_GPIO_DEBOUNCE_OFF (0U << 9U)
/** Debounce on */
#define NPM10XX_GPIO_DEBOUNCE_ON  (1U << 9U)
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NPM10XX_GPIO_H_ */
