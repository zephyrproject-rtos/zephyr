/*
 * SPDX-FileCopyrightText: 2026 SMILE (smile.eu)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_CH570_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_CH570_CLOCK_H_

/**
 * @file
 * @brief Clock ID configuration constants for WCH CH570/CH572 device tree bindings.
 *
 * @note This clock controller configuration header supports WCH CH570/CH572 SoCs only.
 */

/** First SLP clock control register number */
#define CH570_SLP_CLK_REG0 0
/** Second SLP clock control register number */
#define CH570_SLP_CLK_REG1 1
/** Third SLP clock control register number */
#define CH570_SLP_CLK_REG2 2

/**
 * Clock configuration macro that encodes the following information into a single byte:
 * - Bits [4:3]: sleep clock register number (0-2)
 * - Bits [2:0]: bit index in the sleep clock register (0-7)
 */
#define CH570_CLOCK_CONFIG(reg, bit) (((CH570_SLP_CLK_##reg) << 3) | (bit))

/** Extract the SLP clock control register from a clock configuration ID */
#define CH570_GET_REG_NUMBER(id) (((id) >> 3) & 0x3)

/** Extract the clock control bit index from a clock configuration ID */
#define CH570_GET_BIT_IDX(id) ((id) & 0x7)

/** TMR clock control configuration */
#define CH570_CLOCK_TMR    CH570_CLOCK_CONFIG(REG0, 0)
/** CMP clock control configuration */
#define CH570_CLOCK_CMP    CH570_CLOCK_CONFIG(REG0, 1)
/** UART clock control configuration */
#define CH570_CLOCK_UART   CH570_CLOCK_CONFIG(REG0, 4)
/** SPI clock control configurations */
#define CH570_CLOCK_SPI    CH570_CLOCK_CONFIG(REG1, 0)
/** AES/CCM clock control configuration */
#define CH570_CLOCK_AESCCM CH570_CLOCK_CONFIG(REG1, 1)
/** PWM clock control configuration */
#define CH570_CLOCK_PWM    CH570_CLOCK_CONFIG(REG1, 2)
/** I2C clock control configuration */
#define CH570_CLOCK_I2C    CH570_CLOCK_CONFIG(REG1, 3)
/** USB clock control configuration */
#define CH570_CLOCK_USB    CH570_CLOCK_CONFIG(REG1, 4)
/** BLE clock control configuration (CH572 only) */
#define CH570_CLOCK_BLE    CH570_CLOCK_CONFIG(REG1, 7)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_CH570_CLOCK_H_ */
