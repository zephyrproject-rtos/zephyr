/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NXP LPC84x pinctrl definitions.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_LPC84X_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_LPC84X_PINCTRL_H_

/**
 * Encode movable SWM function with pin value.
 *
 * @param func Switch matrix function number.
 * @param pin Physical port pin number.
 */

/**< Movable pinmux encoder */

#define LPC84X_PINMUX_M(func, pin) (((func) << 16) | (pin))

/**
 * Encode fixed SWM function with pin value.
 *
 * @param func Switch matrix function number.
 * @param pin Physical port pin number.
 */

/**< Fixed pinmux encoder */

#define LPC84X_PINMUX_F(func, pin) ((1U << 23) | ((func) << 16) | (pin))

/** SWM movable function indices. */
#define LPC84X_SWM_USART0_TXD    0U  /**< USART0 TXD */
#define LPC84X_SWM_USART0_RXD    1U  /**< USART0 RXD */
#define LPC84X_SWM_USART0_RTS    2U  /**< USART0 RTS */
#define LPC84X_SWM_USART0_CTS    3U  /**< USART0 CTS */
#define LPC84X_SWM_USART0_SCLK   4U  /**< USART0 SCLK */
#define LPC84X_SWM_USART1_TXD    5U  /**< USART1 TXD */
#define LPC84X_SWM_USART1_RXD    6U  /**< USART1 RXD */
#define LPC84X_SWM_USART1_RTS    7U  /**< USART1 RTS */
#define LPC84X_SWM_USART1_CTS    8U  /**< USART1 CTS */
#define LPC84X_SWM_USART1_SCLK   9U  /**< USART1 SCLK */
#define LPC84X_SWM_USART2_TXD    10U /**< USART2 TXD */
#define LPC84X_SWM_USART2_RXD    11U /**< USART2 RXD */
#define LPC84X_SWM_USART2_RTS    12U /**< USART2 RTS */
#define LPC84X_SWM_USART2_CTS    13U /**< USART2 CTS */
#define LPC84X_SWM_USART2_SCLK   14U /**< USART2 SCLK */
#define LPC84X_SWM_SPI0_SCK      15U /**< SPI0 SCK */
#define LPC84X_SWM_SPI0_MOSI     16U /**< SPI0 MOSI */
#define LPC84X_SWM_SPI0_MISO     17U /**< SPI0 MISO */
#define LPC84X_SWM_SPI0_SSEL0    18U /**< SPI0 SSEL0 */
#define LPC84X_SWM_SPI0_SSEL1    19U /**< SPI0 SSEL1 */
#define LPC84X_SWM_SPI0_SSEL2    20U /**< SPI0 SSEL2 */
#define LPC84X_SWM_SPI0_SSEL3    21U /**< SPI0 SSEL3 */
#define LPC84X_SWM_SPI1_SCK      22U /**< SPI1 SCK */
#define LPC84X_SWM_SPI1_MOSI     23U /**< SPI1 MOSI */
#define LPC84X_SWM_SPI1_MISO     24U /**< SPI1 MISO */
#define LPC84X_SWM_SPI1_SSEL0    25U /**< SPI1 SSEL0 */
#define LPC84X_SWM_SPI1_SSEL1    26U /**< SPI1 SSEL1 */
#define LPC84X_SWM_SCT_PIN0      27U /**< SCT PIN0 */
#define LPC84X_SWM_SCT_PIN1      28U /**< SCT PIN1 */
#define LPC84X_SWM_SCT_PIN2      29U /**< SCT PIN2 */
#define LPC84X_SWM_SCT_PIN3      30U /**< SCT PIN3 */
#define LPC84X_SWM_SCT_OUT0      31U /**< SCT OUT0 */
#define LPC84X_SWM_SCT_OUT1      32U /**< SCT OUT1 */
#define LPC84X_SWM_SCT_OUT2      33U /**< SCT OUT2 */
#define LPC84X_SWM_SCT_OUT3      34U /**< SCT OUT3 */
#define LPC84X_SWM_SCT_OUT4      35U /**< SCT OUT4 */
#define LPC84X_SWM_SCT_OUT5      36U /**< SCT OUT5 */
#define LPC84X_SWM_SCT_OUT6      37U /**< SCT OUT6 */
#define LPC84X_SWM_I2C1_SDA      38U /**< I2C1 SDA */
#define LPC84X_SWM_I2C1_SCL      39U /**< I2C1 SCL */
#define LPC84X_SWM_I2C2_SDA      40U /**< I2C2 SDA */
#define LPC84X_SWM_I2C2_SCL      41U /**< I2C2 SCL */
#define LPC84X_SWM_I2C3_SDA      42U /**< I2C3 SDA */
#define LPC84X_SWM_I2C3_SCL      43U /**< I2C3 SCL */
#define LPC84X_SWM_ACMP_OUT      44U /**< ACMP OUT */
#define LPC84X_SWM_CLKOUT        45U /**< CLKOUT */
#define LPC84X_SWM_GPIO_INT_BMAT 46U /**< GPIO INT BMAT */
#define LPC84X_SWM_USART3_TXD    47U /**< USART3 TXD */
#define LPC84X_SWM_USART3_RXD    48U /**< USART3 RXD */
#define LPC84X_SWM_USART3_SCLK   49U /**< USART3 SCLK */
#define LPC84X_SWM_USART4_TXD    50U /**< USART4 TXD */
#define LPC84X_SWM_USART4_RXD    51U /**< USART4 RXD */
#define LPC84X_SWM_USART4_SCLK   52U /**< USART4 SCLK */
#define LPC84X_SWM_T0_MAT_CHN0   53U /**< T0 MAT CHN0 */
#define LPC84X_SWM_T0_MAT_CHN1   54U /**< T0 MAT CHN1 */
#define LPC84X_SWM_T0_MAT_CHN2   55U /**< T0 MAT CHN2 */
#define LPC84X_SWM_T0_MAT_CHN3   56U /**< T0 MAT CHN3 */
#define LPC84X_SWM_T0_CAP_CHN0   57U /**< T0 CAP CHN0 */
#define LPC84X_SWM_T0_CAP_CHN1   58U /**< T0 CAP CHN1 */
#define LPC84X_SWM_T0_CAP_CHN2   59U /**< T0 CAP CHN2 */

