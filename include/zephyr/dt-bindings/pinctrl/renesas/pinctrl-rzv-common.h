/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree pin control helpers for Renesas RZ/V
 * @ingroup pinctrl_rzv
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZV_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZV_COMMON_H_

/**
 * @addtogroup renesas_pinctrl Renesas pin control helpers
 * @ingroup devicetree-pinctrl
 */

/**
 * @defgroup pinctrl_rzv Renesas RZ/V pin control helpers
 * @brief Macros for pin control configuration of Renesas RZ/V SoCs
 * @ingroup renesas_pinctrl
 *
 * General-purpose pins are configured with the RZV_PINMUX() macro, which takes
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
 * (@c BSP_IO_NMI, @c BSP_IO_TMS_SWDIO, @c BSP_IO_TDO), audio clocks, SD/MMC,
 * QSPI flash, RIIC (I2C), watchdog overflow and, on some variants, Ethernet and
 * PCIe reset.
 *
 * An optional digital noise filter can be applied to a pin group with
 * RZV_FILTER_SET(), built from a stage count (@c RZV_FILNUM_*) and a sampling
 * clock divider (@c RZV_FILCLKSEL_*).
 *
 * The pre-defined port/pin combinations differ between RZ/V variants and are
 * provided by a dedicated header per variant:
 *
 * - @c pinctrl-rzv-common.h — RZ/V general-purpose port superset
 * - @c pinctrl-rzv2h.h — RZ/V2H
 * - @c pinctrl-rzv2n.h — RZ/V2N
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-rzv-common.h>
 *
 * &pinctrl {
 *         scif0_default: scif0_default {
 *                 device-pinmux {
 *                         pinmux = <RZV_PINMUX(PORT_04, 0, 2)>,
 *                                  <RZV_PINMUX(PORT_04, 1, 2)>;
 *                         bias-pull-up;
 *                         renesas,filter =
 *                                 RZV_FILTER_SET(RZV_FILNUM_8_STAGE, RZV_FILCLKSEL_DIV_18000);
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
#define PORT_01 0x0100 /* IO port 1 */
#define PORT_02 0x0200 /* IO port 2 */
#define PORT_03 0x0300 /* IO port 3 */
#define PORT_04 0x0400 /* IO port 4 */
#define PORT_05 0x0500 /* IO port 5 */
#define PORT_06 0x0600 /* IO port 6 */
#define PORT_07 0x0700 /* IO port 7 */
#define PORT_08 0x0800 /* IO port 8 */
#define PORT_09 0x0900 /* IO port 9 */
#define PORT_10 0x0A00 /* IO port 10 */
#define PORT_11 0x0B00 /* IO port 11 */
#define PORT_12 0x0C00 /* IO port 12 */
#define PORT_13 0x0D00 /* IO port 13 */
#define PORT_14 0x0E00 /* IO port 14 */
#define PORT_15 0x0F00 /* IO port 15 */
#define PORT_16 0x1000 /* IO port 16 */
#define PORT_17 0x1100 /* IO port 17 */
#define PORT_18 0x1200 /* IO port 18 */
#define PORT_19 0x1300 /* IO port 19 */
#define PORT_20 0x1400 /* IO port 20 */
#define PORT_21 0x1500 /* IO port 21 */
#define PORT_22 0x1600 /* IO port 22 */
#define PORT_23 0x1700 /* IO port 23 */
#define PORT_24 0x1800 /* IO port 24 */
#define PORT_25 0x1900 /* IO port 25 */
#define PORT_26 0x1A00 /* IO port 26 */
#define PORT_27 0x1B00 /* IO port 27 */
#define PORT_28 0x1C00 /* IO port 28 */
#define PORT_29 0x1D00 /* IO port 29 */
#define PORT_30 0x1E00 /* IO port 30 */
#define PORT_31 0x1F00 /* IO port 31 */
#define PORT_32 0x2000 /* IO port 32 */
#define PORT_33 0x2100 /* IO port 33 */
#define PORT_34 0x2200 /* IO port 34 */
#define PORT_35 0x2300 /* IO port 35 */
#define PORT_36 0x2400 /* IO port 36 */
#define PORT_37 0x2500 /* IO port 37 */
#define PORT_38 0x2600 /* IO port 38 */
#define PORT_39 0x2700 /* IO port 39 */
#define PORT_40 0x2800 /* IO port 40 */
#define PORT_41 0x2900 /* IO port 41 */
#define PORT_42 0x2A00 /* IO port 42 */
#define PORT_43 0x2B00 /* IO port 43 */
#define PORT_44 0x2C00 /* IO port 44 */
#define PORT_45 0x2D00 /* IO port 45 */
#define PORT_46 0x2E00 /* IO port 46 */
#define PORT_47 0x2F00 /* IO port 47 */
#define PORT_48 0x3000 /* IO port 48 */

/** @endcond */

/**
 * @brief Create an encoded value containing port/pin/function information.
 *
 * @param port Port identifier (PORT_00..PORT_48).
 * @param pin Pin number within the port.
 * @param func Pin function selector.
 *
 * @return Encoded pinmux value.
 */
#define RZV_PINMUX(port, pin, func) (port | pin | (func << 4))

/** @cond INTERNAL_HIDDEN */

/* Special purpose port */
#define BSP_IO_NMI 0xFFFF0100 /* NMI */

#define BSP_IO_TMS_SWDIO 0xFFFF0200 /* TMS_SWDIO */

#define BSP_IO_TDO 0xFFFF0300 /* TDO */

#define BSP_IO_AUDIO_CLK1 0xFFFF0400 /* AUDIO_CLK1 */
#define BSP_IO_AUDIO_CLK2 0xFFFF0401 /* AUDIO_CLK2 */

