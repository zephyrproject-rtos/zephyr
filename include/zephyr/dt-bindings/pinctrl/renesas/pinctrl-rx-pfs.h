/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RX pin function selector devicetree bindings
 *
 * This header provides pin control definitions for Renesas RX series
 * microcontrollers used in devicetree source files.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RX_H__
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RX_H__

/** @brief Port number bit position */
#define RX_PORT_NUM_POS  0
/** @brief Port number mask */
#define RX_PORT_NUM_MASK 0xf

/** @brief Pin number bit position */
#define RX_PIN_NUM_POS  5
/** @brief Pin number mask */
#define RX_PIN_NUM_MASK 0xf

/** @brief Pin function selector: high impedance, JTAG/SWD */
#define RX_PSEL_HIZ_JTAG_SWD 0x0
/** @brief Pin function selector: SCI channel 0 */
#define RX_PSEL_SCI_0        0xA
/** @brief Pin function selector: SCI channel 2 */
#define RX_PSEL_SCI_2        0xA
/** @brief Pin function selector: SCI channel 4 */
#define RX_PSEL_SCI_4        0xA
/** @brief Pin function selector: SCI channel 6 */
#define RX_PSEL_SCI_6        0xA
/** @brief Pin function selector: SCI channel 8 */
#define RX_PSEL_SCI_8        0xA
/** @brief Pin function selector: SCI channel 10 */
#define RX_PSEL_SCI_10       0xA
/** @brief Pin function selector: SCI channel 12 */
#define RX_PSEL_SCI_12       0xA
/** @brief Pin function selector: SCI channel 1 */
#define RX_PSEL_SCI_1        0xD
/** @brief Pin function selector: SCI channel 3 */
#define RX_PSEL_SCI_3        0xD
/** @brief Pin function selector: SCI channel 5 */
#define RX_PSEL_SCI_5        0xD
/** @brief Pin function selector: SCI channel 7 */
#define RX_PSEL_SCI_7        0xD
/** @brief Pin function selector: SCI channel 9 */
#define RX_PSEL_SCI_9        0xD
/** @brief Pin function selector: SCI channel 11 */
#define RX_PSEL_SCI_11       0xD

/** @brief Pin function selector bit position */
#define RX_PSEL_POS  9
/** @brief Pin function selector mask */
#define RX_PSEL_MASK 0x1f

/** @brief Mode bit position */
#define RX_MODE_POS  16
/** @brief Mode mask */
#define RX_MODE_MASK 0x1

/**
 * @brief Create a pin function selector configuration value
 *
 * @param psel Pin function selector value
 * @param port_num Port number
 * @param pin_num Pin number
 * @return Configured pin control value
 */
#define RX_PSEL(psel, port_num, pin_num)                                                           \
	(1 << RX_MODE_POS | psel << RX_PSEL_POS | port_num << RX_PORT_NUM_POS |                    \
	 pin_num << RX_PIN_NUM_POS)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RX_H__ */