/**
 * Physical port pin identifiers used by SWM.
 * Port 0 pins range from 0–31.
 * Port 1 pins continue sequentially.
 */

/* Port 0 */
#define LPC84X_PIN_P0_0  0U  /**< P0.0 */
#define LPC84X_PIN_P0_1  1U  /**< P0.1 */
#define LPC84X_PIN_P0_2  2U  /**< P0.2 */
#define LPC84X_PIN_P0_3  3U  /**< P0.3 */
#define LPC84X_PIN_P0_4  4U  /**< P0.4 */
#define LPC84X_PIN_P0_5  5U  /**< P0.5 */
#define LPC84X_PIN_P0_6  6U  /**< P0.6 */
#define LPC84X_PIN_P0_7  7U  /**< P0.7 */
#define LPC84X_PIN_P0_8  8U  /**< P0.8 */
#define LPC84X_PIN_P0_9  9U  /**< P0.9 */
#define LPC84X_PIN_P0_10 10U /**< P0.10 */
#define LPC84X_PIN_P0_11 11U /**< P0.11 */
#define LPC84X_PIN_P0_12 12U /**< P0.12 */
#define LPC84X_PIN_P0_13 13U /**< P0.13 */
#define LPC84X_PIN_P0_14 14U /**< P0.14 */
#define LPC84X_PIN_P0_15 15U /**< P0.15 */
#define LPC84X_PIN_P0_16 16U /**< P0.16 */
#define LPC84X_PIN_P0_17 17U /**< P0.17 */
#define LPC84X_PIN_P0_18 18U /**< P0.18 */
#define LPC84X_PIN_P0_19 19U /**< P0.19 */
#define LPC84X_PIN_P0_20 20U /**< P0.20 */
#define LPC84X_PIN_P0_21 21U /**< P0.21 */
#define LPC84X_PIN_P0_22 22U /**< P0.22 */
#define LPC84X_PIN_P0_23 23U /**< P0.23 */
#define LPC84X_PIN_P0_24 24U /**< P0.24 */
#define LPC84X_PIN_P0_25 25U /**< P0.25 */
#define LPC84X_PIN_P0_26 26U /**< P0.26 */
#define LPC84X_PIN_P0_27 27U /**< P0.27 */
#define LPC84X_PIN_P0_28 28U /**< P0.28 */
#define LPC84X_PIN_P0_29 29U /**< P0.29 */
#define LPC84X_PIN_P0_30 30U /**< P0.30 */
#define LPC84X_PIN_P0_31 31U /**< P0.31 */

