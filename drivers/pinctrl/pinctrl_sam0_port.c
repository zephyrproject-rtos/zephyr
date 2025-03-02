/*
 * Copyright (c) 2016 Piotr Mienkowski
 * Copyright (c) 2018 Google LLC.
 * Copyright (c) 2021 Gerson Fernando Budke
 * Copyright (c) 2025 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM0 MCU family I/O Pin Controller (PORT)
 */

#include <zephyr/kernel.h>
#include <stdbool.h>

#include "pinctrl_sam0_port.h"

int soc_port_pinmux_set(uintptr_t pg, uint32_t pin, uint32_t func)
{
	bool is_odd = pin & 1;
	int idx = pin / 2U;

	/* Each pinmux register holds the config for two pins.  The
	 * even numbered pin goes in the bits 0..3 and the odd
	 * numbered pin in bits 4..7.
	 */
	uint8_t tmp = sys_read8(pg + PMUX_OFFSET + idx);
	if (is_odd) {
		tmp |= FIELD_PREP(PMUX_PMUXO_MASK, func);
	} else {
		tmp |= FIELD_PREP(PMUX_PMUXE_MASK, func);
	}
	sys_write8(tmp, pg + PMUX_OFFSET + idx);

	tmp = sys_read8(pg + PINCFG_OFFSET + pin);
	WRITE_BIT(tmp, PINCFG_PMUXEN_BIT, 1);
	sys_write8(tmp, pg + PINCFG_OFFSET + pin);

	return 0;
}

void soc_port_configure(const struct soc_port_pin *pin)
{
	uintptr_t pg = pin->regs;
	uint32_t flags = pin->flags;
	uint32_t func = (pin->flags & SOC_PORT_FUNC_MASK) >> SOC_PORT_FUNC_POS;
	uint8_t pincfg = 0;

	/* Reset or analog I/O: all digital disabled */
	sys_write8(pincfg, pg + PINCFG_OFFSET + pin->pinum);
	sys_write32((1 << pin->pinum), pg + DIRCLR_OFFSET);
	sys_write32((1 << pin->pinum), pg + OUTCLR_OFFSET);

	if (flags & SOC_PORT_PMUXEN_ENABLE) {
		soc_port_pinmux_set(pg, pin->pinum, func);
		return;
	}

	if (flags & (SOC_PORT_PULLUP | SOC_PORT_PULLDOWN)) {
		if (flags & SOC_PORT_PULLUP) {
			sys_write32((1 << pin->pinum), pg + OUTSET_OFFSET);
		}

		WRITE_BIT(pincfg, PINCFG_PULLEN_BIT, 1);
	}

	if (flags & SOC_PORT_INPUT_ENABLE) {
		WRITE_BIT(pincfg, PINCFG_INEN_BIT, 1);
	}

	if (flags & SOC_PORT_OUTPUT_ENABLE) {
		sys_write32((1 << pin->pinum), pg + DIRSET_OFFSET);
	}

	if (flags & SOC_PORT_STRENGTH_STRONGER) {
		WRITE_BIT(pincfg, PINCFG_DRVSTR_BIT, 1);
	}

	sys_write8(pincfg, pg + PINCFG_OFFSET + pin->pinum);
}

void soc_port_list_configure(const struct soc_port_pin pins[],
			     unsigned int size)
{
	for (int i = 0; i < size; i++) {
		soc_port_configure(&pins[i]);
	}
}
