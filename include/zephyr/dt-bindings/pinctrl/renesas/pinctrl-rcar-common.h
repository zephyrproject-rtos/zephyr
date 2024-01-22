/*
 * Copyright (c) 2021-2023 IoT.bzh
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
 * Function shift   [ 4 : 8 ]
 * Empty            [ 9 ]
 * IPSR bank        [ 10 : 14 ]
 * Register index   [ 15 : 17 ] (S4 only)
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

/* Renesas Gen4 has IPSR registers at different base address
 * reg is here an index for the base address.
 * Each base address has 4 IPSR banks.
 */
#define IPnSR(bank, reg, shift, func) \
	IPSR(((reg) << 5U) | (bank), shift, func)

#define IP0SR0(shift, func) IPnSR(0, 0, shift, func)
#define IP1SR0(shift, func) IPnSR(1, 0, shift, func)
#define IP2SR0(shift, func) IPnSR(2, 0, shift, func)
#define IP3SR0(shift, func) IPnSR(3, 0, shift, func)
#define IP0SR1(shift, func) IPnSR(0, 1, shift, func)
#define IP1SR1(shift, func) IPnSR(1, 1, shift, func)
#define IP2SR1(shift, func) IPnSR(2, 1, shift, func)
#define IP3SR1(shift, func) IPnSR(3, 1, shift, func)
#define IP0SR2(shift, func) IPnSR(0, 2, shift, func)
#define IP1SR2(shift, func) IPnSR(1, 2, shift, func)
#define IP2SR2(shift, func) IPnSR(2, 2, shift, func)
#define IP3SR2(shift, func) IPnSR(3, 2, shift, func)
#define IP0SR3(shift, func) IPnSR(0, 3, shift, func)
#define IP1SR3(shift, func) IPnSR(1, 3, shift, func)
#define IP2SR3(shift, func) IPnSR(2, 3, shift, func)
#define IP3SR3(shift, func) IPnSR(3, 3, shift, func)
#define IP0SR4(shift, func) IPnSR(0, 4, shift, func)
#define IP1SR4(shift, func) IPnSR(1, 4, shift, func)
#define IP2SR4(shift, func) IPnSR(2, 4, shift, func)
#define IP3SR4(shift, func) IPnSR(3, 4, shift, func)
#define IP0SR5(shift, func) IPnSR(0, 5, shift, func)
#define IP1SR5(shift, func) IPnSR(1, 5, shift, func)
#define IP2SR5(shift, func) IPnSR(2, 5, shift, func)
#define IP3SR5(shift, func) IPnSR(3, 5, shift, func)

#define PIN_VOLTAGE_NONE 0
#define PIN_VOLTAGE_1P8V 1
#define PIN_VOLTAGE_3P3V 2

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RCAR_COMMON_H_ */
