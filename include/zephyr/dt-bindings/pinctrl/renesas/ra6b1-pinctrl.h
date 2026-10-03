/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree pin control helpers for Renesas RA6B1
 * @ingroup pinctrl_ra6b1
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RA6B1_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RA6B1_PINCTRL_H_

/**
 * @defgroup pinctrl_ra6b1 Renesas RA6B1 pin control helpers
 * @brief Macros for pin control configuration of Renesas RA6B1 SoCs
 * @ingroup devicetree-pinctrl
 * @{
 */

/** Pin function selectors. */
#define RA6B1_FUNC_GPIO           0
#define RA6B1_FUNC_UART_RX        1
#define RA6B1_FUNC_UART_TX        2
#define RA6B1_FUNC_UART2_RX       3
#define RA6B1_FUNC_UART2_TX       4
#define RA6B1_FUNC_UART2_CTSN     5
#define RA6B1_FUNC_UART2_RTSN     6
#define RA6B1_FUNC_UART2_IRDA_RX  7
#define RA6B1_FUNC_UART2_IRDA_TX  8
#define RA6B1_FUNC_UART3_RX       9
#define RA6B1_FUNC_UART3_TX       10
#define RA6B1_FUNC_UART3_CTSN     11
#define RA6B1_FUNC_UART3_RTSN     12
#define RA6B1_FUNC_UART4_RX       13
#define RA6B1_FUNC_UART4_TX       14
#define RA6B1_FUNC_UART4_CTSN     15
#define RA6B1_FUNC_UART4_RTSN     16
#define RA6B1_FUNC_ISO7816_CLK    17
#define RA6B1_FUNC_ISO7816_DATA   18
#define RA6B1_FUNC_SPI_DI         19
#define RA6B1_FUNC_SPI_DO         20
#define RA6B1_FUNC_SPI_CLK        21
#define RA6B1_FUNC_SPI_CS         22
#define RA6B1_FUNC_SPI_CS2        23
#define RA6B1_FUNC_SPI2_DI        24
#define RA6B1_FUNC_SPI2_DO        25
#define RA6B1_FUNC_SPI2_CLK       26
#define RA6B1_FUNC_SPI2_CS        27
#define RA6B1_FUNC_SPI2_CS2       28
#define RA6B1_FUNC_SPI3_DI        29
#define RA6B1_FUNC_SPI3_DO        30
#define RA6B1_FUNC_SPI3_CLK       31
#define RA6B1_FUNC_SPI3_CS        32
#define RA6B1_FUNC_SPI3_CS2       33
#define RA6B1_FUNC_I2C_SCL        34
#define RA6B1_FUNC_I2C_SDA        35
#define RA6B1_FUNC_I2C2_SCL       36
#define RA6B1_FUNC_I2C2_SDA       37
#define RA6B1_FUNC_I2C3_SCL       38
#define RA6B1_FUNC_I2C3_SDA       39
#define RA6B1_FUNC_I3C_SCL        40
#define RA6B1_FUNC_I3C_SDA        41
#define RA6B1_FUNC_USB_SOF        42
#define RA6B1_FUNC_CAN_RX         43
#define RA6B1_FUNC_CAN_TX         44
#define RA6B1_FUNC_ADC            45
#define RA6B1_FUNC_USB            46
#define RA6B1_FUNC_IRGEN          47
#define RA6B1_FUNC_KBSCN_ROW      48
#define RA6B1_FUNC_PCM_DI         49
#define RA6B1_FUNC_PCM_DO         50
#define RA6B1_FUNC_PCM_FSC        51
#define RA6B1_FUNC_PCM_CLK        52
#define RA6B1_FUNC_PDM_DATA       53
#define RA6B1_FUNC_PDM_CLK        54
#define RA6B1_FUNC_TIM_PWMX       55
#define RA6B1_FUNC_TIMX_1SHOT     56
#define RA6B1_FUNC_CLOCK          57
#define RA6B1_FUNC_COEX_EXT_ACT   58
#define RA6B1_FUNC_COEX_SMART_ACT 59
#define RA6B1_FUNC_COEX_SMART_PRI 60
#define RA6B1_FUNC_RF_DIAG        61
#define RA6B1_FUNC_RFFE_SCLK      62
#define RA6B1_FUNC_RFFE_SDATA     63

/** Bit position of the pin number in a pinmux cell. */
#define RA6B1_PINMUX_PIN_POS   0
/** Bit mask for the pin number. */
#define RA6B1_PINMUX_PIN_MASK  0x1f
/** Bit position of the port number in a pinmux cell. */
#define RA6B1_PINMUX_PORT_POS  6
/** Bit mask for the port number. */
#define RA6B1_PINMUX_PORT_MASK 0x3
/** Bit position of the function selector in a pinmux cell. */
#define RA6B1_PINMUX_FUNC_POS  8
/** Bit mask for the function selector. */
#define RA6B1_PINMUX_FUNC_MASK 0x3f

/**
 * @brief Encode a function, port, and pin into a pinmux cell.
 *
 * @param func Pin function name without the @c RA6B1_FUNC_ prefix.
 * @param port Port number.
 * @param pin Pin number within the port.
 *
 * @return Encoded pinmux value.
 */
#define RAFW_PINMUX(func, port, pin)                                                         \
	(((RA6B1_FUNC_##func) << RA6B1_PINMUX_FUNC_POS) |                                    \
	 ((port) << RA6B1_PINMUX_PORT_POS) | (pin) << RA6B1_PINMUX_PIN_POS)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RA6B1_PINCTRL_H_ */
