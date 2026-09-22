/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Musca-B1 pinctrl pin function macros
 *
 * This file contains macros that can be used to select alternative functions
 * for the Musca-B1 GPIO pins using the board's pinctrl driver.
 */

/** Bitmask for selecting ALTF1 on Musca-B1. */
#define MUSCA_B1_ALT_FUNC_MASK 0x1

/** Bit position at which to apply bitmask for selecting ALTF1 on Musca-B1. */
#define MUSCA_B1_ALT_FUNC_POS 0

/** Bitmask for the pin number in a Musca-B1 pinmux value. */
#define MUSCA_B1_EXP_NUM_MASK 0xF

/** Bit position of the pin number in a Musca-B1 pinmux value. */
#define MUSCA_B1_EXP_NUM_POS  4

/**
 * The Musca-B1 GPIO only has one alternate function: this macro specifies that bit 'exp_num'
 * should use the alternate function.
 */
#define MUSCA_B1_PINMUX(exp_num) (exp_num << MUSCA_B1_EXP_NUM_POS | 1 << MUSCA_B1_ALT_FUNC_POS)

/**
 * @name Musca-B1 pinctrl pin functions
 * @{
 */

#define UART0_RXD     MUSCA_B1_PINMUX(0)  /**< UART0 RxD */
#define UART0_TXD     MUSCA_B1_PINMUX(1)  /**< UART0 TxD */
#define MR_I2S_SD     MUSCA_B1_PINMUX(2)  /**< I2S receiver SD */
#define MR_I2S_WS     MUSCA_B1_PINMUX(3)  /**< I2S receiver WS */
#define MR_I2S_SCK    MUSCA_B1_PINMUX(4)  /**< I2S receiver SCK */
#define MT_I2S_SD0    MUSCA_B1_PINMUX(6)  /**< I2S transmitter SD0 */
#define MT_I2S_WS0    MUSCA_B1_PINMUX(5)  /**< I2S transmitter WS0 */
#define MT_I2S_SD1    MUSCA_B1_PINMUX(7)  /**< I2S transmitter SD1 */
#define MT_I2S_WS1    MUSCA_B1_PINMUX(8)  /**< I2S transmitter WS1 */
#define MT_I2S_SCK    MUSCA_B1_PINMUX(9)  /**< I2S transmitter SCK */
#define SPIO_NSS0     MUSCA_B1_PINMUX(10) /**< SPIO nSS0 */
#define SPIO_MOSI     MUSCA_B1_PINMUX(11) /**< SPIO MOSI */
#define SPIO_MISO     MUSCA_B1_PINMUX(12) /**< SPIO MISO */
#define SPIO_SCK      MUSCA_B1_PINMUX(13) /**< SPIO SCK */
#define I2C0_DATA     MUSCA_B1_PINMUX(14) /**< I2C0 data */
#define I2C0_CLOCK    MUSCA_B1_PINMUX(15) /**< I2C0 clock */

/** @} */