#define BSP_IO_SD0_CLK   0xFFFF0600 /* SD0_CLK */
#define BSP_IO_SD0_CMD   0xFFFF0601 /* SD0_CMD */
#define BSP_IO_SD0_RST_N 0xFFFF0602 /* SD0_RST_N */

#define BSP_IO_SD0_DATA0 0xFFFF0700 /* SD0_DATA0 */
#define BSP_IO_SD0_DATA1 0xFFFF0701 /* SD0_DATA1 */
#define BSP_IO_SD0_DATA2 0xFFFF0702 /* SD0_DATA2 */
#define BSP_IO_SD0_DATA3 0xFFFF0703 /* SD0_DATA3 */
#define BSP_IO_SD0_DATA4 0xFFFF0704 /* SD0_DATA4 */
#define BSP_IO_SD0_DATA5 0xFFFF0705 /* SD0_DATA5 */
#define BSP_IO_SD0_DATA6 0xFFFF0706 /* SD0_DATA6 */
#define BSP_IO_SD0_DATA7 0xFFFF0707 /* SD0_DATA7 */

#define BSP_IO_SD1_CLK 0xFFFF0800 /* SD1_CLK */
#define BSP_IO_SD1_CMD 0xFFFF0801 /* SD1_CMD */

#define BSP_IO_SD1_DATA0 0xFFFF0900 /* SD1_DATA0 */
#define BSP_IO_SD1_DATA1 0xFFFF0901 /* SD1_DATA1 */
#define BSP_IO_SD1_DATA2 0xFFFF0902 /* SD1_DATA2 */
#define BSP_IO_SD1_DATA3 0xFFFF0903 /* SD1_DATA3 */

#define BSP_IO_QSPI0_SPCLK 0xFFFF0A00 /* QSPI0_SPCLK */
#define BSP_IO_QSPI0_IO0   0xFFFF0A01 /* QSPI0_IO0 */
#define BSP_IO_QSPI0_IO1   0xFFFF0A02 /* QSPI0_IO1 */
#define BSP_IO_QSPI0_IO2   0xFFFF0A03 /* QSPI0_IO2 */
#define BSP_IO_QSPI0_IO3   0xFFFF0A04 /* QSPI0_IO3 */
#define BSP_IO_QSPI0_SSL   0xFFFF0A05 /* QSPI0_SSL */

#define BSP_IO_QSPI1_SPCLK 0xFFFF0B00 /* QSPI1_SPCLK */
#define BSP_IO_QSPI1_IO0   0xFFFF0B01 /* QSPI1_IO0 */
#define BSP_IO_QSPI1_IO1   0xFFFF0B02 /* QSPI1_IO1 */
#define BSP_IO_QSPI1_IO2   0xFFFF0B03 /* QSPI1_IO2 */
#define BSP_IO_QSPI1_IO3   0xFFFF0B04 /* QSPI1_IO3 */
#define BSP_IO_QSPI1_SSL   0xFFFF0B05 /* QSPI1_SSL */

#define BSP_IO_QSPI_RESET_N 0xFFFF0C00 /* QSPI_RESET_N */
#define BSP_IO_QSPI_WP_N    0xFFFF0C01 /* QSPI_WP_N */
#define BSP_IO_QSPI_INT_N   0xFFFF0C02 /* QSPI_INT_N */

#define BSP_IO_WDTOVF_PERROUT_N 0xFFFF0D00 /* WDTOVF_PERROUT_N */

#define BSP_IO_RIIC0_SDA 0xFFFF0E00 /* RIIC0_SDA */
#define BSP_IO_RIIC0_SCL 0xFFFF0E01 /* RIIC0_SCL */
#define BSP_IO_RIIC1_SDA 0xFFFF0E02 /* RIIC1_SDA */
#define BSP_IO_RIIC1_SCL 0xFFFF0E03 /* RIIC1_SCL */

/** @endcond */

/**
 * @name Renesas RZ/V digital noise filter options
 *
 * Set the number of filter stages (FILNUM) and the sampling clock divider
 * (FILCLKSEL).
 * @{
 */

#define RZV_FILNUM_4_STAGE  0 /**< FILNUM: 4-stage filter */
#define RZV_FILNUM_8_STAGE  1 /**< FILNUM: 8-stage filter */
#define RZV_FILNUM_12_STAGE 2 /**< FILNUM: 12-stage filter */
#define RZV_FILNUM_16_STAGE 3 /**< FILNUM: 16-stage filter */

#define RZV_FILCLKSEL_NOT_DIV   0 /**< FILCLKSEL: no division */
#define RZV_FILCLKSEL_DIV_9000  1 /**< FILCLKSEL: divided by 9000 */
#define RZV_FILCLKSEL_DIV_18000 2 /**< FILCLKSEL: divided by 18000 */
#define RZV_FILCLKSEL_DIV_36000 3 /**< FILCLKSEL: divided by 36000 */

/** @} */

/**
 * @brief Encode filter stage count and clock selection into one configuration value.
 *
 * Encoding:
 * - bits [3:2] = FILNUM (masked to 2 bits)
 * - bits [1:0] = FILCLKSEL (masked to 2 bits)
 *
 * @param filnum Filter stage selection (RZV_FILNUM_*).
 * @param filclksel Filter clock selection (RZV_FILCLKSEL_*).
 *
 * @return Encoded filter configuration value.
 */
#define RZV_FILTER_SET(filnum, filclksel) (((filnum) & 0x3) << 0x2) | (filclksel & 0x3)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZV_COMMON_H_ */
