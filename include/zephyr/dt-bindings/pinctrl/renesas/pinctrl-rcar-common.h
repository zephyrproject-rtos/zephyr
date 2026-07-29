/*
 * Copyright (c) 2021-2023 IoT.bzh
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree pin control helpers for Renesas R-Car
 * @ingroup pinctrl_rcar
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RCAR_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RCAR_COMMON_H_

/**
 * @addtogroup renesas_pinctrl Renesas pin control helpers
 * @ingroup devicetree-pinctrl
 */

/**
 * @defgroup pinctrl_rcar Renesas R-Car pin control helpers
 * @brief Pin and function macros for Renesas R-Car Gen3/Gen4/Gen5 SoCs
 * @ingroup renesas_pinctrl
 *
 * The R-Car Pin Function Controller (PFC) is configured by pairing a pin
 * identifier with the alternate function to route to it. Both are defined per
 * SoC in the matching header:
 *
 * - @c pinctrl-r8a77951.h — R-Car H3
 * - @c pinctrl-r8a77961.h — R-Car M3-W
 * - @c pinctrl-r8a779f0.h — R-Car S4
 * - @c pinctrl-r8a779g0.h — R-Car V4H
 * - @c pinctrl-r8a78000.h — R-Car Gen5
 *
 * Each of those headers provides @c PIN_\<NAME\> pin identifiers and
 * @c FUNC_\<NAME\> alternate-function values, where @c \<NAME\> is the signal
 * name from the SoC datasheet (for example @c PIN_RD and @c FUNC_CAN0_TX_A).
 * Both are built from the encoding helpers defined here — RCAR_GP_PIN() and
 * RCAR_NOGP_PIN() for pins, IPSR() / IPnSR() (Gen3/4) or RCAR_ALTSEL_FUNC()
 * (Gen5) for functions.
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-r8a77951.h>
 *
 * &pfc {
 *         can0_data_a_tx_default: can0_data_a_tx_default {
 *                 pin = <PIN_RD FUNC_CAN0_TX_A>;
 *         };
 * };
 * @endcode
 *
 * @{
 */

/**
 * @brief Utility macro to build IPSR property entry (Gen3/4 only).
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

/** @cond INTERNAL_HIDDEN */
/* Arbitrary number to encode non capable gpio pin */
#define PIN_NOGPSR_START 1024U
/** @endcond */

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

/**
 * @brief Utility macro to build an IPSR property entry for Gen4 SoCs.
 *
 * Renesas Gen4 has IPSR registers at different base addresses; @p reg is an
 * index for the base address. Each base address has 4 IPSR banks.
 *
 * @param bank the IPSR register bank.
 * @param reg the IPSR base address index.
 * @param shift the bit shift for this alternate function.
 * @param func the 4 bits encoded alternate function.
 */
#define IPnSR(bank, reg, shift, func) \
	IPSR(((reg) << 5U) | (bank), shift, func)

/** @cond INTERNAL_HIDDEN */
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
#define IP0SR6(shift, func) IPnSR(0, 6, shift, func)
#define IP1SR6(shift, func) IPnSR(1, 6, shift, func)
#define IP2SR6(shift, func) IPnSR(2, 6, shift, func)
#define IP3SR6(shift, func) IPnSR(3, 6, shift, func)
#define IP0SR7(shift, func) IPnSR(0, 7, shift, func)
#define IP1SR7(shift, func) IPnSR(1, 7, shift, func)
#define IP2SR7(shift, func) IPnSR(2, 7, shift, func)
#define IP3SR7(shift, func) IPnSR(3, 7, shift, func)
#define IP0SR8(shift, func) IPnSR(0, 8, shift, func)
#define IP1SR8(shift, func) IPnSR(1, 8, shift, func)
#define IP2SR8(shift, func) IPnSR(2, 8, shift, func)
#define IP3SR8(shift, func) IPnSR(3, 8, shift, func)
/** @endcond */

/**
 * @brief Macro to define a dummy IPSR flag for a pin
 *
 * This macro is used to define a dummy IPSR flag for a pin in the R-Car PFC
 * driver. It is intended for pins that do not have a specific function
 * defined in IPSR but always act as a peripheral. The dummy IPSR flag ensures
 * that the driver sets the 'peripheral' bit for such pins.
 *
 * @see RCAR_PIN_FLAGS_FUNC_DUMMY
 */
#define IPSR_DUMMY IPnSR(0x1f, 7, 0x1f, 0xf)

/**
 * @brief Utility macro to build ALTSEL property entry (Gen5 only).
 * ALTSELn (n = 0 to 3): Alternative Peripheral Function Select Registers.
 * Each function is coded on 4 bits hold by ALTSELn registers (1 bit per ALTSELn register).
 *
 * @param group The GPIO group (from 0 to 10).
 * @param pin   The pin (i.e. bit from 0 to 31).
 * @param func  The alternate function, encoded on 4 bits.
 *
 * Function code [ 0 : 3 ]
 * Empty         [ 4 : 9 ]
 * GPIO group    [ 10 : 13 ]
 * Pin           [ 14 : 18 ]
 */
#define RCAR_ALTSEL_FUNC(group, pin, func) (((pin) << 14U) | ((group) << 10U) | (func))

/**
 * @name R-Car pin I/O voltage levels
 * @{
 */
#define PIN_VOLTAGE_NONE 0 /**< No I/O voltage selection. */
#define PIN_VOLTAGE_1P8V 1 /**< 1.8 V I/O voltage. */
#define PIN_VOLTAGE_3P3V 2 /**< 3.3 V I/O voltage. */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RCAR_COMMON_H_ */