/* Port 1 */
#define LPC84X_PIN_P1_0  32U /**< P1.0 */
#define LPC84X_PIN_P1_1  33U /**< P1.1 */
#define LPC84X_PIN_P1_2  34U /**< P1.2 */
#define LPC84X_PIN_P1_3  35U /**< P1.3 */
#define LPC84X_PIN_P1_4  36U /**< P1.4 */
#define LPC84X_PIN_P1_5  37U /**< P1.5 */
#define LPC84X_PIN_P1_6  38U /**< P1.6 */
#define LPC84X_PIN_P1_7  39U /**< P1.7 */
#define LPC84X_PIN_P1_8  40U /**< P1.8 */
#define LPC84X_PIN_P1_9  41U /**< P1.9 */
#define LPC84X_PIN_P1_10 42U /**< P1.10 */
#define LPC84X_PIN_P1_11 43U /**< P1.11 */
#define LPC84X_PIN_P1_12 44U /**< P1.12 */
#define LPC84X_PIN_P1_13 45U /**< P1.13 */
#define LPC84X_PIN_P1_14 46U /**< P1.14 */
#define LPC84X_PIN_P1_15 47U /**< P1.15 */
#define LPC84X_PIN_P1_16 48U /**< P1.16 */
#define LPC84X_PIN_P1_17 49U /**< P1.17 */
#define LPC84X_PIN_P1_18 50U /**< P1.18 */
#define LPC84X_PIN_P1_19 51U /**< P1.19 */
#define LPC84X_PIN_P1_20 52U /**< P1.20 */
#define LPC84X_PIN_P1_21 53U /**< P1.21 */

/**
 * IOCON register indices for each PIO pin.
 * The numbering reflects hardware IOCON register ordering.
 * It does not strictly follow port/pin numeric order.
 */
