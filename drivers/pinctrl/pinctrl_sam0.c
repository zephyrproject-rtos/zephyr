/*
 * Copyright (c) 2022, Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <soc_port.h>

/** Utility macro that expands to the PORT port address if it exists */
#define SAM_PORT_ADDR_OR_NONE(nodelabel)					\
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),			\
		   (DT_REG_ADDR(DT_NODELABEL(nodelabel)),))

/** SAM0 port addresses */
static const uint32_t sam_port_addrs[] = {
	SAM_PORT_ADDR_OR_NONE(porta)
	SAM_PORT_ADDR_OR_NONE(portb)
	SAM_PORT_ADDR_OR_NONE(portc)
	SAM_PORT_ADDR_OR_NONE(portd)
	SAM_PORT_ADDR_OR_NONE(porte)
	SAM_PORT_ADDR_OR_NONE(portf)
};

static void pinctrl_configure_pin(pinctrl_soc_pin_t pin)
{
	struct soc_port_pin soc_pin;
	uint8_t  port_idx, port_func;

	port_idx = SAM_PINMUX_PORT_GET(pin);
	__ASSERT_NO_MSG(port_idx < ARRAY_SIZE(sam_port_addrs));
	port_func = SAM_PINMUX_FUNC_GET(pin);

	soc_pin.regs = (PortGroup *) sam_port_addrs[port_idx];
	soc_pin.pinum = SAM_PINMUX_PIN_GET(pin);
	soc_pin.flags = SAM_PINCTRL_FLAGS_GET(pin) << SOC_PORT_FLAGS_POS;

	if (port_func == SAM_PINMUX_FUNC_periph) {
		soc_pin.flags |= (SAM_PINMUX_PERIPH_GET(pin)
				  << SOC_PORT_FUNC_POS)
			      |  SOC_PORT_PMUXEN_ENABLE;
	}

	soc_port_configure(&soc_pin);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(*pins++);
	}

	return 0;
}
