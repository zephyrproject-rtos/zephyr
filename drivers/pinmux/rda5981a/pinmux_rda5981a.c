/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include "pinmux.h"
#include <pinmux.h>
#include <pinmux/rda5981a/pinmux_rda5981a.h>
#include "soc_registers.h"

static const uint8_t porta_iomux_index[] = {
	26, 27, 14, 15, 16, 17, 18, 19, 10, 11
};

static uint8_t get_iomux_index(uint32_t pin)
{
	int port = PIN_TO_PORT(pin), i;
	uint8_t index = NC;

	switch (port) {
	case PIN_PORTA:
		for (i = 0; i < ARRAY_SIZE(porta_iomux_index); i++)
			if (porta_iomux_index[i] == GET_PIN(pin))
				index = i; /* PORTA gpio 26,27,14,15,16,17,18,19,10,11*/
		break;
	case PIN_PORTB:
		index = GET_PIN(pin); /* PORTB gpio 0-9*/
		break;
	case PIN_PORTC:
		index = GET_PIN(pin) - 12; /* PORTC gpio 12-21*/
		break;
	case PIN_PORTD:
		index = GET_PIN(pin) - 22; /* PORTD gpio 22-25*/
		break;
	default:
		break;
	}

	return index;
}

void pinmux_rda5981a_set(uint32_t pin, uint32_t func)
{
	volatile struct pincfg_rda5981a *cfg =
		(struct pincfg_rda5981a *)RDA_PINCFG_BASE;
	int index = PIN_TO_PORT(pin);
	int offset = get_iomux_index(pin);

	/* WTF !!!? */
	if ((PIN_PORTC == index)) {
		offset = offset << 1;
		cfg->mode2 &= ~(0x03 << offset);
	} else if (PIN_PORTD == index) {
		if (2 > offset) {
			offset = (offset << 1) + 20;
			cfg->mode2 &= ~(0x03 << offset);
		} else {
			offset = (offset << 1) - 4;
			cfg->mode3 &= ~(0x03 << offset);
		}
	}

	offset = get_iomux_index(pin) * 3; /* for the reg bit offset */
	cfg->iomuxctrl[index] &= ~(0x07 << offset);
	cfg->iomuxctrl[index] |= (func & 0x07) << offset;
}

void rda5981a_setup_pins(const struct pin_config *pinconf, uint32_t pins)
{
	int i;

	for (i = 0; i < pins; i++) {
		pinmux_rda5981a_set(pinconf[i].pin_num, pinconf[i].mode);
	}
}
