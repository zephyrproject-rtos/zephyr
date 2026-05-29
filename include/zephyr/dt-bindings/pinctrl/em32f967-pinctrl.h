/*
 * Copyright (c) 2024 ELAN Microelectronics Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_EM32F967_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_EM32F967_PINCTRL_H_

/**
 * @brief EM32F967 Pin Control Definitions (STM32-style)
 *
 * This header defines the pin control macros for the EM32F967 microcontroller
 * following the STM32 pinctrl design pattern.
 * Based on EM32F967_Complete_Specification_v1.0.md
 */

/* Pin multiplexing values for EM32F967 - based on CSV specification */
#define EM32F967_GPIO 0 /* 000: GPIO function */
#define EM32F967_AF1  1 /* 001: Alternate function 1 (SPI1, etc.) */
#define EM32F967_AF2  2 /* 010: Alternate function 2 (UART, etc.) */
#define EM32F967_AF3  3 /* 011: Alternate function 3 (Timer, 7816, etc.) */
#define EM32F967_AF4  4 /* 100: Alternate function 4 (I2C, etc.) */
#define EM32F967_AF5  5 /* 101: Alternate function 5 (I2C, WKUP, etc.) */
#define EM32F967_AF6  6 /* 110: Alternate function 6 (SSP2, etc.) */
#define EM32F967_AF7  7 /* 111: Alternate function 7 (PWM, etc.) */

/**
 * @brief Pin configuration bit field encoding (STM32-style)
 *
 * Fields:
 * - mux  [ 0 : 2 ]  - Multiplexing function (3 bits)
 * - pin  [ 3 : 6 ]  - Pin number (4 bits, 0-15)
 * - port [ 7 : 7 ]  - Port identifier (1 bit, 0=PA, 1=PB)
 *
 * This encoding allows up to 2 ports, 16 pins per port, and 8 mux functions
 */
#define EM32F967_MUX_SHIFT  0U
#define EM32F967_MUX_MASK   0x7U
#define EM32F967_PIN_SHIFT  3U
#define EM32F967_PIN_MASK   0xFU
#define EM32F967_PORT_SHIFT 7U
#define EM32F967_PORT_MASK  0x1U

/**
 * @brief EM32F967 Pin Multiplexing Macro (STM32-style)
 *
 * @param port Port identifier ('A' or 'B')
 * @param pin Pin number (0-15)
 * @param mux Multiplexing function (GPIO, AF1-AF7)
 */
#define EM32F967_PINMUX(port, pin, mux)                                                            \
	(((((port) - 'A') & EM32F967_PORT_MASK) << EM32F967_PORT_SHIFT) |                          \
	 (((pin) & EM32F967_PIN_MASK) << EM32F967_PIN_SHIFT) |                                     \
	 (((EM32F967_##mux) & EM32F967_MUX_MASK) << EM32F967_MUX_SHIFT))

/* Helper macros to extract fields from encoded pinmux value */
#define EM32F967_DT_PINMUX_PORT(pinmux) (((pinmux) >> EM32F967_PORT_SHIFT) & EM32F967_PORT_MASK)

#define EM32F967_DT_PINMUX_PIN(pinmux) (((pinmux) >> EM32F967_PIN_SHIFT) & EM32F967_PIN_MASK)

#define EM32F967_DT_PINMUX_MUX(pinmux) (((pinmux) >> EM32F967_MUX_SHIFT) & EM32F967_MUX_MASK)

/* Pull-up/Pull-down definitions for 5V tolerant pins (PA0-PA6) */
#define EM32F967_PULL_NONE_5V 0 /* 00: Floating */
#define EM32F967_PULL_UP0_5V  1 /* 01: PU0 (66KΩ@5V, 101KΩ@3.3V, 238KΩ@1.8V) */
#define EM32F967_PULL_UP1_5V  2 /* 10: PU1 (4.7KΩ@5V, 6.41KΩ@3.3V, 12.7KΩ@1.8V) */
#define EM32F967_PULL_DOWN_5V 3 /* 11: PD (15KΩ@5V, 21.8KΩ@3.3V, 49.6KΩ@1.8V) */

/* Pull-up/Pull-down definitions for 3V pins (PA11-PA15, PB0-PB15) */
#define EM32F967_PULL_NONE_3V 0 /* 00: Floating */
#define EM32F967_PULL_UP0_3V  1 /* 01: PU0 (66KΩ@3.3V, 140KΩ@1.8V) */
#define EM32F967_PULL_UP1_3V  2 /* 10: PU1 (4.7KΩ@3.3V, 8.53KΩ@1.8V) */
#define EM32F967_PULL_DOWN_3V 3 /* 11: PD (15KΩ@3.3V, 25.2KΩ@1.8V) */

/* Drive strength definitions */
#define EM32F967_DRIVE_NORMAL 0 /* Normal drive strength */
#define EM32F967_DRIVE_HIGH   1 /* High drive strength (Load=30pF) */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_EM32F967_PINCTRL_H_ */
