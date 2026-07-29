/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree pin control helpers for Renesas RZ/G (RZ/G3S)
 * @ingroup pinctrl_rzg
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG_COMMON_H_

/**
 * @addtogroup renesas_pinctrl Renesas pin control helpers
 * @ingroup devicetree-pinctrl
 */

/**
 * @defgroup pinctrl_rzg Renesas RZ/G pin control helpers
 * @brief Macros for pin control configuration of Renesas RZ/G SoCs
 * @ingroup renesas_pinctrl
 *
 * General-purpose pins are configured with the RZG_PINMUX() macro, which takes
 * three fields:
 *
 * - @c port — the port identifier, one of the @c PORT_* values.
 * - @c pin — the pin number within that port.
 * - @c func — the alternate-function number selecting the peripheral signal
 *   routed to the pin, taken from the SoC's Pin Function Controller table; it
 *   has no symbolic macro and is passed as a plain integer.
 *
 * Dedicated-function pins that sit outside the general-purpose port matrix are
 * referenced directly through their @c BSP_IO_* identifiers, covering debug
 * (@c BSP_IO_NMI, @c BSP_IO_TMS_SWDIO, @c BSP_IO_TDO), audio clocks, XSPI flash,
 * I3C, SD/MMC and watchdog overflow.
 *
 * An optional digital noise filter can be applied to a pin group with
 * RZG_FILTER_SET(), built from a stage count (@c RZG_FILNUM_*) and a sampling
 * clock divider (@c RZG_FILCLKSEL_*).
 *
 * The pre-defined port/pin combinations differ between RZ/G variants and are
 * provided by a dedicated header per variant:
 *
 * - @c pinctrl-rzg-common.h — RZ/G3S
 * - @c pinctrl-rzg2-common.h — RZ/G2L, RZ/G2LC, RZ/G2UL
 * - @c pinctrl-rzg3e.h — RZ/G3E
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-rzg-common.h>
 *
 * &pinctrl {
 *         scif0_default: scif0_default {
 *                 device-pinmux {
 *                         pinmux = <RZG_PINMUX(PORT_08, 1, 5)>,
 *                                  <RZG_PINMUX(PORT_08, 2, 5)>;
 *                         bias-pull-up;
 *                         renesas,filter =
 *                                 RZG_FILTER_SET(RZG_FILNUM_8_STAGE, RZG_FILCLKSEL_DIV_18000);
 *                 };
 *         };
 * };
 * @endcode
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/* Superset list of all possible IO ports. */
#define PORT_00 0x0000 /* IO port 0 */
#define PORT_01 0x1000 /* IO port 1 */
#define PORT_02 0x1100 /* IO port 2 */
#define PORT_03 0x1200 /* IO port 3 */
#define PORT_04 0x1300 /* IO port 4 */
#define PORT_05 0x0100 /* IO port 5 */
#define PORT_06 0x0200 /* IO port 6 */
#define PORT_07 0x1400 /* IO port 7 */
#define PORT_08 0x1500 /* IO port 8 */
#define PORT_09 0x1600 /* IO port 9 */
#define PORT_10 0x1700 /* IO port 10 */
#define PORT_11 0x0300 /* IO port 11 */
#define PORT_12 0x0400 /* IO port 12 */
#define PORT_13 0x0500 /* IO port 13 */
#define PORT_14 0x0600 /* IO port 14 */
#define PORT_15 0x0700 /* IO port 15 */
#define PORT_16 0x0800 /* IO port 16 */
#define PORT_17 0x0900 /* IO port 17 */
#define PORT_18 0x0A00 /* IO port 18 */

/** @endcond */

/**
 * @brief Create an encoded value containing port/pin/function information.
 *
 * @param port Port identifier (PORT_00..PORT_18).
 * @param pin Pin number within the port.
 * @param func Pin function selector.
 *
 * @return Encoded pinmux value.
 */
#define RZG_PINMUX(port, pin, func) (port | pin | (func << 4))

/** @cond INTERNAL_HIDDEN */

/* Special purpose port */
#define BSP_IO_NMI 0xFFFF0000 /* NMI */

#define BSP_IO_TMS_SWDIO 0xFFFF0100 /* TMS_SWDIO */
#define BSP_IO_TDO       0xFFFF0101 /* TDO */

#define BSP_IO_AUDIO_CLK1 0xFFFF0200 /* AUDIO_CLK1 */
#define BSP_IO_AUDIO_CLK2 0xFFFF0201 /* AUDIO_CLK2 */

