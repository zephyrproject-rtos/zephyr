/*
 * Copyright (c) 2021 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RCAR_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RCAR_COMMON_H_

/**
 * @brief Utility macro to build IPSR property entry.
 * IPSR: Peripheral Function Select Register
 * Each IPSR bank can hold 8 cellules of 4 bits coded function.
 *
 * @param bank the IPSR register bank.
 * @param shift the bit shift for this alternate function.
 * @param func the 4 bits encoded alternate function.
 *
 * Function code    [ 0 : 3 ]
 * Function shift   [ 4 : 9 ]
 * IPSR bank        [ 10 : 13 ]
 */
#define IPSR(bank, shift, func) (((bank) << 10U) | ((shift) << 4U) | (func))

/* Arbitrary number to encode non capable gpio pin */
#define PIN_NOGPSR_START 1024U

/**
 * @brief Utility macro to encode a GPIO capable pin
 *
 * @param bank the GPIO bank
 * @param pin the pin within the GPIO bank (0..31)
 */
#define RCAR_GP_PIN(bank, pin)     (((bank) * 32U) + (pin))

/**
 * @brief Utility macro to encode a non capable GPIO pin
 *
 * @param pin the encoded pin number
 */
#define RCAR_NOGP_PIN(pin)         (PIN_NOGPSR_START + pin)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RCAR_COMMON_H_ */
