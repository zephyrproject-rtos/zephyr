/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RA-specific GPIO devicetree flags.
 * @ingroup gpio_interface_renesas_ra
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RA_GPIO_IOPORT_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RA_GPIO_IOPORT_H_

/**
 * @defgroup gpio_interface_renesas_ra Renesas RA-specific GPIO flags
 * @ingroup gpio_interface_ext
 *
 * The drive strength is encoded in bits 8-9 of @ref gpio_dt_flags_t.
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define RENESAS_GPIO_DS_POS                   (8)
#define RENESAS_GPIO_DS_MSK                   (0x3U << RENESAS_GPIO_DS_POS)
/** @endcond */

/**
 * @name Drive strength
 * @{
 */
/** Low drive strength. */
#define RENESAS_GPIO_DS_LOW                   (0x0 << RENESAS_GPIO_DS_POS)
/** Middle drive strength. */
#define RENESAS_GPIO_DS_MIDDLE                (0x1 << RENESAS_GPIO_DS_POS)
/** High-speed high drive strength. */
#define RENESAS_GPIO_DS_HIGH_SPEED_HIGH_DRIVE (0x2 << RENESAS_GPIO_DS_POS)
/** High drive strength. */
#define RENESAS_GPIO_DS_HIGH_DRIVE            (0x3 << RENESAS_GPIO_DS_POS)
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RA_GPIO_IOPORT_H_ */
