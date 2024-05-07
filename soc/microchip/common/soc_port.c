/*
 * Copyright (c) 2016 Piotr Mienkowski
 * Copyright (c) 2018 Google LLC.
 * Copyright (c) 2021 Gerson Fernando Budke
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM0 MCU family I/O Pin Controller (PORT)
 */

#include <stdbool.h>

#include "soc_port.h"

int soc_port_pinmux_set(PortGroup *pg, uint32_t pin, uint32_t func)
{
	bool is_odd = pin & 1;
	int idx = pin / 2U;

	/* Each pinmux register holds the config for two pins.  The
	 * even numbered pin goes in the bits 0..3 and the odd
	 * numbered pin in bits 4..7.
	 */
	if (is_odd) {
		pg->PMUX[idx].bit.PMUXO = func;
	} else {
		pg->PMUX[idx].bit.PMUXE = func;
	}
	pg->PINCFG[pin].bit.PMUXEN = 1;

	return 0;
}

void soc_port_configure(const struct soc_port_pin *pin)
{
	PortGroup *pg = pin->regs;
	uint32_t flags = pin->flags;
	uint32_t func = (pin->flags & SOC_PORT_FUNC_MASK) >> SOC_PORT_FUNC_POS;
	PORT_PINCFG_Type pincfg = { .reg = 0 };

	/* Reset or analog I/O: all digital disabled */
	pg->PINCFG[pin->pinum] = pincfg;
	pg->DIRCLR.reg = (1 << pin->pinum);
	pg->OUTCLR.reg = (1 << pin->pinum);

	if (flags & SOC_PORT_PMUXEN_ENABLE) {
		soc_port_pinmux_set(pg, pin->pinum, func);
		return;
	}

	if (flags & (SOC_PORT_PULLUP | SOC_PORT_PULLDOWN)) {
		if (flags & SOC_PORT_PULLUP) {
			pg->OUTSET.reg = (1 << pin->pinum);
		}

		pincfg.bit.PULLEN = 1;
	}

	if (flags & SOC_PORT_INPUT_ENABLE) {
		pincfg.bit.INEN = 1;
	}

	if (flags & SOC_PORT_OUTPUT_ENABLE) {
		pg->DIRSET.reg = (1 << pin->pinum);
	}

	if (flags & SOC_PORT_STRENGTH_STRONGER) {
		pincfg.bit.DRVSTR = 1;
	}

	pg->PINCFG[pin->pinum] = pincfg;
}

void soc_port_list_configure(const struct soc_port_pin pins[],
			     unsigned int size)
{
	for (int i = 0; i < size; i++) {
		soc_port_configure(&pins[i]);
	}
}
