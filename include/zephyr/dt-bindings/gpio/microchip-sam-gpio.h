/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 * @brief Microchip SAM PIO4-specific GPIO devicetree flags.
 * @ingroup gpio_interface_microchip_sam
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_MICROCHIP_SAM_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_MICROCHIP_SAM_GPIO_H_

/**
 * @defgroup gpio_interface_microchip_sam Microchip SAM PIO4-specific GPIO flags
 * @ingroup gpio_interface_ext
 * @{
 */

/** Disable slew rate control, which the driver enables by default. */
#define SAM_GPIO_DIS_SLEWRATE       (1U << 8)
/** Enable input debouncing (IFEN and IFSCEN). */
#define SAM_GPIO_DEBOUNCE       (1U << 9)
/** Disable the Schmitt trigger, which is enabled after reset. */
#define SAM_GPIO_DIS_SCHMIT     (1U << 10)

/**
 * @name Drive strength
 *
 * Without one of these flags, @ref SAM_GPIO_DRVSTR_DEFAULT is used.
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define SAM_GPIO_DRVSTR_POS     (11U)
#define SAM_GPIO_DRVSTR_MASK        (0x3U << SAM_GPIO_DRVSTR_POS)
/** @endcond */
/** Default drive strength. */
#define SAM_GPIO_DRVSTR_DEFAULT     (0U << SAM_GPIO_DRVSTR_POS)
/** Low drive strength. */
#define SAM_GPIO_DRVSTR_LOW     (1U << SAM_GPIO_DRVSTR_POS)
/** Medium drive strength. */
#define SAM_GPIO_DRVSTR_MED     (2U << SAM_GPIO_DRVSTR_POS)
/** High drive strength. */
#define SAM_GPIO_DRVSTR_HI      (3U << SAM_GPIO_DRVSTR_POS)
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_MICROCHIP_SAM_GPIO_H_ */
