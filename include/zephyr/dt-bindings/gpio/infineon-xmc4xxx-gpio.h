/*
 * Copyright (c) 2022 Schlumberger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Infineon XMC4xxx-specific GPIO devicetree flags.
 * @ingroup gpio_interface_xmc4xxx
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_INFINEON_XMC4XXX_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_INFINEON_XMC4XXX_GPIO_H_

/**
 * @defgroup gpio_interface_xmc4xxx XMC4xxx-specific GPIO flags
 * @ingroup gpio_interface_ext
 *
 * The output driver strength is encoded in bits 9-12 of @ref gpio_dt_flags_t and only applies
 * to outputs. Without one of these flags, the strong driver with medium edge is used.
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define XMC4XXX_GPIO_DS_POS                   9
#define XMC4XXX_GPIO_DS_MASK                  0xf
/** @endcond */

/**
 * @name Output driver strength
 * @{
 */
/** Strong driver, sharp edge. */
#define XMC4XXX_GPIO_DS_STRONG_SHARP_EDGE    (0x1 << XMC4XXX_GPIO_DS_POS)
/** Strong driver, medium edge. */
#define XMC4XXX_GPIO_DS_STRONG_MEDIUM_EDGE   (0x2 << XMC4XXX_GPIO_DS_POS)
/** Strong driver, soft edge. */
#define XMC4XXX_GPIO_DS_STRONG_SOFT_EDGE     (0x3 << XMC4XXX_GPIO_DS_POS)
/** Strong driver, slow edge. */
#define XMC4XXX_GPIO_DS_STRONG_SLOW_EDGE     (0x4 << XMC4XXX_GPIO_DS_POS)
/** Medium driver. */
#define XMC4XXX_GPIO_DS_MEDIUM               (0x5 << XMC4XXX_GPIO_DS_POS)
/* values 5, 6 not set in xmc4_gpio.h */
/** Weak driver. */
#define XMC4XXX_GPIO_DS_WEAK                 (0x8 << XMC4XXX_GPIO_DS_POS)
/** @} */

/** @cond INTERNAL_HIDDEN */
#define XMC4XXX_GPIO_GET_DS(flags)           ((flags >> XMC4XXX_GPIO_DS_POS) & XMC4XXX_GPIO_DS_MASK)
/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_INFINEON_XMC4XXX_GPIO_H_ */
