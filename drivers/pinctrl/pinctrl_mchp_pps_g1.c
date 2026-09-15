/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file pinctrl_mchp_pps_g1.c
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
#define MCHP_PORT_ADDR_OR_NONE(nodelabel)                                                          \
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),            \
		(DT_REG_ADDR(DT_NODELABEL(nodelabel))))

/* Pins which supports analog function */
static const uint16_t mchp_ain_pin_mask[] = {0x4008, 0xCCFF, 0x0080, 0x00FC, 0x0060};

/**
 * @brief Array of port addresses for the MCHP G1 series pps peripheral.
 *
 * This array contains the register addresses of the ports (PORTA, PORTB, PORTC, PORTD, and PORTE)
 * for the MCHP G1 series pps peripheral. The addresses are obtained using the
 * MCHP_PORT_ADDR_OR_NONE macro, which ensures that only existing ports are included.
 * This can be updated for other devices using conditional compilation directives
 */
static const uint32_t mchp_port_addrs[] = {
	MCHP_PORT_ADDR_OR_NONE(porta),
	MCHP_PORT_ADDR_OR_NONE(portb),
#ifdef GPIOC_BASE_ADDRESS
	MCHP_PORT_ADDR_OR_NONE(portc),
#endif
#ifdef GPIOD_BASE_ADDRESS
	MCHP_PORT_ADDR_OR_NONE(portd),
#endif
#ifdef GPIOE_BASE_ADDRESS
	MCHP_PORT_ADDR_OR_NONE(porte),
#endif
	MCHP_PORT_ADDR_OR_NONE(pinctrl),
};

#define PINCTRL_REG_POS (ARRAY_SIZE(mchp_port_addrs) - 1)

/**
 * @brief Configure pin multiplexing functionality for each pins
 *
 * This function configures the pinmux registers for a given pin based on
 * the provided pin control information.
 *
 * @param pin Pointer to the pin control information
 */
static inline void pinctrl_pinmux(const pinctrl_soc_pin_t *pin)
{
	uint32_t *pps_register = (uint32_t *)(mchp_port_addrs[PINCTRL_REG_POS] +
					      (MCHP_PINMUX_PINOFFSET_GET(pin->pinmux)));

	if (pps_register != NULL) {
		*pps_register = MCHP_PINMUX_REGVAL_GET(pin->pinmux);
	}
}

/**
 * @brief Set all pin configuration registers by checking the flags
 *
 * This function configures various pin settings such as pull-up, pull-down,
 * input enable, output enable, and open drain based on the provided
 * pin control information.
 *
 * @param pin Pointer to the pin control information
 */
static void pinctrl_set_flags(const pinctrl_soc_pin_t *pin)
{
	uint8_t pin_num = MCHP_PINMUX_PIN_GET(pin->pinmux);
	uint8_t port_id = MCHP_PINMUX_PORT_GET(pin->pinmux);
	uint16_t pin_mask = BIT(pin_num);

	/* Get the GPIO register address based on the port information from pin control */
	gpio_registers_t *pRegister = (gpio_registers_t *)mchp_port_addrs[port_id];

	if (pRegister != NULL) {
		/* If any JTAG pins are being repurposed, disable JTAG globally */
		CFG_REGS->CFG_CFGCON0CLR = CFG_CFGCON0_JTAGEN_Msk;

		/* Digital Mode Enable */
		pRegister->GPIO_ANSELCLR = pin_mask & mchp_ain_pin_mask[port_id];

		/* Check if the pin is configured for input */
		if ((pin->pinflag & MCHP_PINCTRL_INPUTENABLE) != 0) {

			pRegister->GPIO_TRISSET = pin_mask;

			/* Check if the pin should have a pull-up resistor enabled */
			if ((pin->pinflag & MCHP_PINCTRL_PULLUP) != 0) {
				pRegister->GPIO_CNPUSET = pin_mask;
			}
			/* Check if the pin should have a pull-down resistor enabled */
			else if ((pin->pinflag & MCHP_PINCTRL_PULLDOWN) != 0) {
				pRegister->GPIO_CNPDSET = pin_mask;
			}
		}
		/* Check if the pin is configured for output */
		else if ((pin->pinflag & MCHP_PINCTRL_OUTPUTENABLE) != 0) {
			pRegister->GPIO_TRISCLR = pin_mask;

			/* Check if the pin is configured for open-drain output */
			if ((pin->pinflag & MCHP_PINCTRL_OPENDRAIN) != 0) {
				pRegister->GPIO_ODCSET = pin_mask;
			}
		} else {
			return;
		}
	}
}

/**
 * @brief Configure a specific pin based on the provided pin configuration.
 *
 * This helper function configures a pin by determining its port function and then
 * calling the appropriate functions to set the pinmux and other pin configurations.
 * Pinctrl configurations are skipped if direct mode is enabled
 *
 * @param pin The pin configuration to be applied. This is of type pinctrl_soc_pin_t.
 */
static void pinctrl_configure_pin(pinctrl_soc_pin_t pin)
{
	uint8_t port_func = MCHP_PINMUX_FUNC_GET(pin.pinmux);
	/* check if direct mode is configured */
	if (port_func != MCHP_PINMUX_FUNC_directmode) {
		/* check if alternate function is configured */
		if (port_func == MCHP_PINMUX_FUNC_periph) {
			pinctrl_pinmux(&pin);
		}
		/* set all other pin configurations */
		pinctrl_set_flags(&pin);
	}
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
