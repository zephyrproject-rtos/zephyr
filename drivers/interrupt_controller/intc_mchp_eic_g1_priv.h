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
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#define EIC_LINES_PER_PORT 16

/* Pins of one port whose EIC line is offset from pin % EIC_LINES_PER_PORT */
struct eic_mchp_g1_special_pins {
	uint32_t pins;
	uint8_t offset;
	bool subtract;
};

#define EIC_MCHP_G1_SPECIAL_PINS(node_id, prop, sub)                                              \
	{DT_PROP_BY_IDX(node_id, prop, 0), DT_PROP_BY_IDX(node_id, prop, 1), sub}

/* The special-pins groups of each port, in the order they are matched */
#if defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CM_JH)
#define EIC_MCHP_G1_PORTA_SPECIAL_PINS(node_id)                                                    \
	{EIC_MCHP_G1_SPECIAL_PINS(node_id, porta_special_pins_1, false)}
#define EIC_MCHP_G1_PORTC_SPECIAL_PINS(node_id)                                                    \
	{EIC_MCHP_G1_SPECIAL_PINS(node_id, portc_special_pins_1, false),                           \
	 EIC_MCHP_G1_SPECIAL_PINS(node_id, portc_special_pins_2, true)}
#else
#define EIC_MCHP_G1_PORTB_SPECIAL_PINS(node_id)                                                    \
	{EIC_MCHP_G1_SPECIAL_PINS(node_id, portb_special_pins_1, false),                           \
	 EIC_MCHP_G1_SPECIAL_PINS(node_id, portb_special_pins_2, true),                            \
	 EIC_MCHP_G1_SPECIAL_PINS(node_id, portb_special_pins_3, true)}
#define EIC_MCHP_G1_PORTC_SPECIAL_PINS(node_id)                                                    \
	{EIC_MCHP_G1_SPECIAL_PINS(node_id, portc_special_pins_1, false)}
#define EIC_MCHP_G1_PORTD_SPECIAL_PINS(node_id)                                                    \
	{EIC_MCHP_G1_SPECIAL_PINS(node_id, portd_special_pins_2, false),                           \
	 EIC_MCHP_G1_SPECIAL_PINS(node_id, portd_special_pins_1, true),                            \
	 EIC_MCHP_G1_SPECIAL_PINS(node_id, portd_special_pins_3, true)}
#endif

/*
 * EIC line of a pin, given the special-pins groups of its port.
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
				eic_line += groups[i].offset;
			}
			break;
		}
	}

	return eic_line;
}

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_MCHP_EIC_G1_PRIV_H_ */
