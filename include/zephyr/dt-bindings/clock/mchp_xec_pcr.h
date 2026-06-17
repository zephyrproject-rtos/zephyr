/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_xec_pcr.h
 * @brief Clock IDs for the Microchip MEC SoC series
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCHP_XEC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCHP_XEC_H_

/** @brief PLL 32KHz clock source for full on */
#define MCHP_XEC_PLL_CLK32K_SRC_SIL_OSC 0U
#define MCHP_XEC_PLL_CLK32K_SRC_XTAL    1U
#define MCHP_XEC_PLL_CLK32K_SRC_PIN     2U

/** @brief Peripheral 32KHz clock source for full on and VBAT operation */
#define MCHP_XEC_PERIPH_CLK32K_SRC_SO_SO     0U
#define MCHP_XEC_PERIPH_CLK32K_SRC_XTAL_XTAL 1U
#define MCHP_XEC_PERIPH_CLK32K_SRC_PIN_SO    2U
#define MCHP_XEC_PERIPH_CLK32K_SRC_PIN_XTAL  3U

/** clock IDs for clock control driver */
/** @brief Core clock */
#define MCHP_XEC_PCR_CLK_CORE        0
/** @brief CPU clock */
#define MCHP_XEC_PCR_CLK_CPU         1
/** @brief AHB clock */
#define MCHP_XEC_PCR_CLK_BUS         2
/** @brief Clock for normal peripherals */
#define MCHP_XEC_PCR_CLK_PERIPH      3
/** @brief Clock for fast peripherals */
#define MCHP_XEC_PCR_CLK_PERIPH_FAST 4
/** @brief Peripheral slow clock */
#define MCHP_XEC_PCR_CLK_PERIPH_SLOW 5
/** @brief Clock derived from external crystal */
#define MCHP_XEC_PCR_CLK_XTAL        6
/** @brief I3C controllers input clock for SoC with I3C */
#define MCHP_XEC_PCR_CLK_I3C         7

/** @brief Encode a peripheral's sleep-en/clock-req/reset-en index and bit position
 *
 * @param idx zero based index of PCR SCR register. Range is 0 through 4.
 * @param bitpos position in the 32-bit PCR SCR register
 */
#define MCHP_XEC_SCR_ENCODE(idx, bitpos) (((idx) & 0x7) | (((bitpos) & 0x1f) << 3))

/** @brief Encode a peripheral's sleep-en/clock-req/reset-en index and bit position
 *
 * @param idx zero based index of PCR SCR register. Range is 0 through 4.
 * @param bitpos position in the 32-bit PCR SCR register
 */
#define MCHP_XEC_SCR_ENCODE(idx, bitpos) (((idx) & 0x7) | (((bitpos) & 0x1f) << 3))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCHP_XEC_H_ */
