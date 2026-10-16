/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/ztest.h>

#include "intc_mchp_eic_g1_priv.h"

#define EIC_NODE DT_NODELABEL(eic)

static const struct eic_mchp_g1_special_pins portb[] = EIC_MCHP_G1_PORTB_SPECIAL_PINS(EIC_NODE);
static const struct eic_mchp_g1_special_pins portd[] = EIC_MCHP_G1_PORTD_SPECIAL_PINS(EIC_NODE);

/* EXTINT line of each PB and PD pad of the PIC32CM5112GC00100 ATDF, -1 where there is none */
static const int8_t portb_lines[] = {0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 2, 14, 15, 0, 1, 2};
static const int8_t portd_lines[] = {-1, 1,  -1, -1, 2,  3,  -1, 4, 5, 6, 8,
				     9,  10, 11, 12, 13, 14, 15, 0, 1, 2};

ZTEST(intc_mchp_eic_g1, test_portb)
{
	for (int pin = 0; pin < ARRAY_SIZE(portb_lines); pin++) {
		zassert_equal(eic_mchp_g1_line_from_pin(pin, portb, ARRAY_SIZE(portb)),
			      portb_lines[pin], "PB%02d", pin);
	}
}

ZTEST(intc_mchp_eic_g1, test_portd)
{
	for (int pin = 0; pin < ARRAY_SIZE(portd_lines); pin++) {
		if (portd_lines[pin] < 0) {
			continue;
		}
		zassert_equal(eic_mchp_g1_line_from_pin(pin, portd, ARRAY_SIZE(portd)),
			      portd_lines[pin], "PD%02d", pin);
	}
}

ZTEST_SUITE(intc_mchp_eic_g1, NULL, NULL, NULL, NULL, NULL);
