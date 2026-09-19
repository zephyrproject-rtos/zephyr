/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_WUC_WUC_RENESAS_RA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_WUC_WUC_RENESAS_RA_H_

/**
 * @file
 * @ingroup wuc_interface
 * @brief Renesas RA WUC wakeup source encodings.
 */

/**
 * @brief WUC specifier encoding (single 32-bit cell).
 *
 *   bits [7:0]  - flat bit position in DPSIER/DPSIFR 64-bit mask
 *                (reg_index * 8 + bit_in_reg)
 *   bit  [8]    - edge select for IRQ pins
 *                0 = falling (default), 1 = rising
 *
 * Usage: wakeup-ctrls = <&wuc RA_WUC_DPSS_IRQ13_FALLING>;
 */

/** @name Edge encodings */
/** @{ */
/** Edge encoding for falling edge. */
#define RA_WUC_DPSS_FALLING  0
/** Edge encoding for rising edge. */
#define RA_WUC_DPSS_RISING   1
/** @} */

/** @name WUC specifier helpers */
/** @{ */
/** Encode a DPSIER/DPSIFR bit and edge selection. */
#define RA_WUC_DPSS_ID(bit, edge)  (((edge) << 8) | ((bit) & 0xFF))
/** Extract the flat bit position from an encoded specifier. */
#define RA_WUC_DPSS_GET_BIT(id)    ((id) & 0xFF)
/** Extract the edge selection from an encoded specifier. */
#define RA_WUC_DPSS_GET_EDGE(id)   (((id) >> 8) & 0x1)
/** @} */

/** @name DPSIER0 wakeup sources (IRQ0-IRQ7) */
/** @{ */
/** IRQ0 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ0_FALLING   RA_WUC_DPSS_ID(0,  RA_WUC_DPSS_FALLING)
/** IRQ0 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ0_RISING    RA_WUC_DPSS_ID(0,  RA_WUC_DPSS_RISING)
/** IRQ1 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ1_FALLING   RA_WUC_DPSS_ID(1,  RA_WUC_DPSS_FALLING)
/** IRQ1 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ1_RISING    RA_WUC_DPSS_ID(1,  RA_WUC_DPSS_RISING)
/** IRQ2 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ2_FALLING   RA_WUC_DPSS_ID(2,  RA_WUC_DPSS_FALLING)
/** IRQ2 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ2_RISING    RA_WUC_DPSS_ID(2,  RA_WUC_DPSS_RISING)
/** IRQ3 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ3_FALLING   RA_WUC_DPSS_ID(3,  RA_WUC_DPSS_FALLING)
/** IRQ3 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ3_RISING    RA_WUC_DPSS_ID(3,  RA_WUC_DPSS_RISING)
/** IRQ4 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ4_FALLING   RA_WUC_DPSS_ID(4,  RA_WUC_DPSS_FALLING)
/** IRQ4 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ4_RISING    RA_WUC_DPSS_ID(4,  RA_WUC_DPSS_RISING)
/** IRQ5 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ5_FALLING   RA_WUC_DPSS_ID(5,  RA_WUC_DPSS_FALLING)
/** IRQ5 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ5_RISING    RA_WUC_DPSS_ID(5,  RA_WUC_DPSS_RISING)
/** IRQ6 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ6_FALLING   RA_WUC_DPSS_ID(6,  RA_WUC_DPSS_FALLING)
/** IRQ6 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ6_RISING    RA_WUC_DPSS_ID(6,  RA_WUC_DPSS_RISING)
/** IRQ7 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ7_FALLING   RA_WUC_DPSS_ID(7,  RA_WUC_DPSS_FALLING)
/** IRQ7 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ7_RISING    RA_WUC_DPSS_ID(7,  RA_WUC_DPSS_RISING)
/** @} */

