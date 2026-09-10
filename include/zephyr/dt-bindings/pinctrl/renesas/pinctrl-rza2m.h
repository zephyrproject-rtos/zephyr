/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree pin control helpers for Renesas RZ/A2M
 * @ingroup pinctrl_rza2m
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA2M_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA2M_H_

/**
 * @addtogroup renesas_pinctrl Renesas pin control helpers
 * @ingroup devicetree-pinctrl
 */

/**
 * @defgroup pinctrl_rza2m Renesas RZ/A2M pin control helpers
 * @brief Macros for pin control configuration of the Renesas RZ/A2M SoC
 * @ingroup renesas_pinctrl
 *
 * Each pin configuration is built with the RZA2M_PINMUX() macro, which takes a
 * port, the pin number within that port, and the alternate-function number.
 * Ports are named as in the hardware manual: @c PORT_00 ... @c PORT_09,
 * @c PORT_A ... @c PORT_M, plus @c PORT_CKIO and @c PORT_PPOC.
 *
 * RZ/A2M uses an older pinmux scheme than the other RZ/A SoCs: the
 * alternate-function index is encoded in the upper bits and taken from the SoC's
 * pin-function table (there is no symbolic macro for it).
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-rza2m.h>
 *
 * &pinctrl {
 *         scifa4_default: scifa4_default {
 *                 device-pinmux {
 *                         pinmux = <RZA2M_PINMUX(PORT_09, 0, 4)>,
 *                                  <RZA2M_PINMUX(PORT_09, 1, 4)>;
 *                 };
 *         };
 * };
 * @endcode
 *
 * @{
 */

/** Number of pins per port. */
#define RZA2M_PIN_NUM_IN_PORT 8

/** @cond INTERNAL_HIDDEN */

/* Port names as labeled in the Hardware Manual */
#define PORT_00 0
#define PORT_01 1
#define PORT_02 2
#define PORT_03 3
#define PORT_04 4
#define PORT_05 5
#define PORT_06 6
#define PORT_07 7
#define PORT_08 8
#define PORT_09 9
#define PORT_A  10
#define PORT_B  11
#define PORT_C  12
#define PORT_D  13
#define PORT_E  14
#define PORT_F  15
#define PORT_G  16
#define PORT_H  17
/* No I */
#define PORT_J  18
#define PORT_K  19
#define PORT_L  20
#define PORT_M  21 /* Pins PM_0/1 are labeled JP_0/1 in HW manual */

#define PORT_CKIO 22
#define PORT_PPOC 23 /* Select between 1.8V and 3.3V for SPI and SD/MMC */

#define PIN_POSEL 0 /* Sets function for POSEL0 bits. 00, 01, 10 - 1.8v, 11 - 3.3v */
#define PIN_POC2  1 /* Sets function for SSD host 0, 0 - 1.8v 1 - 3.3v */
#define PIN_POC3  2 /* Sets function for SSD host 1, 0 - 1.8v 1 - 3.3v */

/** @endcond */

/**
 * @brief Create an encoded pinmux value from a port, pin and function.
 *
 * The pin index is built from the port and pin position numbers; the alternate
 * function identifier is stored in the upper 16 bits.
 *
 * @param b Port identifier (PORT_00..PORT_M, plus PORT_CKIO / PORT_PPOC).
 * @param p Pin number within the port.
 * @param f Alternate function identifier.
 *
 * @return Encoded pinmux value.
 */
#define RZA2M_PINMUX(b, p, f) ((b) * RZA2M_PIN_NUM_IN_PORT + (p) | (f << 16))

/** @cond INTERNAL_HIDDEN */
#define CKIO_DRV RZA2M_PINMUX(PORT_CKIO, 0, 0)
/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA2M_H_ */
