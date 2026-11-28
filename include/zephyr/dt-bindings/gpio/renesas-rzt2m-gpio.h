/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RZ/T2M-specific GPIO devicetree flags.
 * @ingroup gpio_interface_rzt2m
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZT2M_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZT2M_GPIO_H_
#include <zephyr/sys/util.h>

/**
 * @defgroup gpio_interface_rzt2m RZ/T2M-specific GPIO flags
 * @ingroup gpio_interface_ext
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define RZT2M_GPIO_DRIVE_OFFSET 8
#define RZT2M_GPIO_DRIVE_MASK   GENMASK(RZT2M_GPIO_DRIVE_OFFSET + 2, RZT2M_GPIO_DRIVE_OFFSET)
/** @endcond */

/**
 * @name GPIO pin drive strength
 * @{
 */
/** Low drive strength. */
#define RZT2M_GPIO_DRIVE_LOW        (0U << RZT2M_GPIO_DRIVE_OFFSET)
/** Middle drive strength. */
#define RZT2M_GPIO_DRIVE_MIDDLE     (1U << RZT2M_GPIO_DRIVE_OFFSET)
/** High drive strength. */
#define RZT2M_GPIO_DRIVE_HIGH       (2U << RZT2M_GPIO_DRIVE_OFFSET)
/** Ultra high drive strength. */
#define RZT2M_GPIO_DRIVE_ULTRA_HIGH (3U << RZT2M_GPIO_DRIVE_OFFSET)
/** @} */

/** @cond INTERNAL_HIDDEN */
#define RZT2M_GPIO_SCHMITT_TRIGGER_OFFSET 10
#define RZT2M_GPIO_SCHMITT_TRIGGER_MASK   BIT(RZT2M_GPIO_SCHMITT_TRIGGER_OFFSET)
/** @endcond */

/**
 * @brief Enable GPIO pin schmitt trigger
 */
#define RZT2M_GPIO_SCHMITT_TRIGGER BIT(RZT2M_GPIO_SCHMITT_TRIGGER_OFFSET)

/** @cond INTERNAL_HIDDEN */
#define RZT2M_GPIO_SLEW_RATE_OFFSET 11
#define RZT2M_GPIO_SLEW_RATE_MASK   BIT(RZT2M_GPIO_SLEW_RATE_OFFSET)
/** @endcond */

/**
 * @name GPIO pin slew rate
 * @{
 */
/** Slow slew rate. */
#define RZT2M_GPIO_SLEW_RATE_SLOW 0U
/** Fast slew rate. */
#define RZT2M_GPIO_SLEW_RATE_FAST BIT(RZT2M_GPIO_SLEW_RATE_OFFSET)
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZT2M_GPIO_H_ */