#define BSP_IO_XSPI_SPCLK   0xFFFF0400 /* XSPI_SPCLK */
#define BSP_IO_XSPI_RESET_N 0xFFFF0401 /* XSPI_RESET_N */
#define BSP_IO_XSPI_WP_N    0xFFFF0402 /* XSPI_WP_N */
#define BSP_IO_XSPI_DS      0xFFFF0403 /* XSPI_DS */
#define BSP_IO_XSPI_CS0_N   0xFFFF0404 /* XSPI_CS0_N */
#define BSP_IO_XSPI_CS1_N   0xFFFF0405 /* XSPI_CS1_N */

#define BSP_IO_XSPI_IO0 0xFFFF0500 /* XSPI_IO0 */
#define BSP_IO_XSPI_IO1 0xFFFF0501 /* XSPI_IO1 */
#define BSP_IO_XSPI_IO2 0xFFFF0502 /* XSPI_IO2 */
#define BSP_IO_XSPI_IO3 0xFFFF0503 /* XSPI_IO3 */
#define BSP_IO_XSPI_IO4 0xFFFF0504 /* XSPI_IO4 */
#define BSP_IO_XSPI_IO5 0xFFFF0505 /* XSPI_IO5 */
#define BSP_IO_XSPI_IO6 0xFFFF0506 /* XSPI_IO6 */
#define BSP_IO_XSPI_IO7 0xFFFF0507 /* XSPI_IO7 */

#define BSP_IO_WDTOVF_PERROUT 0xFFFF0600 /* WDTOVF_PERROUT */

#define BSP_IO_I3C_SDA 0xFFFF0900 /* I3C_SDA */
#define BSP_IO_I3C_SCL 0xFFFF0901 /* I3C_SCL */

#define BSP_IO_SD0_CLK   0xFFFF1000 /* CD0_CLK */
#define BSP_IO_SD0_CMD   0xFFFF1001 /* CD0_CMD */
#define BSP_IO_SD0_RST_N 0xFFFF1002 /* CD0_RST_N */

#define BSP_IO_SD0_DATA0 0xFFFF1100 /* SD0_DATA0 */
#define BSP_IO_SD0_DATA1 0xFFFF1101 /* SD0_DATA1 */
#define BSP_IO_SD0_DATA2 0xFFFF1102 /* SD0_DATA2 */
#define BSP_IO_SD0_DATA3 0xFFFF1103 /* SD0_DATA3 */
#define BSP_IO_SD0_DATA4 0xFFFF1104 /* SD0_DATA4 */
#define BSP_IO_SD0_DATA5 0xFFFF1105 /* SD0_DATA5 */
#define BSP_IO_SD0_DATA6 0xFFFF1106 /* SD0_DATA6 */
#define BSP_IO_SD0_DATA7 0xFFFF1107 /* SD0_DATA7 */

#define BSP_IO_SD1_CLK 0xFFFF1200 /* SD1_CLK */
#define BSP_IO_SD1_CMD 0xFFFF1201 /* SD1_CMD */

#define BSP_IO_SD1_DATA0 0xFFFF1300 /* SD1_DATA0 */
#define BSP_IO_SD1_DATA1 0xFFFF1301 /* SD1_DATA1 */
#define BSP_IO_SD1_DATA2 0xFFFF1302 /* SD1_DATA2 */
#define BSP_IO_SD1_DATA3 0xFFFF1303 /* SD1_DATA3 */

/** @endcond */

/**
 * @name Renesas RZ/G digital noise filter options
 *
 * Set the number of filter stages (FILNUM) and the sampling clock divider
 * (FILCLKSEL).
 * @{
 */

#define RZG_FILNUM_4_STAGE  0 /**< FILNUM: 4-stage filter */
#define RZG_FILNUM_8_STAGE  1 /**< FILNUM: 8-stage filter */
#define RZG_FILNUM_12_STAGE 2 /**< FILNUM: 12-stage filter */
#define RZG_FILNUM_16_STAGE 3 /**< FILNUM: 16-stage filter */

#define RZG_FILCLKSEL_NOT_DIV   0 /**< FILCLKSEL: no division */
#define RZG_FILCLKSEL_DIV_9000  1 /**< FILCLKSEL: divided by 9000 */
#define RZG_FILCLKSEL_DIV_18000 2 /**< FILCLKSEL: divided by 18000 */
#define RZG_FILCLKSEL_DIV_36000 3 /**< FILCLKSEL: divided by 36000 */

/** @} */

/**
 * @brief Encode filter stage count and clock selection into one configuration value.
 *
 * Encoding:
 * - bits [3:2] = FILNUM (masked to 2 bits)
 * - bits [1:0] = FILCLKSEL (masked to 2 bits)
 *
 * @param filnum Filter stage selection (RZG_FILNUM_*).
 * @param filclksel Filter clock selection (RZG_FILCLKSEL_*).
 *
 * @return Encoded filter configuration value.
 */
#define RZG_FILTER_SET(filnum, filclksel) (((filnum) & 0x3) << 0x2) | (filclksel & 0x3)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG_COMMON_H_ */
