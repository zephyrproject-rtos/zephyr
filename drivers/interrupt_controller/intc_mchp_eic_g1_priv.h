/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Microchip Technology Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_MCHP_EIC_G1_PRIV_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_MCHP_EIC_G1_PRIV_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/util.h>

#define EIC_LINES_PER_PORT 16

/* Pins of one port whose EIC line is offset from pin % EIC_LINES_PER_PORT */
struct eic_mchp_g1_special_pins {
	uint32_t pins;
	uint8_t offset;
	bool subtract;
};

/*
 * EIC line of a pin, given the special-pins groups of its port in the order they are matched.
 * A pin in no group keeps pin % EIC_LINES_PER_PORT.
 */
static inline uint8_t
eic_mchp_g1_line_from_pin(int pin, const struct eic_mchp_g1_special_pins *groups, size_t num_groups)
{
	uint8_t eic_line = pin % EIC_LINES_PER_PORT;

	for (size_t i = 0; i < num_groups; i++) {
		if ((groups[i].pins & BIT(pin)) != 0) {
			if (groups[i].subtract) {
				eic_line -= groups[i].offset;
			} else {
				eic_line = (eic_line + groups[i].offset) % EIC_LINES_PER_PORT;
			}
			break;
		}
	}

	return eic_line;
}

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_MCHP_EIC_G1_PRIV_H_ */