/** @name DPSIER1 wakeup sources (IRQ8-IRQ15) */
/** @{ */
/** IRQ8 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ8_FALLING   RA_WUC_DPSS_ID(8,  RA_WUC_DPSS_FALLING)
/** IRQ8 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ8_RISING    RA_WUC_DPSS_ID(8,  RA_WUC_DPSS_RISING)
/** IRQ9 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ9_FALLING   RA_WUC_DPSS_ID(9,  RA_WUC_DPSS_FALLING)
/** IRQ9 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ9_RISING    RA_WUC_DPSS_ID(9,  RA_WUC_DPSS_RISING)
/** IRQ10 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ10_FALLING  RA_WUC_DPSS_ID(10, RA_WUC_DPSS_FALLING)
/** IRQ10 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ10_RISING   RA_WUC_DPSS_ID(10, RA_WUC_DPSS_RISING)
/** IRQ11 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ11_FALLING  RA_WUC_DPSS_ID(11, RA_WUC_DPSS_FALLING)
/** IRQ11 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ11_RISING   RA_WUC_DPSS_ID(11, RA_WUC_DPSS_RISING)
/** IRQ12 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ12_FALLING  RA_WUC_DPSS_ID(12, RA_WUC_DPSS_FALLING)
/** IRQ12 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ12_RISING   RA_WUC_DPSS_ID(12, RA_WUC_DPSS_RISING)
/** IRQ13 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ13_FALLING  RA_WUC_DPSS_ID(13, RA_WUC_DPSS_FALLING)
/** IRQ13 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ13_RISING   RA_WUC_DPSS_ID(13, RA_WUC_DPSS_RISING)
/** IRQ14 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ14_FALLING  RA_WUC_DPSS_ID(14, RA_WUC_DPSS_FALLING)
/** IRQ14 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ14_RISING   RA_WUC_DPSS_ID(14, RA_WUC_DPSS_RISING)
/** IRQ15 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ15_FALLING  RA_WUC_DPSS_ID(15, RA_WUC_DPSS_FALLING)
/** IRQ15 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ15_RISING   RA_WUC_DPSS_ID(15, RA_WUC_DPSS_RISING)
/** @} */

/** @name DPSIER2 wakeup sources (LVD1, LVD2, RTC, NMI) */
/** @{ */
/** LVD1 wakeup source on falling edge. */
#define RA_WUC_DPSS_LVD1_FALLING      RA_WUC_DPSS_ID(16, RA_WUC_DPSS_FALLING)
/** LVD1 wakeup source on rising edge. */
#define RA_WUC_DPSS_LVD1_RISING       RA_WUC_DPSS_ID(16, RA_WUC_DPSS_RISING)
/** LVD2 wakeup source on falling edge. */
#define RA_WUC_DPSS_LVD2_FALLING      RA_WUC_DPSS_ID(17, RA_WUC_DPSS_FALLING)
/** LVD2 wakeup source on rising edge. */
#define RA_WUC_DPSS_LVD2_RISING       RA_WUC_DPSS_ID(17, RA_WUC_DPSS_RISING)
/** RTC interval wakeup source. */
#define RA_WUC_DPSS_RTC_INTERVAL      RA_WUC_DPSS_ID(18, RA_WUC_DPSS_FALLING)
/** RTC alarm wakeup source. */
#define RA_WUC_DPSS_RTC_ALARM         RA_WUC_DPSS_ID(19, RA_WUC_DPSS_FALLING)
/** NMI wakeup source on falling edge. */
#define RA_WUC_DPSS_NMI_FALLING       RA_WUC_DPSS_ID(20, RA_WUC_DPSS_FALLING)
/** NMI wakeup source on rising edge. */
#define RA_WUC_DPSS_NMI_RISING        RA_WUC_DPSS_ID(20, RA_WUC_DPSS_RISING)
/** @} */

/** @name DPSIER3 wakeup sources (USB, ULPT, IWDT, SOSTD, VBATT) */
/** @{ */
/** USBFS wakeup source. */
#define RA_WUC_DPSS_USBFS             RA_WUC_DPSS_ID(24, RA_WUC_DPSS_FALLING)
/** USBHS wakeup source. */
#define RA_WUC_DPSS_USBHS             RA_WUC_DPSS_ID(25, RA_WUC_DPSS_FALLING)
/** ULPT0 wakeup source. */
#define RA_WUC_DPSS_ULPT0             RA_WUC_DPSS_ID(26, RA_WUC_DPSS_FALLING)
/** ULPT1 wakeup source. */
#define RA_WUC_DPSS_ULPT1             RA_WUC_DPSS_ID(27, RA_WUC_DPSS_FALLING)
/** IWDT wakeup source. */
#define RA_WUC_DPSS_IWDT              RA_WUC_DPSS_ID(29, RA_WUC_DPSS_FALLING)
/** SOSTD wakeup source. */
#define RA_WUC_DPSS_SOSTD             RA_WUC_DPSS_ID(30, RA_WUC_DPSS_FALLING)
/** VBATT wakeup source. */
#define RA_WUC_DPSS_VBATT             RA_WUC_DPSS_ID(31, RA_WUC_DPSS_FALLING)
/** @} */

