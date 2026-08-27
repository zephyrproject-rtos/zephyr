/*
 * SPDX-FileCopyrightText: 2026 SMILE (smile.eu)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NOTE: This pin controller configuration header supports WCH CH570/CH572 SoCs only.
 */

/**
 * @file
 * @brief Pinctrl definitions for WCH CH570/CH572 SoCs only
 *
 * This header provides pin multiplexing definitions for WCH CH570/CH572 SoCs.
 * Each peripheral can be configured using multiple alternate functions (AFs).
 *
 * The pinmux encoding uses a 14-bit value:
 * - Bits [0:3]    - Target pin number (0-11)
 * - Bits [4:8]    - Peripheral remap field offset in the pin alternate register (0-16)
 * - Bits [9:10]   - Width of the alternate function field in the pin alternate register (0-3 bits)
 * - Bits [11:13]  - Alternate function value (0-7)
 *
 * Pin naming convention: {PERIPHERAL}_{FUNCTION}_{PIN}_{REMAP_NUMBER}.
 * Example: UART_TXD_PA10_7 represents the UART TXD function on pin PA10 using remap
 *          configuration 7.
 *
 * Note: Some functions do not have a remap configuration. For consistency, a macro is still defined
 *       so that it can be used in devicetree (e.g., to configure internal pull-ups). This occurs
 *       if and only if the field width is 0 and the field offset is 16.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_CH570_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_CH570_PINCTRL_H_

/**
 * @name Pinctrl pin functions bit field positions and masks.
 * @{
 */

/** Offset of the UART_RXD function field */
#define CH570_PINCTRL_UART_RXD       0
/** Offset of the UART_TXD function field */
#define CH570_PINCTRL_UART_TXD       3
/** Offset of the TMR function field */
#define CH570_PINCTRL_TMR            6
/** Offset of the SPI_CS function field */
#define CH570_PINCTRL_SPI_CS         8
/** Offset of the I2C_PIN function field */
#define CH570_PINCTRL_I2C_PIN        9
/** Offset of the SPI_CLK function field */
#define CH570_PINCTRL_SPI_CLK        11
/** Offset of the OSC_25M_ENABLE function field */
#define CH570_PINCTRL_OSC_25M_ENABLE 12
/** Offset of the no-alternate function configuration field (must be 16) */
#define CH570_PINCTRL_NO_ALTERNATE   16

/** Width of the UART_RXD alternate function field (8 remap options) */
#define CH570_PINCTRL_UART_RXD_ALT_WIDTH       3
/** Width of the UART_TXD alternate function field (8 remap options) */
#define CH570_PINCTRL_UART_TXD_ALT_WIDTH       3
/** Width of the TMR alternate function field (4 remap options) */
#define CH570_PINCTRL_TMR_ALT_WIDTH            2
/** Width of the SPI_CS alternate function field (2 remap options) */
#define CH570_PINCTRL_SPI_CS_ALT_WIDTH         1
/** Width of the I2C_PIN alternate function field (4 remap options) */
#define CH570_PINCTRL_I2C_PIN_ALT_WIDTH        2
/** Width of the SPI_CLK alternate function field (2 remap options) */
#define CH570_PINCTRL_SPI_CLK_ALT_WIDTH        1
/** Width of the OSC_25M_ENABLE alternate function field (2 remap options) */
#define CH570_PINCTRL_OSC_25M_ENABLE_ALT_WIDTH 1
/** Width of the no-alternate configuration (must absolutely be 0) */
#define CH570_PINCTRL_NO_ALTERNATE_ALT_WIDTH   0

/** @} */

/**
 * @name Pinmux encoding bit field definitions
 * @{
 */

/** Position of the pin number field in the pinmux encoded value */
#define CH570_PINCTRL_PIN_SHIFT 0
/** Mask for the pin number field in the pinmux encoded value */
#define CH570_PINCTRL_PIN_MASK  GENMASK(3, 0)

/** Position of the remap offset field in the pinmux encoded value */
#define CH570_PINCTRL_ALT_FIELD_OFFSET_SHIFT 4
/** Mask for the remap offset field in the pinmux encoded value */
#define CH570_PINCTRL_ALT_FIELD_OFFSET_MASK  GENMASK(8, 4)

/** Position of the alternate function field width in the pinmux encoded value */
#define CH570_PINCTRL_ALT_FIELD_WIDTH_SHIFT 9
/** Mask for the alternate function field width in the pinmux encoded value */
#define CH570_PINCTRL_ALT_FIELD_WIDTH_MASK  GENMASK(10, 9)

/** Position of the alternate function value in the pinmux encoded value */
#define CH570_PINCTRL_ALT_FUNC_SHIFT 11
/** Mask for the alternate function value in the pinmux encoded value */
#define CH570_PINCTRL_ALT_FUNC_MASK  GENMASK(13, 11)

/** @} */

