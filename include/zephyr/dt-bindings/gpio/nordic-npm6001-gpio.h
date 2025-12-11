/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NPM6001_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NPM6001_GPIO_H_

/**
 * @brief nPM6001-specific GPIO Flags
 * @defgroup gpio_interface_npm6001 nPM6001-specific GPIO Flags
 *
 * The drive flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as
 * follows:
 *
 * - Bit 8: Drive strength (0=NORMAL, 1=HIGH)
 * - Bit 9: Input type (0=SCHMITT, 1=CMOS)
 *
 * @ingroup gpio_interface_ext
 * @{
 */

/**
 * @name nPM6001 GPIO drive strength flags
 * @brief nPM6001 GPIO drive strength flags
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/** Drive mode field mask */
#define NPM6001_GPIO_DRIVE_MSK	0x0100U
/** @endcond */

/** Normal drive */
#define NPM6001_GPIO_DRIVE_NORMAL	(0U << 8U)
/** High drive */
#define NPM6001_GPIO_DRIVE_HIGH		(1U << 8U)

/** @} */

/**
 * @name nPM6001 GPIO drive strength flags
 * @brief nPM6001 GPIO drive strength flags
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/** Input type field mask */
#define NPM6001_GPIO_SENSE_MSK	0x0200U
/** @endcond */

/** Schmitt trigger input type */
#define NPM6001_GPIO_SENSE_SCHMITT	(0U << 9U)
/** CMOS input type */
#define NPM6001_GPIO_SENSE_CMOS		(1U << 9U)

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NRF_GPIO_H_ */
