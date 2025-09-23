/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NRF_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NRF_GPIO_H_

/**
 * @brief nRF-specific GPIO Flags
 * @defgroup gpio_interface_nrf nRF-specific GPIO Flags
 * @ingroup gpio_interface_ext
 * @{
 */

/**
 * @name nRF GPIO drive flags
 * @brief nRF GPIO drive flags
 *
 * Standard (S) or High (H) drive modes can be applied to both pin levels, 0 or
 * 1. High drive mode will increase current capabilities of the pin (refer to
 * each SoC reference manual).
 *
 * When the pin is configured to operate in open-drain mode (wired-and), the
 * drive mode can only be selected for the 0 level (1 is disconnected).
 * Similarly, when the pin is configured to operate in open-source mode
 * (wired-or), the drive mode can only be set for the 1 level
 * (0 is disconnected).
 *
 * The drive flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as
 * follows:
 *
 * - Bit 8: Drive mode for '0' (0=Standard, 1=High)
 * - Bit 9: Drive mode for '1' (0=Standard, 1=High)
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/** Drive mode field mask */
#define NRF_GPIO_DRIVE_MSK	0x0300U
/** @endcond */

/** Standard drive for '0' (default, used with GPIO_OPEN_DRAIN) */
#define NRF_GPIO_DRIVE_S0	(0U << 8U)
/** High drive for '0' (used with GPIO_OPEN_DRAIN) */
#define NRF_GPIO_DRIVE_H0	(1U << 8U)
/** Standard drive for '1' (default, used with GPIO_OPEN_SOURCE) */
#define NRF_GPIO_DRIVE_S1	(0U << 9U)
/** High drive for '1' (used with GPIO_OPEN_SOURCE) */
#define NRF_GPIO_DRIVE_H1	(1U << 9U)
/** Standard drive for '0' and '1' (default) */
#define NRF_GPIO_DRIVE_S0S1	(NRF_GPIO_DRIVE_S0 | NRF_GPIO_DRIVE_S1)
/** Standard drive for '0' and high for '1' */
#define NRF_GPIO_DRIVE_S0H1	(NRF_GPIO_DRIVE_S0 | NRF_GPIO_DRIVE_H1)
/** High drive for '0' and standard for '1' */
#define NRF_GPIO_DRIVE_H0S1	(NRF_GPIO_DRIVE_H0 | NRF_GPIO_DRIVE_S1)
/** High drive for '0' and '1' */
#define NRF_GPIO_DRIVE_H0H1	(NRF_GPIO_DRIVE_H0 | NRF_GPIO_DRIVE_H1)

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NRF_GPIO_H_ */
