/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NRF_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NRF_PINCTRL_H_

/**
 * @name nRF pinctrl pin functions.
 * @{
 */
/** @} */

/**
 * @name nRF pinctrl pin direction.
 * @note Values match nrf_gpio_pin_dir_t.
 * @{
 */
/** Input. */
#define PINCTRL_NRF_DIR_INP 0
/** Output. */
#define PINCTRL_NRF_DIR_OUT 1
/** @} */

/**
 * @name nRF pinctrl input buffer.
 * @note Values match nrf_gpio_pin_input_t.
 * @{
 */
/** Connect input */
#define PINCTRL_NRF_INP_ICONN 0
/** Disconnect input */
#define PINCTRL_NRF_INP_IDISC 1
/** @} */

/**
 * @name nRF pinctrl pull-up/down.
 * @note Values match nrf_gpio_pin_pull_t.
 * @{
 */
/** Pull-up disabled. */
#define PINCTRL_NRF_PULL_PN 0
/** Pull-down enabled. */
#define PINCTRL_NRF_PULL_PD 1
/** Pull-up enabled. */
#define PINCTRL_NRF_PULL_PU 2
/** @} */

/**
 * @name Positions and masks for the pincfg bit field.
 * @{
 */
#define PINCTRL_NRF_PIN_POS 0U
#define PINCTRL_NRF_PIN_MSK 0x3FU
#define PINCTRL_NRF_DIR_POS 6U
#define PINCTRL_NRF_DIR_MSK 0x1U
#define PINCTRL_NRF_INP_POS 7U
#define PINCTRL_NRF_INP_MSK 0x1U
#define PINCTRL_NRF_PULL_POS 8U
#define PINCTRL_NRF_PULL_MSK 0x3U
#define PINCTRL_NRF_FUN_POS 16U
#define PINCTRL_NRF_FUN_MSK 0xFFFFU
/** @} */

/**
 * @brief Utility macro to build nRF pinctrl configuration entry for a pin.
 *
 * The information is encoded in a bitfield organized as follows:
 *
 * - 31..16: Pin function.
 * - 15..10: Reserved.
 * - 9..8: Pin pull configuration.
 * - 7: Pin input buffer configuration.
 * - 6: Pin direction.
 * - 5..0: Pin number (combination of port and pin).
 *
 * @param port Port (0 or 1).
 * @param pin Pin (0..32).
 * @param dir Pin direction configuration.
 * @param inp Pin input buffer configuration.
 * @param pull Pin pull configuration.
 * @param fun Pin function configuration.
 */
#define PINCTRL_NRF(port, pin, dir, inp, pull, fun) \
	((((((port) * 32U) + (pin)) & PINCTRL_NRF_PIN_MSK) << PINCTRL_NRF_PIN_POS) | \
	 (((PINCTRL_NRF_DIR_ ## dir) & PINCTRL_NRF_DIR_MSK) << PINCTRL_NRF_DIR_POS) | \
	 (((PINCTRL_NRF_INP_ ## inp) & PINCTRL_NRF_INP_MSK) << PINCTRL_NRF_INP_POS) | \
	 (((PINCTRL_NRF_PULL_ ## pull) & PINCTRL_NRF_PULL_MSK) << PINCTRL_NRF_PULL_POS) | \
	 (((PINCTRL_NRF_FUN_ ## fun) & PINCTRL_NRF_FUN_MSK) << PINCTRL_NRF_FUN_POS))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NRF_PINCTRL_H_ */
