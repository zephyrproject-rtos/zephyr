/*
 * Copyright (c) 2021 Gerson Fernando Budke <nandojve@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_BFLB_PINCTRL_H_
#define ZEPHYR_BFLB_PINCTRL_H_

/**
 * @brief FUN configuration bitfield
 *
 * The Zephyr version of Bouffalo Lab function field encode 3 information:
 *  - The pin function itself
 *  - The peripheral instance (ex. uart0, uart1, etc)
 *  - The pin mode
 *
 * Because GPIO is a subset of functions, it is necessary define all modes
 * when a pin is configured. To keep it simple, mode is already defined for all
 * alternate functions and analog. In the case of GPIO, the pincfg-node should
 * be used to configure GPIO function.
 *
 * Pin function configuration is coded using 2 bytes with the following fields
 *    Byte 0 - Function
 *    Byte 1 - Pin Mode             [ 0 : 1 ]
 *           - Peripheral Instance  [ 4 : 5 ]
 *
 * ex.:
 * uart1 - 0x1207
 *         1         2               07
 *         Instance  Alternate Mode  Function UART
 *
 * gpio  - 0x010b
 *         0         2               0b
 *         Instance  Output          Function GPIO
 */

#define BFLB_FUN_clk_out		0x0200
#define BFLB_FUN_bt_coexist		0x0201
#define BFLB_FUN_flash_psram		0x0202
#define BFLB_FUN_qspi			0x0202
#define BFLB_FUN_i2s			0x0203
#define BFLB_FUN_spi			0x0204
#define BFLB_FUN_i2c			0x0206
#define BFLB_FUN_uart0			0x0207
#define BFLB_FUN_uart1			0x1207
#define BFLB_FUN_pwm			0x0208
#define BFLB_FUN_cam			0x0209
#define BFLB_FUN_analog			0x030a
#define BFLB_FUN_gpio			0x000b
#define BFLB_FUN_rf_test		0x020c
#define BFLB_FUN_scan			0x020d
#define BFLB_FUN_jtag			0x020e
#define BFLB_FUN_debug			0x020f
#define BFLB_FUN_external_pa		0x0210
#define BFLB_FUN_usb_transceiver	0x0211
#define BFLB_FUN_usb_controller		0x0212
#define BFLB_FUN_ether_mac		0x0213
#define BFLB_FUN_emca			0x0213
#define BFLB_FUN_qdec			0x0014
#define BFLB_FUN_key_scan_in		0x0215
#define BFLB_FUN_key_scan_row		0x0215
#define BFLB_FUN_key_scan_drive		0x0216
#define BFLB_FUN_key_scan_col		0x0216
#define BFLB_FUN_cam_misc		0x0217
#define BFLB_FUN_FUNC_POS		0U
#define BFLB_FUN_FUNC_MASK		0x1F

#define BFLB_FUN_MODE_POS		8U
#define BFLB_FUN_MODE_MASK		0x03
#define BFLB_FUN_MODE_INPUT		(0x0 << BFLB_FUN_MODE_POS)
#define BFLB_FUN_MODE_OUTPUT		(0x1 << BFLB_FUN_MODE_POS)
#define BFLB_FUN_MODE_AF		(0x2 << BFLB_FUN_MODE_POS)
#define BFLB_FUN_MODE_ANALOG		(0x3 << BFLB_FUN_MODE_POS)

#define BFLB_FUN_INST_POS		12U
#define BFLB_FUN_INST_MASK		0x03
#define BFLB_FUN_INST_0			(0x0 << BFLB_FUN_INST_POS)
#define BFLB_FUN_INST_1			(0x1 << BFLB_FUN_INST_POS)

#define BFLB_SIG_UART_RTS		0x0
#define BFLB_SIG_UART_CTS		0x1
#define BFLB_SIG_UART_TXD		0x2
#define BFLB_SIG_UART_RXD		0x3
#define BFLB_SIG_UART_LEN		0x4
#define BFLB_SIG_UART_MASK		0x3

/**
 * @brief helper macro to encode an IO port pin in a numerical format
 *
 * - fun Function value. It should be lower case value defined by a
 *       BFLB_FUN_function.
 * - pin Pin number.
 *
 * ex.: How to define uart0 rx/tx pins, which is define as BFLB_FUN_uart0
 *
 *	bflb,pins = <BFLB_PIN(uart0,   7)>,
 *		    <BFLB_PIN(uart0,  16)>;
 */
#define BFLB_PIN(fun, pin) &glb BFLB_FUN_##fun pin

#define BFLB_FUN_2_FUNC(fun) (((fun) >> BFLB_FUN_FUNC_POS) & BFLB_FUN_FUNC_MASK)
#define BFLB_FUN_2_MODE(fun) (((fun) >> BFLB_FUN_MODE_POS) & BFLB_FUN_MODE_MASK)
#define BFLB_FUN_2_INST(fun) (((fun) >> BFLB_FUN_INST_POS) & BFLB_FUN_INST_MASK)

/*
 * Pin configuration is coded following below bit fields:
 *  - GPIO Mode             [ 0 : 1 ]
 *  - GPIO Schmitt Trigger  [ 2 ]
 *  - GPIO Drive Strength   [ 3 : 4 ]
 */

/* GPIO Pull-up/pull-down/High impedance */
#define BFLB_GPIO_MODE_POS		0U
#define BFLB_GPIO_MODE_MASK		0x3
#define BFLB_GPIO_MODE_HIGH_IMPEDANCE	(0x0 << BFLB_GPIO_MODE_POS)
#define BFLB_GPIO_MODE_PULL_UP		(0x1 << BFLB_GPIO_MODE_POS)
#define BFLB_GPIO_MODE_PULL_DOWN	(0x2 << BFLB_GPIO_MODE_POS)
#define BFLB_CFG_2_GPIO_MODE(cfg) (((cfg) >> BFLB_GPIO_MODE_POS) & BFLB_GPIO_MODE_MASK)

/* GPIO Input Schmitt trigger */
#define BFLB_GPIO_INP_SMT_POS		2U
#define BFLB_GPIO_INP_SMT_MASK		0x1
#define BFLB_GPIO_INP_SMT_EN		(0x1 << BFLB_GPIO_INP_SMT_POS)
#define BFLB_CFG_2_GPIO_INP_SMT(cfg) (((cfg) >> BFLB_GPIO_INP_SMT_POS) & BFLB_GPIO_INP_SMT_MASK)

/* GPIO Output Drive Strength */
#define BFLB_GPIO_DRV_STR_POS		3U
#define BFLB_GPIO_DRV_STR_MASK		0x3
#define BFLB_CFG_2_GPIO_DRV_STR(cfg) (((cfg) >> BFLB_GPIO_DRV_STR_POS) & BFLB_GPIO_DRV_STR_MASK)

#endif	/* ZEPHYR_BFLB_PINCTRL_H_ */
