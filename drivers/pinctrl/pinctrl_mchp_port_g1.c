/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file pinctrl_mchp_port_g1.c
 * @brief Pin control driver for Microchip devices.
 *
 * This file provides the implementation of pin control functions
 * for Microchip-based systems.
 */

#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

/**
 * @brief Utility macro that expands to the PORT address if it exists.
 *
 * This macro checks if a node label exists in the device tree and, if it does,
 * it expands to the register address of that node label. If the node label does
 * not exist, it expands to nothing.
 *
 * @param nodelabel The node label to check in the device tree.
 */
/* clang-format off */
#define MCHP_PORT_ADDR_OR_NONE(nodelabel)                              \
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),            \
		(DT_REG_ADDR(DT_NODELABEL(nodelabel)),))

/**
 * @brief Array of port addresses for the MCHP G1 series.
 *
 * This array contains the register addresses of the ports (PORTA, PORTB, PORTC, and PORTD)
 * for the MCHP G1 series port peripheral. The addresses are obtained using the
 * MCHP_PORT_ADDR_OR_NONE macro, which ensures that only existing ports are included.
 * This can be updated for other devices using conditional compilation directives
 */
static const uint32_t mchp_port_addrs[] = {
	MCHP_PORT_ADDR_OR_NONE(porta)
	MCHP_PORT_ADDR_OR_NONE(portb)
	MCHP_PORT_ADDR_OR_NONE(portc)
	MCHP_PORT_ADDR_OR_NONE(portd)
};
/* clang-format on */

/**
 * @brief Set pinmux registers using odd/even logic
 *
 * This function configures the pinmux registers for a given pin based on
 * the provided pin control information. It determines whether the pin number
 * is odd or even and sets the appropriate bits in the PORT_PMUX register.
 *
 * @param pin Pointer to the pin control information
 * @param reg Pointer to the base address of the port group registers
 */
static void pinctrl_pinmux(const pinctrl_soc_pin_t *pin)
{
	port_group_registers_t *pRegister;

	uint8_t pin_num = MCHP_PINMUX_PIN_GET(pin->pinmux);
	uint8_t port_id = MCHP_PINMUX_PORT_GET(pin->pinmux);
	uint8_t pin_mux = MCHP_PINMUX_PERIPH_GET(pin->pinmux);

	bool is_odd = pin_num & 1;
	uint32_t idx = pin_num / 2U;

	pRegister = (port_group_registers_t *)mchp_port_addrs[port_id];

	if (pRegister != NULL) {
		/* Each pinmux register holds the config for two pins.  The
		 * even numbered pin goes in the bits 0..3 and the odd
		 * numbered pin in bits 4..7.
		 */
		if (is_odd == true) {
			pRegister->PORT_PMUX[idx] |= PORT_PMUX_PMUXO(pin_mux);
		} else {
			pRegister->PORT_PMUX[idx] |= PORT_PMUX_PMUXE(pin_mux);
		}
		pRegister->PORT_PINCFG[pin_num] |= (uint8_t)PORT_PINCFG_PMUXEN_Msk;
	}
}

/**
 * @brief Set all pin configuration registers by checking the flags
 *
 * This function configures various pin settings such as pull-up, pull-down,
 * input enable, output enable, and drive strength based on the provided
 * pin control information.
 *
 * @param pin Pointer to the pin control information
 * @param reg Pointer to the base address of the port group registers
 */
static void pinctrl_set_flags(const pinctrl_soc_pin_t *pin)
{
	port_group_registers_t *pRegister;

	uint8_t pin_num = MCHP_PINMUX_PIN_GET(pin->pinmux);
	uint8_t port_id = MCHP_PINMUX_PORT_GET(pin->pinmux);

	pRegister = (port_group_registers_t *)mchp_port_addrs[port_id];

	if (pRegister != NULL) {

		/* Check if pull-up or pull-down resistors need to be configured */
		if (((pin->pinflag & MCHP_PINCTRL_PULLUP) != 0) ||
		    ((pin->pinflag & MCHP_PINCTRL_PULLDOWN) != 0)) {
			if ((pin->pinflag & MCHP_PINCTRL_PULLUP) != 0) {
				/* If pull-up resistor enabled,
				 * set the corresponding bit in PORT_OUT reg
				 */
				pRegister->PORT_OUT |= (1 << pin_num);
			}
			pRegister->PORT_PINCFG[pin_num] |= PORT_PINCFG_PULLEN(1);
		} else {
			pRegister->PORT_PINCFG[pin_num] &= ~PORT_PINCFG_PULLEN(1);
		}

		/* if input is enabled, set the corresponding bit in PORT_PINCFG register */
		if ((pin->pinflag & MCHP_PINCTRL_INPUTENABLE) != 0) {
			pRegister->PORT_PINCFG[pin_num] |= PORT_PINCFG_INEN(1);
		} else {
			pRegister->PORT_PINCFG[pin_num] &= ~PORT_PINCFG_INEN(1);
		}

		/* if output is enabled, set the corresponding bit in PORT_DIR register */
		if ((pin->pinflag & MCHP_PINCTRL_OUTPUTENABLE) != 0) {
			pRegister->PORT_DIR |= (1 << pin_num);
		} else {
			pRegister->PORT_DIR &= ~(1 << pin_num);
		}

		/* if drive strength is enabled, set the corresponding bit in PORT_PINCFG reg */
		if ((pin->pinflag & MCHP_PINCTRL_DRIVESTRENGTH) != 0) {
			pRegister->PORT_PINCFG[pin_num] |= PORT_PINCFG_DRVSTR(1);
		} else {
			pRegister->PORT_PINCFG[pin_num] &= ~PORT_PINCFG_DRVSTR(1);
		}
	}
}

/**
 * @brief Configure a specific pin based on the provided pin configuration.
 *
 * This helper function configures a pin by determining its port function and then
 * calling the appropriate functions to set the pinmux and other pin configurations.
 *
 * @param pin The pin configuration to be applied. This is of type pinctrl_soc_pin_t.
 */
static void pinctrl_configure_pin(pinctrl_soc_pin_t pin)
{
	uint8_t port_func = MCHP_PINMUX_FUNC_GET(pin.pinmux);

	/* call pinmux function if alternate function is configured */
	if (port_func == MCHP_PINMUX_FUNC_periph) {
		pinctrl_pinmux(&pin);
	}

	/* set all other pin configurations */
	pinctrl_set_flags(&pin);
}

/**
 * @brief Configure multiple pins.
 *
 * This function configures a set of pins based on the provided pin
 * configuration array.
 *
 * @param pins Pointer to an array of pinctrl_soc_pin_t structures that
 *             define the pin configurations.
 * @param pin_cnt Number of pins to configure.
 * @param reg Unused parameter.
 *
 * @return 0 on success.
 */
int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(*pins);
		pins++;
	}

	return 0;
}
