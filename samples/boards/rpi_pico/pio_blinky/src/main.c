/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2022 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hardware/pio.h>
#include <hardware/clocks.h>
#include "blink.pio.h"

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    pio->txf[sm] = clock_get_hz(clk_sys) / (2 * freq);
}

void main(void)
{
	PIO pio = pio0;
	uint offset = pio_add_program(pio, &blink_program);

    blink_pin_forever(pio, 0, offset, 25, 3);
}