#define IOCON_INDEX_PIO0_17 0U  /**< PIO0_17 IOCON index */
#define IOCON_INDEX_PIO0_13 1U  /**< PIO0_13 IOCON index */
#define IOCON_INDEX_PIO0_12 2U  /**< PIO0_12 IOCON index */
#define IOCON_INDEX_PIO0_5  3U  /**< PIO0_5 IOCON index */
#define IOCON_INDEX_PIO0_4  4U  /**< PIO0_4 IOCON index */
#define IOCON_INDEX_PIO0_3  5U  /**< PIO0_3 IOCON index */
#define IOCON_INDEX_PIO0_2  6U  /**< PIO0_2 IOCON index */
#define IOCON_INDEX_PIO0_11 7U  /**< PIO0_11 IOCON index */
#define IOCON_INDEX_PIO0_10 8U  /**< PIO0_10 IOCON index */
#define IOCON_INDEX_PIO0_16 9U  /**< PIO0_16 IOCON index */
#define IOCON_INDEX_PIO0_15 10U /**< PIO0_15 IOCON index */
#define IOCON_INDEX_PIO0_1  11U /**< PIO0_1 IOCON index */
#define IOCON_INDEX_PIO0_9  13U /**< PIO0_9 IOCON index */
#define IOCON_INDEX_PIO0_8  14U /**< PIO0_8 IOCON index */
#define IOCON_INDEX_PIO0_7  15U /**< PIO0_7 IOCON index */
#define IOCON_INDEX_PIO0_6  16U /**< PIO0_6 IOCON index */
#define IOCON_INDEX_PIO0_0  17U /**< PIO0_0 IOCON index */
#define IOCON_INDEX_PIO0_14 18U /**< PIO0_14 IOCON index */
#define IOCON_INDEX_PIO0_28 20U /**< PIO0_28 IOCON index */
#define IOCON_INDEX_PIO0_27 21U /**< PIO0_27 IOCON index */
#define IOCON_INDEX_PIO0_26 22U /**< PIO0_26 IOCON index */
#define IOCON_INDEX_PIO0_25 23U /**< PIO0_25 IOCON index */
#define IOCON_INDEX_PIO0_24 24U /**< PIO0_24 IOCON index */
#define IOCON_INDEX_PIO0_23 25U /**< PIO0_23 IOCON index */
#define IOCON_INDEX_PIO0_22 26U /**< PIO0_22 IOCON index */
#define IOCON_INDEX_PIO0_21 27U /**< PIO0_21 IOCON index */
#define IOCON_INDEX_PIO0_20 28U /**< PIO0_20 IOCON index */
#define IOCON_INDEX_PIO0_19 29U /**< PIO0_19 IOCON index */
#define IOCON_INDEX_PIO0_18 30U /**< PIO0_18 IOCON index */
#define IOCON_INDEX_PIO1_8  31U /**< PIO1_8 IOCON index */
#define IOCON_INDEX_PIO1_9  32U /**< PIO1_9 IOCON index */
#define IOCON_INDEX_PIO1_12 33U /**< PIO1_12 IOCON index */
#define IOCON_INDEX_PIO1_13 34U /**< PIO1_13 IOCON index */
#define IOCON_INDEX_PIO0_31 35U /**< PIO0_31 IOCON index */
#define IOCON_INDEX_PIO1_0  36U /**< PIO1_0 IOCON index */
#define IOCON_INDEX_PIO1_1  37U /**< PIO1_1 IOCON index */
#define IOCON_INDEX_PIO1_2  38U /**< PIO1_2 IOCON index */
#define IOCON_INDEX_PIO1_14 39U /**< PIO1_14 IOCON index */
#define IOCON_INDEX_PIO1_15 40U /**< PIO1_15 IOCON index */
#define IOCON_INDEX_PIO1_3  41U /**< PIO1_3 IOCON index */
#define IOCON_INDEX_PIO1_4  42U /**< PIO1_4 IOCON index */
#define IOCON_INDEX_PIO1_5  43U /**< PIO1_5 IOCON index */
#define IOCON_INDEX_PIO1_16 44U /**< PIO1_16 IOCON index */
#define IOCON_INDEX_PIO1_17 45U /**< PIO1_17 IOCON index */
#define IOCON_INDEX_PIO1_6  46U /**< PIO1_6 IOCON index */
#define IOCON_INDEX_PIO1_18 47U /**< PIO1_18 IOCON index */
#define IOCON_INDEX_PIO1_19 48U /**< PIO1_19 IOCON index */
#define IOCON_INDEX_PIO1_7  49U /**< PIO1_7 IOCON index */
#define IOCON_INDEX_PIO0_29 50U /**< PIO0_29 IOCON index */
#define IOCON_INDEX_PIO0_30 51U /**< PIO0_30 IOCON index */
#define IOCON_INDEX_PIO1_20 52U /**< PIO1_20 IOCON index */
#define IOCON_INDEX_PIO1_21 53U /**< PIO1_21 IOCON index */
#define IOCON_INDEX_PIO1_11 54U /**< PIO1_11 IOCON index */
#define IOCON_INDEX_PIO1_10 55U /**< PIO1_10 IOCON index */

