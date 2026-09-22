/*
 * SPDX-FileCopyrightText: 2024 ELAN Microelectronics Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_EM32F967_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_EM32F967_PINCTRL_H_

/*
 * EM32F967 Pin Control Definitions (EM32-style)
 */

/* Pin multiplexing values for EM32F967 - based on CSV specification */
/**
 * @brief GPIO mux function selector (mux field value 0).
 *
 * Selects the plain GPIO function (no alternate function) for a pin when used
 * as the @p mux argument of @ref EM32F967_PINMUX / @ref EM32F967_PINMUX_IO.
 */
#define EM32F967_GPIO 0 /* 000: GPIO function */
#define EM32F967_AF1  1 /* 001: Alternate function 1 (SPI1, etc.) */
#define EM32F967_AF2  2 /* 010: Alternate function 2 (UART, etc.) */
#define EM32F967_AF3  3 /* 011: Alternate function 3 (Timer, 7816, etc.) */
#define EM32F967_AF4  4 /* 100: Alternate function 4 (I2C, etc.) */
#define EM32F967_AF5  5 /* 101: Alternate function 5 (I2C, WKUP, etc.) */
#define EM32F967_AF6  6 /* 110: Alternate function 6 (SSP2, etc.) */
#define EM32F967_AF7  7 /* 111: Alternate function 7 (PWM, etc.) */

/**
 * @brief Pin configuration bit field encoding (EM32-style)
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
 * @brief EM32F967 Pin Multiplexing Macro (EM32-style)
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

/**
 * @brief Embedded IP_SHARE operations carried inside the pinmux cell
 *
 * The high 24 bits of the pinmux value (bits [31:8]) may encode up to two
 * IP_SHARE read-modify-write operations.  The pinctrl driver
 * (drivers/pinctrl/pinctrl_em32.c, em32_apply_ioshare_from_pinmux()) decodes
 * and applies them when present, instead of falling back to the built-in
 * em32_ioshare_table[] lookup.
 *
 * Bit layout (must stay in sync with the PINMUX_IO* defines in the driver):
 *
 *   bit  [ 8]     io1_valid    (1 = first IP_SHARE op present)
 *   bits [13:9]   io1_shift    (bit position 0-31 in IP_SHARE)
 *   bits [15:14]  io1_width_m1 (field width minus 1: 0 = 1 bit)
 *   bits [19:16]  io1_value    (value written at io1_shift)
 *   bit  [20]     io2_valid    (1 = second IP_SHARE op present)
 *   bits [25:21]  io2_shift
 *   bits [27:26]  io2_width_m1
 *   bits [31:28]  io2_value
 */
#define EM32F967_IO_NONE 0U

/* Explicit op builders (shift = bit position, width = field width in bits) */
/**
 * @brief Build the first (slot 1) embedded IP_SHARE operation.
 *
 * Encodes a single IP_SHARE read-modify-write operation into pinmux bits
 * [19:8] (valid flag, shift, width-1 and value). Combine with
 * @ref EM32F967_IO2 via bitwise OR to encode a second operation.
 *
 * @param shift Bit position (0-31) of the field within the IP_SHARE register.
 * @param width Field width in bits (1-4).
 * @param val   Value to write into the field.
 */
#define EM32F967_IO1(shift, width, val)                                                            \
	((1U << 8) | (((shift) & 0x1FU) << 9) | ((((width) - 1U) & 0x3U) << 14) |                  \
	 (((val) & 0xFU) << 16))

/**
 * @brief Build the second (slot 2) embedded IP_SHARE operation.
 *
 * Encodes a single IP_SHARE read-modify-write operation into pinmux bits
 * [31:20] (valid flag, shift, width-1 and value). Combine with
 * @ref EM32F967_IO1 (or its convenience helpers) via bitwise OR to encode a
 * second operation in the same pinmux cell.
 *
 * @param shift Bit position (0-31) of the field within the IP_SHARE register.
 * @param width Field width in bits (1-4).
 * @param val   Value to write into the field.
 */
#define EM32F967_IO2(shift, width, val)                                                            \
	((1U << 20) | (((shift) & 0x1FU) << 21) | ((((width) - 1U) & 0x3U) << 26) |                \
	 (((val) & 0xFU) << 28))

/* Convenience helpers for slot 1 (combine with EM32F967_IO2() via bitwise OR) */
/**
 * @brief Slot 1 helper: set a single IP_SHARE bit.
 *
 * Equivalent to @ref EM32F967_IO1 with a 1-bit field whose value is 1.
 *
 * @param pos Bit position (0-31) to set in the IP_SHARE register.
 */
#define EM32F967_IO_SET_BIT(pos)       EM32F967_IO1((pos), 1U, 1U)
/**
 * @brief Slot 1 helper: clear a single IP_SHARE bit.
 *
 * Equivalent to @ref EM32F967_IO1 with a 1-bit field whose value is 0.
 *
 * @param pos Bit position (0-31) to clear in the IP_SHARE register.
 */
#define EM32F967_IO_CLR_BIT(pos)       EM32F967_IO1((pos), 1U, 0U)
/**
 * @brief Slot 1 helper: write a 2-bit IP_SHARE field.
 *
 * Equivalent to @ref EM32F967_IO1 with a 2-bit field.
 *
 * @param pos Bit position (0-31) of the 2-bit field in the IP_SHARE register.
 * @param val Value (0-3) to write into the field.
 */
#define EM32F967_IO_SET_2BIT(pos, val) EM32F967_IO1((pos), 2U, (val))
/**
 * @brief Slot 1 helper: clear a 2-bit IP_SHARE field.
 *
 * Equivalent to @ref EM32F967_IO1 with a 2-bit field whose value is 0.
 *
 * @param pos Bit position (0-31) of the 2-bit field in the IP_SHARE register.
 */
#define EM32F967_IO_CLR_2BIT(pos)      EM32F967_IO1((pos), 2U, 0U)

/**
 * @brief EM32F967 pinmux macro with embedded IP_SHARE operation(s)
 *
 * @param port Port identifier ('A' or 'B')
 * @param pin Pin number (0-15)
 * @param mux Multiplexing function (GPIO, AF1-AF7)
 * @param ioshare One or two IP_SHARE ops built with EM32F967_IO_* / EM32F967_IO1 /
 *                EM32F967_IO2, or EM32F967_IO_NONE.
 */
#define EM32F967_PINMUX_IO(port, pin, mux, ioshare) (EM32F967_PINMUX(port, pin, mux) | (ioshare))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_EM32F967_PINCTRL_H_ */
