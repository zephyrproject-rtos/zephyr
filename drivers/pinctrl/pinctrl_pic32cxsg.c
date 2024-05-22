/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <soc_port.h>

/** Utility macro that expands to the PORT port address if it exists */
#define PIC32CXSG_PORT_ADDR_OR_NONE(nodelabel)					\
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),			\
		   (DT_REG_ADDR(DT_NODELABEL(nodelabel)),))

/** SAM0 port addresses */
static const uint32_t pic32cxsg_port_addrs[] = {
	PIC32CXSG_PORT_ADDR_OR_NONE(porta)
	PIC32CXSG_PORT_ADDR_OR_NONE(portb)
	PIC32CXSG_PORT_ADDR_OR_NONE(portc)
	PIC32CXSG_PORT_ADDR_OR_NONE(portd)
	PIC32CXSG_PORT_ADDR_OR_NONE(porte)
	PIC32CXSG_PORT_ADDR_OR_NONE(portf)
};

static void pinctrl_configure_pin(pinctrl_soc_pin_t pin)
{
	struct soc_port_pin soc_pin;
	uint8_t  port_idx, port_func;

	port_idx = PIC32CXSG_PINMUX_PORT_GET(pin);
	__ASSERT_NO_MSG(port_idx < ARRAY_SIZE(pic32cxsg_port_addrs));
	port_func = PIC32CXSG_PINMUX_FUNC_GET(pin);

	soc_pin.regs = (PortGroup *) pic32cxsg_port_addrs[port_idx];
	soc_pin.pinum = PIC32CXSG_PINMUX_PIN_GET(pin);
	soc_pin.flags = PIC32CXSG_PINCTRL_FLAGS_GET(pin) << SOC_PORT_FLAGS_POS;

	if (port_func == PIC32CXSG_PINMUX_FUNC_periph) {
		soc_pin.flags |= (PIC32CXSG_PINMUX_PERIPH_GET(pin)
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
