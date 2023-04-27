/*
 * Copyright (c) 2022 Silicon Labs
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_GECKO_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_GECKO_PINCTRL_H_

/*
 * The whole GECKO_pin configuration information is encoded in a 32-bit bitfield
 * organized as follows:
 *
 * - 31..24: Pin function.
 * - 23..16: Reserved.
 * - 15..8:  Port for UART_RX/UART_TX functions.
 * - 7..0:   Pin number for UART_RX/UART_TX functions.
 * - 15..8:  Reserved for UART_LOC function.
 * - 7..0:   Loc for UART_LOC function.
 */

/**
 * @name GECKO_pin configuration bit field positions and masks.
 * @{
 */

/** Position of the function field. */
#define GECKO_FUN_POS 24U
/** Mask for the function field. */
#define GECKO_FUN_MSK 0xFFFU

/** Position of the pin field. */
#define GECKO_PIN_POS 0U
/** Mask for the pin field. */
#define GECKO_PIN_MSK 0xFFU

/** Position of the port field. */
#define GECKO_PORT_POS 8U
/** Mask for the port field. */
#define GECKO_PORT_MSK 0xFFU

/** Position of the loc field. */
#define GECKO_LOC_POS 0U
/** Mask for the pin field. */
#define GECKO_LOC_MSK 0xFFU

/** @} */

/**
 * @name GECKO_pinctrl pin functions.
 * @{
 */

/** UART TX */
#define GECKO_FUN_UART_TX  0U
/** UART RX */
#define GECKO_FUN_UART_RX  1U
/** UART RTS */
#define GECKO_FUN_UART_RTS 2U
/** UART CTS */
#define GECKO_FUN_UART_CTS 3U
/** UART RX LOCATION */
#define GECKO_FUN_UART_RX_LOC 4U
/** UART TX LOCATION */
#define GECKO_FUN_UART_TX_LOC 5U
/** UART RTS LOCATION */
#define GECKO_FUN_UART_RTS_LOC 6U
/** UART CTS LOCATION */
#define GECKO_FUN_UART_CTS_LOC 7U

#define GECKO_FUN_SPIM_MISO 8U
#define GECKO_FUN_SPIM_MOSI 9U
#define GECKO_FUN_SPIM_CS  10U
#define GECKO_FUN_SPIM_SCK  11U

#define GECKO_FUN_LEUART_RX_LOC 12U
#define GECKO_FUN_LEUART_TX_LOC 13U

#define GECKO_FUN_SPIS_MISO 14U
#define GECKO_FUN_SPIS_MOSI 15U
#define GECKO_FUN_SPIS_CS  16U
#define GECKO_FUN_SPIS_SCK  17U

#define GECKO_FUN_SPI_MISO_LOC 18U
#define GECKO_FUN_SPI_MOSI_LOC 19U
#define GECKO_FUN_SPI_CS_LOC  20U
#define GECKO_FUN_SPI_SCK_LOC  21U

/** @} */

/**
 * @brief Utility macro to build GECKO psels property entry.
 *
 * @param fun Pin function configuration (see GECKO_FUNC_{name} macros).
 * @param port Port (0 or 1).
 * @param pin Pin (0..31).
 */
#define GECKO_PSEL(fun, port, pin)                                                                 \
	(((GECKO_PORT_##port & GECKO_PORT_MSK) << GECKO_PORT_POS) |                                \
	 ((GECKO_PIN(##pin##) & GECKO_PIN_MSK) << GECKO_PIN_POS) |                                 \
	 ((GECKO_FUN_##fun & GECKO_FUN_MSK) << GECKO_FUN_POS))

/**
 * @brief Utility macro to build GECKO_psels property entry.
 *
 * @param fun Pin function configuration (see GECKO_FUNC_{name} macros).
 * @param loc Location.
 */
#define GECKO_LOC(fun, loc)                                                                        \
	(((GECKO_LOCATION(##loc##) & GECKO_LOC_MSK) << GECKO_LOC_POS) |                            \
	 ((GECKO_FUN_##fun##_LOC & GECKO_FUN_MSK) << GECKO_FUN_POS))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_GECKO_PINCTRL_H_ */