/**
 * @brief Encode pinmux configuration
 *
 * @param pin Pin number (0-11)
 * @param peripheral Peripheral name (e.g., UART, TMR). It must match with a defined pinmux macros.
 * @param remapping Remap configuration number (maximum: 0-7)
 * @return Encoded pinmux value for use in devicetree
 */
#define CH570_PINMUX_DEFINE(pin, peripheral, remapping)                                            \
	((pin << CH570_PINCTRL_PIN_SHIFT) |                                                        \
	 (CH570_PINCTRL_##peripheral << CH570_PINCTRL_ALT_FIELD_OFFSET_SHIFT) |                    \
	 (CH570_PINCTRL_##peripheral##_ALT_WIDTH << CH570_PINCTRL_ALT_FIELD_WIDTH_SHIFT) |         \
	 (remapping << CH570_PINCTRL_ALT_FUNC_SHIFT))

/**
 * @name UART
 * @{
 */
/** UART TXD (no remap) */
#define UART_TXD_PA3_0  CH570_PINMUX_DEFINE(3, UART_TXD, 0)
/** UART TXD (remap 1) */
#define UART_TXD_PA2_1  CH570_PINMUX_DEFINE(2, UART_TXD, 1)
/** UART TXD (remap 2) */
#define UART_TXD_PA1_2  CH570_PINMUX_DEFINE(1, UART_TXD, 2)
/** UART TXD (remap 3) */
#define UART_TXD_PA0_3  CH570_PINMUX_DEFINE(0, UART_TXD, 3)
/** UART TXD (remap 4) */
#define UART_TXD_PA7_4  CH570_PINMUX_DEFINE(7, UART_TXD, 4)
/** UART TXD (remap 5) */
#define UART_TXD_PA8_5  CH570_PINMUX_DEFINE(8, UART_TXD, 5)
/** UART TXD (remap 6) */
#define UART_TXD_PA11_6 CH570_PINMUX_DEFINE(11, UART_TXD, 6)
/** UART TXD (remap 7) */
#define UART_TXD_PA10_7 CH570_PINMUX_DEFINE(10, UART_TXD, 7)

/*
 * Documentation is inconsistent for UART RXD_4. AF table lists UART RXD on PA4 (config 4),
 * while the pinout section shows PA6. PA6 is used in practice.
 */
/** UART RXD (no remap) */
#define UART_RXD_PA2_0  CH570_PINMUX_DEFINE(2, UART_RXD, 0)
/** UART RXD (remap 1) */
#define UART_RXD_PA3_1  CH570_PINMUX_DEFINE(3, UART_RXD, 1)
/** UART RXD (remap 2) */
#define UART_RXD_PA0_2  CH570_PINMUX_DEFINE(0, UART_RXD, 2)
/** UART RXD (remap 3) */
#define UART_RXD_PA1_3  CH570_PINMUX_DEFINE(1, UART_RXD, 3)
/** UART RXD (remap 4) */
#define UART_RXD_PA6_4  CH570_PINMUX_DEFINE(6, UART_RXD, 4)
/** UART RXD (remap 5) */
#define UART_RXD_PA9_5  CH570_PINMUX_DEFINE(9, UART_RXD, 5)
/** UART RXD (remap 6) */
#define UART_RXD_PA10_6 CH570_PINMUX_DEFINE(10, UART_RXD, 6)
/** UART RXD (remap 7) */
#define UART_RXD_PA11_7 CH570_PINMUX_DEFINE(11, UART_RXD, 7)
/** @} */

/**
 * @name TIMER (PWM0 or CAPIN (2-channels))
 * @{
 */
/** TMR first input (first capture input channel) or output (pwm) (no remap) */
#define TMR_PWM0_CAPIN1_PA7_0 CH570_PINMUX_DEFINE(7, TMR, 0)
/** TMR first input (capture) or output (tmr) (remap 1) */
#define TMR_PWM0_CAPIN1_PA2_1 CH570_PINMUX_DEFINE(2, TMR, 1)
/** TMR first input (capture) or output (tmr) (remap 2) */
#define TMR_PWM0_CAPIN1_PA4_2 CH570_PINMUX_DEFINE(4, TMR, 2)
/** TMR first input (capture) or output (tmr) (remap 3) */
#define TMR_PWM0_CAPIN1_PA9_3 CH570_PINMUX_DEFINE(9, TMR, 3)
/** TMR second capture input channel (no remap) */
#define TMR_CAPIN2_PA2_0      CH570_PINMUX_DEFINE(2, TMR, 0)
/** TMR second capture input channel (remap 1) */
#define TMR_CAPIN2_PA7_1      CH570_PINMUX_DEFINE(7, TMR, 1)
/** TMR second capture input channel (remap 2) */
#define TMR_CAPIN2_PA9_2      CH570_PINMUX_DEFINE(9, TMR, 2)
/** TMR second capture input channel (remap 3) */
#define TMR_CAPIN2_PA4_3      CH570_PINMUX_DEFINE(4, TMR, 3)
/** @} */

/**
 * @name SPI
 * @{
 */
/** SPI_CS (no remap) */
#define SPI_CS_PA4_0   CH570_PINMUX_DEFINE(4, SPI_CS, 0)
/** SPI_CS (remap) */
#define SPI_CS_PA2_1   CH570_PINMUX_DEFINE(2, SPI_CS, 1)
/** SPI_CLK (no remap) */
#define SPI_CLK_PA5_0  CH570_PINMUX_DEFINE(5, SPI_CLK, 0)
/** SPI_CLK (remap) */
#define SPI_CLK_PA3_1  CH570_PINMUX_DEFINE(3, SPI_CLK, 1)
/** SPI_MOSI (no remap) */
#define SPI_MOSI_PA7_0 CH570_PINMUX_DEFINE(7, NO_ALTERNATE, 0)
/** SPI_MISO (no remap) */
#define SPI_MISO_PA6_0 CH570_PINMUX_DEFINE(6, NO_ALTERNATE, 0)
/** @} */

/**
 * @name I2C
 * @{
 */
/** I2C_SCL (no remap) */
#define I2C_SCL_PA8_0 CH570_PINMUX_DEFINE(8, I2C_PIN, 0)
/** I2C_SCL (remap 1) */
#define I2C_SCL_PA0_1 CH570_PINMUX_DEFINE(0, I2C_PIN, 1)
/** I2C_SCL (remap 2) */
#define I2C_SCL_PA3_2 CH570_PINMUX_DEFINE(3, I2C_PIN, 2)
/** I2C_SCL (remap 3) */
#define I2C_SCL_PA5_3 CH570_PINMUX_DEFINE(5, I2C_PIN, 3)
/** I2C_SDA (no remap) */
#define I2C_SDA_PA9_0 CH570_PINMUX_DEFINE(9, I2C_PIN, 0)
/** I2C_SDA (remap 1) */
#define I2C_SDA_PA1_1 CH570_PINMUX_DEFINE(1, I2C_PIN, 1)
/** I2C_SDA (remap 2) */
#define I2C_SDA_PA2_2 CH570_PINMUX_DEFINE(2, I2C_PIN, 2)
/** I2C_SDA (remap 3) */
#define I2C_SDA_PA6_3 CH570_PINMUX_DEFINE(6, I2C_PIN, 3)
/** @} */

/**
 * @name Keyscan
 * @{
 */
/** Keyscan input 0 (no alternate function) */
#define KEYSCAN_0_PA2_0  CH570_PINMUX_DEFINE(2, NO_ALTERNATE, 0)
/** Keyscan input 1 (no alternate function) */
#define KEYSCAN_1_PA3_0  CH570_PINMUX_DEFINE(3, NO_ALTERNATE, 0)
/** Keyscan input 2 (no alternate function) */
#define KEYSCAN_2_PA8_0  CH570_PINMUX_DEFINE(8, NO_ALTERNATE, 0)
/** Keyscan input 3 (no alternate function) */
#define KEYSCAN_3_PA10_0 CH570_PINMUX_DEFINE(10, NO_ALTERNATE, 0)
/** Keyscan input 4 (no alternate function) */
#define KEYSCAN_4_PA11_0 CH570_PINMUX_DEFINE(11, NO_ALTERNATE, 0)
/** @} */

/**
 * @name CMP
 * @{
 */
/** CMP positive input (no alternate function) */
#define CMP_P0_PA3_0 CH570_PINMUX_DEFINE(3, NO_ALTERNATE, 0)
/** CMP positive input (PA7 selected via CMP CTRL register instead of PA3) */
#define CMP_P1_PA7_0 CH570_PINMUX_DEFINE(7, NO_ALTERNATE, 0)
/** CMP negative input (no alternate function) */
#define CMP_N_PA2_0  CH570_PINMUX_DEFINE(2, NO_ALTERNATE, 0)
/** @} */

/**
 * @name PWM
 * @{
 */
/** PWM output 1 (no alternate function) */
#define PWM1_PA7_0 CH570_PINMUX_DEFINE(7, NO_ALTERNATE, 0)
/** PWM output 2 (no alternate function) */
#define PWM2_PA2_0 CH570_PINMUX_DEFINE(2, NO_ALTERNATE, 0)
/** PWM output 3 (no alternate function) */
#define PWM3_PA3_0 CH570_PINMUX_DEFINE(3, NO_ALTERNATE, 0)
/** PWM output 4 (no alternate function) */
#define PWM4_PA4_0 CH570_PINMUX_DEFINE(4, NO_ALTERNATE, 0)
/** PWM output 5 (no alternate function) */
#define PWM5_PA8_0 CH570_PINMUX_DEFINE(8, NO_ALTERNATE, 0)
/** @} */

/**
 * @name OSC
 * @{
 */
/** Oscillator 25MHz output */
#define OSC25M_ENABLE_PA4_0 CH570_PINMUX_DEFINE(4, OSC_25M_ENABLE, 1)
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_CH570_PINCTRL_H_ */