/**
 * Encode SWM function and IOCON index into a single 16-bit value.
 * @param swm SWM function or port pin identifier.
 * @param iocon IOCON register index.
 */

/* < Pin define encoder */
#define LPC84X_PIN_DEFINE(swm, iocon) ((((swm) & 0xFF) << 8) | ((iocon) & 0xFF))

/**
 * Encoded pin definitions combining physical port pin identifier
 * and corresponding IOCON index.
 *
 * Intended for use in Devicetree pinctrl nodes.
 */

#define P0_0  LPC84X_PIN_DEFINE(LPC84X_PIN_P0_0, IOCON_INDEX_PIO0_0)   /**< P0_0 */
#define P0_1  LPC84X_PIN_DEFINE(LPC84X_PIN_P0_1, IOCON_INDEX_PIO0_1)   /**< P0_1 */
#define P0_2  LPC84X_PIN_DEFINE(LPC84X_PIN_P0_2, IOCON_INDEX_PIO0_2)   /**< P0_2 */
#define P0_3  LPC84X_PIN_DEFINE(LPC84X_PIN_P0_3, IOCON_INDEX_PIO0_3)   /**< P0_3 */
#define P0_4  LPC84X_PIN_DEFINE(LPC84X_PIN_P0_4, IOCON_INDEX_PIO0_4)   /**< P0_4 */
#define P0_5  LPC84X_PIN_DEFINE(LPC84X_PIN_P0_5, IOCON_INDEX_PIO0_5)   /**< P0_5 */
#define P0_6  LPC84X_PIN_DEFINE(LPC84X_PIN_P0_6, IOCON_INDEX_PIO0_6)   /**< P0_6 */
#define P0_7  LPC84X_PIN_DEFINE(LPC84X_PIN_P0_7, IOCON_INDEX_PIO0_7)   /**< P0_7 */
#define P0_8  LPC84X_PIN_DEFINE(LPC84X_PIN_P0_8, IOCON_INDEX_PIO0_8)   /**< P0_8 */
#define P0_9  LPC84X_PIN_DEFINE(LPC84X_PIN_P0_9, IOCON_INDEX_PIO0_9)   /**< P0_9 */
#define P0_10 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_10, IOCON_INDEX_PIO0_10) /**< P0_10 */
#define P0_11 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_11, IOCON_INDEX_PIO0_11) /**< P0_11 */
#define P0_12 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_12, IOCON_INDEX_PIO0_12) /**< P0_12 */
#define P0_13 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_13, IOCON_INDEX_PIO0_13) /**< P0_13 */
#define P0_14 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_14, IOCON_INDEX_PIO0_14) /**< P0_14 */
#define P0_15 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_15, IOCON_INDEX_PIO0_15) /**< P0_15 */
#define P0_16 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_16, IOCON_INDEX_PIO0_16) /**< P0_16 */
#define P0_17 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_17, IOCON_INDEX_PIO0_17) /**< P0_17 */
#define P0_18 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_18, IOCON_INDEX_PIO0_18) /**< P0_18 */
#define P0_19 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_19, IOCON_INDEX_PIO0_19) /**< P0_19 */
#define P0_20 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_20, IOCON_INDEX_PIO0_20) /**< P0_20 */
#define P0_21 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_21, IOCON_INDEX_PIO0_21) /**< P0_21 */
#define P0_22 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_22, IOCON_INDEX_PIO0_22) /**< P0_22 */
#define P0_23 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_23, IOCON_INDEX_PIO0_23) /**< P0_23 */
#define P0_24 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_24, IOCON_INDEX_PIO0_24) /**< P0_24 */
#define P0_25 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_25, IOCON_INDEX_PIO0_25) /**< P0_25 */
#define P0_26 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_26, IOCON_INDEX_PIO0_26) /**< P0_26 */
#define P0_27 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_27, IOCON_INDEX_PIO0_27) /**< P0_27 */
#define P0_28 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_28, IOCON_INDEX_PIO0_28) /**< P0_28 */
#define P0_29 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_29, IOCON_INDEX_PIO0_29) /**< P0_29 */
#define P0_30 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_30, IOCON_INDEX_PIO0_30) /**< P0_30 */
#define P0_31 LPC84X_PIN_DEFINE(LPC84X_PIN_P0_31, IOCON_INDEX_PIO0_31) /**< P0_31 */
#define P1_0  LPC84X_PIN_DEFINE(LPC84X_PIN_P1_0, IOCON_INDEX_PIO1_0)   /**< P1_0 */
#define P1_1  LPC84X_PIN_DEFINE(LPC84X_PIN_P1_1, IOCON_INDEX_PIO1_1)   /**< P1_1 */
#define P1_2  LPC84X_PIN_DEFINE(LPC84X_PIN_P1_2, IOCON_INDEX_PIO1_2)   /**< P1_2 */
#define P1_3  LPC84X_PIN_DEFINE(LPC84X_PIN_P1_3, IOCON_INDEX_PIO1_3)   /**< P1_3 */
#define P1_4  LPC84X_PIN_DEFINE(LPC84X_PIN_P1_4, IOCON_INDEX_PIO1_4)   /**< P1_4 */
#define P1_5  LPC84X_PIN_DEFINE(LPC84X_PIN_P1_5, IOCON_INDEX_PIO1_5)   /**< P1_5 */
#define P1_6  LPC84X_PIN_DEFINE(LPC84X_PIN_P1_6, IOCON_INDEX_PIO1_6)   /**< P1_6 */
#define P1_7  LPC84X_PIN_DEFINE(LPC84X_PIN_P1_7, IOCON_INDEX_PIO1_7)   /**< P1_7 */
#define P1_8  LPC84X_PIN_DEFINE(LPC84X_PIN_P1_8, IOCON_INDEX_PIO1_8)   /**< P1_8 */
#define P1_9  LPC84X_PIN_DEFINE(LPC84X_PIN_P1_9, IOCON_INDEX_PIO1_9)   /**< P1_9 */
#define P1_10 LPC84X_PIN_DEFINE(LPC84X_PIN_P1_10, IOCON_INDEX_PIO1_10) /**< P1_10 */
#define P1_11 LPC84X_PIN_DEFINE(LPC84X_PIN_P1_11, IOCON_INDEX_PIO1_11) /**< P1_11 */
#define P1_12 LPC84X_PIN_DEFINE(LPC84X_PIN_P1_12, IOCON_INDEX_PIO1_12) /**< P1_12 */
#define P1_13 LPC84X_PIN_DEFINE(LPC84X_PIN_P1_13, IOCON_INDEX_PIO1_13) /**< P1_13 */
#define P1_14 LPC84X_PIN_DEFINE(LPC84X_PIN_P1_14, IOCON_INDEX_PIO1_14) /**< P1_14 */
#define P1_15 LPC84X_PIN_DEFINE(LPC84X_PIN_P1_15, IOCON_INDEX_PIO1_15) /**< P1_15 */
#define P1_16 LPC84X_PIN_DEFINE(LPC84X_PIN_P1_16, IOCON_INDEX_PIO1_16) /**< P1_16 */
#define P1_17 LPC84X_PIN_DEFINE(LPC84X_PIN_P1_17, IOCON_INDEX_PIO1_17) /**< P1_17 */
#define P1_18 LPC84X_PIN_DEFINE(LPC84X_PIN_P1_18, IOCON_INDEX_PIO1_18) /**< P1_18 */
#define P1_19 LPC84X_PIN_DEFINE(LPC84X_PIN_P1_19, IOCON_INDEX_PIO1_19) /**< P1_19 */
#define P1_20 LPC84X_PIN_DEFINE(LPC84X_PIN_P1_20, IOCON_INDEX_PIO1_20) /**< P1_20 */
#define P1_21 LPC84X_PIN_DEFINE(LPC84X_PIN_P1_21, IOCON_INDEX_PIO1_21) /**< P1_21 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_LPC84X_PINCTRL_H_ */
