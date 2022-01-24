/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NRF_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NRF_PINCTRL_H_

/*
 * The whole nRF pin configuration information is encoded in a 32-bit bitfield
 * organized as follows:
 *
 * - 31..16: Pin function.
 * - 15..13: Reserved.
 * - 12:     Pin low power mode.
 * - 11..8:  Pin output drive configuration.
 * - 7..6:   Pin pull configuration.
 * - 5..0:   Pin number (combination of port and pin).
 */

/**
 * @name nRF pin configuration bit field positions and masks.
 * @{
 */

/** Position of the function field. */
#define NRF_FUN_POS 16U
/** Mask for the function field. */
#define NRF_FUN_MSK 0xFFFFU
/** Position of the low power field. */
#define NRF_LP_POS 12U
/** Mask for the low power field. */
#define NRF_LP_MSK 0x1U
/** Position of the drive configuration field. */
#define NRF_DRIVE_POS 8U
/** Mask for the drive configuration field. */
#define NRF_DRIVE_MSK 0xFU
/** Position of the pull configuration field. */
#define NRF_PULL_POS 6U
/** Mask for the pull configuration field. */
#define NRF_PULL_MSK 0x3U
/** Position of the pin field. */
#define NRF_PIN_POS 0U
/** Mask for the pin field. */
#define NRF_PIN_MSK 0x3FU

/** @} */

/**
 * @name nRF pinctrl pin functions.
 * @{
 */

/** UART TX */
#define NRF_FUN_UART_TX 0U
/** UART RX */
#define NRF_FUN_UART_RX 1U
/** UART RTS */
#define NRF_FUN_UART_RTS 2U
/** UART CTS */
#define NRF_FUN_UART_CTS 3U

/** @} */

/**
 * @name nRF pinctrl output drive.
 * @note Values match nrf_gpio_pin_drive_t constants.
 * @{
 */

/** Standard '0', standard '1'. */
#define NRF_DRIVE_S0S1 0U
/** High drive '0', standard '1'. */
#define NRF_DRIVE_H0S1 1U
/** Standard '0', high drive '1'. */
#define NRF_DRIVE_S0H1 2U
/** High drive '0', high drive '1'. */
#define NRF_DRIVE_H0H1 3U
/** Disconnect '0' standard '1'. */
#define NRF_DRIVE_D0S1 4U
/** Disconnect '0', high drive '1'. */
#define NRF_DRIVE_D0H1 5U
/** Standard '0', disconnect '1'. */
#define NRF_DRIVE_S0D1 6U
/** High drive '0', disconnect '1'. */
#define NRF_DRIVE_H0D1 7U
/** Extra high drive '0', extra high drive '1'. */
#define NRF_DRIVE_E0E1 11U

/** @} */

/**
 * @name nRF pinctrl pull-up/down.
 * @note Values match nrf_gpio_pin_pull_t constants.
 * @{
 */

/** Pull-up disabled. */
#define NRF_PULL_NONE 0U
/** Pull-down enabled. */
#define NRF_PULL_DOWN 1U
/** Pull-up enabled. */
#define NRF_PULL_UP 3U

/** @} */

/**
 * @name nRF pinctrl low power mode.
 * @{
 */

/** Input. */
#define NRF_LP_DISABLE 0U
/** Output. */
#define NRF_LP_ENABLE 1U

/** @} */

/**
 * @brief Utility macro to build nRF psels property entry.
 *
 * @param fun Pin function configuration (see NRF_FUNC_{name} macros).
 * @param port Port (0 or 1).
 * @param pin Pin (0..31).
 */
#define NRF_PSEL(fun, port, pin)						       \
	((((((port) * 32U) + (pin)) & NRF_PIN_MSK) << NRF_PIN_POS) |	       \
	 ((NRF_FUN_ ## fun & NRF_FUN_MSK) << NRF_FUN_POS))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NRF_PINCTRL_H_ */
