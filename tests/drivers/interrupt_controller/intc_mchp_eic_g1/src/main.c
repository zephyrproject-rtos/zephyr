/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/ztest.h>

#include "intc_mchp_eic_g1_priv.h"

#define EIC_NODE DT_NODELABEL(eic)

static const uint8_t porta[] = DT_PROP(EIC_NODE, porta_eic_lines);
static const uint8_t portb[] = DT_PROP(EIC_NODE, portb_eic_lines);
static const uint8_t portc[] = DT_PROP(EIC_NODE, portc_eic_lines);
static const uint8_t portd[] = DT_PROP(EIC_NODE, portd_eic_lines);

/* EXTINT line of each pad of the PIC32CM5112GC00100 ATDF, -1 where there is none */
static const int8_t porta_lines[] = {0, 1, 2,  3,  4,  5,  6,  7,  8, 9, 10,
				     11, 12, 13, 14, 15, 0, 1, 2, 3, 4};
static const int8_t portb_lines[] = {0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 2, 14, 15, 0, 1, 2};
static const int8_t portc_lines[] = {0,  1,  2,  3,  4,  5, 6, -1, -1, 9,
				     10, 11, 12, 13, 14, 15, 0, 1,  2,  3};
static const int8_t portd_lines[] = {-1, 1,  -1, -1, 2,  3,  -1, 4, 5, 6, 8,
				     9,  10, 11, 12, 13, 14, 15, 0, 1, 2};

static void check_port(char port, const uint8_t *table, const int8_t *lines, size_t num_pins)
{
	for (size_t pin = 0; pin < num_pins; pin++) {
		if (lines[pin] < 0) {
			continue;
		}
		zassert_equal(eic_mchp_g1_line_from_pin((int)pin, table), (uint8_t)lines[pin],
			      "P%c%02zu", port, pin);
	}
}

ZTEST(intc_mchp_eic_g1, test_porta)
{
	check_port('A', porta, porta_lines, ARRAY_SIZE(porta_lines));
}

ZTEST(intc_mchp_eic_g1, test_portb)
{
	check_port('B', portb, portb_lines, ARRAY_SIZE(portb_lines));
}

ZTEST(intc_mchp_eic_g1, test_portc)
{
	check_port('C', portc, portc_lines, ARRAY_SIZE(portc_lines));
}

ZTEST(intc_mchp_eic_g1, test_portd)
{
	check_port('D', portd, portd_lines, ARRAY_SIZE(portd_lines));
}

ZTEST_SUITE(intc_mchp_eic_g1, NULL, NULL, NULL, NULL, NULL);
