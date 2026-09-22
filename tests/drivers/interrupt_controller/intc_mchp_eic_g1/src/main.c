/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "intc_mchp_eic_g1_priv.h"

/* PIC32CM GC00 port B: portb-special-pins-1 = <0x0003ef80 1>, portb-special-pins-2 = <0x1000 6> */
static const struct eic_mchp_g1_special_pins gc00_portb[] = {
	{0x0003ef80, 1, false},
	{0x00001000, 6, false},
};

/* SAM D5x/E5x and PIC32CX SG port B: portb-special-pins-1 = <0x3c000000 2> */
static const struct eic_mchp_g1_special_pins samd5x_portb[] = {
	{0x3c000000, 2, false},
};

/* PIC32CM JH port C: portc-special-pins-1 = <0xef 8>, portc-special-pins-2 = <0xff00 8> */
static const struct eic_mchp_g1_special_pins jh_portc[] = {
	{0x000000ef, 8, false},
	{0x0000ff00, 8, true},
};

#define LINE(pin, groups) eic_mchp_g1_line_from_pin(pin, groups, ARRAY_SIZE(groups))

ZTEST(intc_mchp_eic_g1, test_offset_wraps_to_line_0)
{
	zassert_equal(LINE(15, gc00_portb), 0, "PB15 is EXTINT0");
}

ZTEST(intc_mchp_eic_g1, test_offset_wraps_to_line_2)
{
	zassert_equal(LINE(12, gc00_portb), 2, "PB12 is EXTINT2");
}

ZTEST(intc_mchp_eic_g1, test_offset_without_wrap)
{
	zassert_equal(LINE(26, samd5x_portb), 12, "PB26 is EXTINT12");
	zassert_equal(LINE(29, samd5x_portb), 15, "PB29 is EXTINT15");
	zassert_equal(LINE(0, jh_portc), 8, "PC0 is EXTINT8");
	zassert_equal(LINE(7, jh_portc), 15, "PC7 is EXTINT15");
	zassert_equal(LINE(8, jh_portc), 0, "PC8 is EXTINT0");
}

ZTEST(intc_mchp_eic_g1, test_pin_in_no_group)
{
	zassert_equal(LINE(4, gc00_portb), 4, "PB4 is EXTINT4");
	zassert_equal(LINE(4, samd5x_portb), 4, "PB4 is EXTINT4");
}

ZTEST_SUITE(intc_mchp_eic_g1, NULL, NULL, NULL, NULL, NULL);
