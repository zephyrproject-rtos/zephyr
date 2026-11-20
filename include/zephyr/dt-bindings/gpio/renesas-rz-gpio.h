/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RZ/A, RZ/G and RZ/V-specific GPIO devicetree flags.
 * @ingroup gpio_interface_renesas_rz
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZ_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZ_GPIO_H_

/*********************************RZ/A,G,V**************************************/

/**
 * @defgroup gpio_interface_renesas_rz Renesas RZ/A, RZ/G and RZ/V-specific GPIO flags
 * @ingroup gpio_interface_ext
 *
 * The flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as follows:
 *
 * - Bits 8-9: pin driving ability (IOLH)
 * - Bit 10: digital noise filter on/off (FILONOFF)
 * - Bits 11-12: digital noise filter number (FILNUM)
 * - Bits 13-14: digital noise filter clock selection (FILCLKSEL)
 *
 * Example:
 *
 * @code{.dts}
 * gpio-consumer {
 *	out-gpios = <&port8 2 (GPIO_PULL_UP | RZ_GPIO_FILTER_SET(1, 3, 3))>;
 * };
 * gpio-consumer {
 *	out-gpios = <&port8 2 (GPIO_PULL_UP | RZ_GPIO_IOLH_SET(2))>;
 * };
 * @endcode
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define RZ_GPIO_IOLH_SHIFT         8U
/** @endcond */

/**
 * @brief Set the pin driving ability.
 *
 * @param iolh_val IOLH value, from 0 to 3.
 */
#define RZ_GPIO_IOLH_SET(iolh_val) (iolh_val << RZ_GPIO_IOLH_SHIFT)

/** @cond INTERNAL_HIDDEN */
#define RZ_GPIO_FILTER_SHIFT    10U
#define RZ_GPIO_FILNUM_SHIFT    1U
#define RZ_GPIO_FILCLKSEL_SHIFT 3U
/** @endcond */

/**
 * @brief Configure the digital noise filter.
 *
 * @param fillonoff 1 to enable the filter, 0 to disable it.
 * @param filnum FILNUM value, from 0 to 3.
 * @param filclksel FILCLKSEL value, from 0 to 3.
 */
#define RZ_GPIO_FILTER_SET(fillonoff, filnum, filclksel)                                           \
	(((fillonoff) | ((filnum) << RZ_GPIO_FILNUM_SHIFT) |                                       \
	  ((filclksel) << RZ_GPIO_FILCLKSEL_SHIFT))                                                \
	 << RZ_GPIO_FILTER_SHIFT)

/** @} */

/*******************************************************************************/

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZ_GPIO_H_ */