/** @name DPSIER4 wakeup sources (IRQ16-IRQ23) */
/** @{ */
/** IRQ16 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ16_FALLING  RA_WUC_DPSS_ID(32, RA_WUC_DPSS_FALLING)
/** IRQ16 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ16_RISING   RA_WUC_DPSS_ID(32, RA_WUC_DPSS_RISING)
/** IRQ17 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ17_FALLING  RA_WUC_DPSS_ID(33, RA_WUC_DPSS_FALLING)
/** IRQ17 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ17_RISING   RA_WUC_DPSS_ID(33, RA_WUC_DPSS_RISING)
/** IRQ18 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ18_FALLING  RA_WUC_DPSS_ID(34, RA_WUC_DPSS_FALLING)
/** IRQ18 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ18_RISING   RA_WUC_DPSS_ID(34, RA_WUC_DPSS_RISING)
/** IRQ19 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ19_FALLING  RA_WUC_DPSS_ID(35, RA_WUC_DPSS_FALLING)
/** IRQ19 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ19_RISING   RA_WUC_DPSS_ID(35, RA_WUC_DPSS_RISING)
/** IRQ20 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ20_FALLING  RA_WUC_DPSS_ID(36, RA_WUC_DPSS_FALLING)
/** IRQ20 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ20_RISING   RA_WUC_DPSS_ID(36, RA_WUC_DPSS_RISING)
/** IRQ21 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ21_FALLING  RA_WUC_DPSS_ID(37, RA_WUC_DPSS_FALLING)
/** IRQ21 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ21_RISING   RA_WUC_DPSS_ID(37, RA_WUC_DPSS_RISING)
/** IRQ22 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ22_FALLING  RA_WUC_DPSS_ID(38, RA_WUC_DPSS_FALLING)
/** IRQ22 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ22_RISING   RA_WUC_DPSS_ID(38, RA_WUC_DPSS_RISING)
/** IRQ23 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ23_FALLING  RA_WUC_DPSS_ID(39, RA_WUC_DPSS_FALLING)
/** IRQ23 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ23_RISING   RA_WUC_DPSS_ID(39, RA_WUC_DPSS_RISING)
/** @} */

/** @name DPSIER5 wakeup sources (IRQ24-IRQ31) */
/** @{ */
/** IRQ24 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ24_FALLING  RA_WUC_DPSS_ID(40, RA_WUC_DPSS_FALLING)
/** IRQ24 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ24_RISING   RA_WUC_DPSS_ID(40, RA_WUC_DPSS_RISING)
/** IRQ25 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ25_FALLING  RA_WUC_DPSS_ID(41, RA_WUC_DPSS_FALLING)
/** IRQ25 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ25_RISING   RA_WUC_DPSS_ID(41, RA_WUC_DPSS_RISING)
/** IRQ26 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ26_FALLING  RA_WUC_DPSS_ID(42, RA_WUC_DPSS_FALLING)
/** IRQ26 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ26_RISING   RA_WUC_DPSS_ID(42, RA_WUC_DPSS_RISING)
/** IRQ27 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ27_FALLING  RA_WUC_DPSS_ID(43, RA_WUC_DPSS_FALLING)
/** IRQ27 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ27_RISING   RA_WUC_DPSS_ID(43, RA_WUC_DPSS_RISING)
/** IRQ28 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ28_FALLING  RA_WUC_DPSS_ID(44, RA_WUC_DPSS_FALLING)
/** IRQ28 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ28_RISING   RA_WUC_DPSS_ID(44, RA_WUC_DPSS_RISING)
/** IRQ29 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ29_FALLING  RA_WUC_DPSS_ID(45, RA_WUC_DPSS_FALLING)
/** IRQ29 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ29_RISING   RA_WUC_DPSS_ID(45, RA_WUC_DPSS_RISING)
/** IRQ30 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ30_FALLING  RA_WUC_DPSS_ID(46, RA_WUC_DPSS_FALLING)
/** IRQ30 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ30_RISING   RA_WUC_DPSS_ID(46, RA_WUC_DPSS_RISING)
/** IRQ31 wakeup source on falling edge. */
#define RA_WUC_DPSS_IRQ31_FALLING  RA_WUC_DPSS_ID(47, RA_WUC_DPSS_FALLING)
/** IRQ31 wakeup source on rising edge. */
#define RA_WUC_DPSS_IRQ31_RISING   RA_WUC_DPSS_ID(47, RA_WUC_DPSS_RISING)
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_WUC_WUC_RENESAS_RA_H_ */
