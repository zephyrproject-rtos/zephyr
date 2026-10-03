/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * PIO program for PDM microphone - ported from ArduinoCore-mbed pdm.pio
 *
 * Generates PDM clock on the CLK pin (side-set), samples DIN on the
 * falling edge (clock goes low), accumulates 8 bits, and pushes to the
 * RX FIFO.  The host reads the FIFO via DMA.
 *
 * Clock high  -> push 8 bits if full (iffull noblock keeps running if not)
 * Clock low   -> shift one bit in from DIN
 *
 * Wrap target/wrap at instructions 0-1 so the SM loops continuously.
 */

#ifndef _DMIC_RP2040_PIO_H_
#define _DMIC_RP2040_PIO_H_

#include <hardware/pio.h>
#include <hardware/gpio.h>

/* Program definition -------------------------------------------------------- */

#define pdm_pio_wrap_target 0
#define pdm_pio_wrap        1

/* .program pdm_pio
 *   push iffull noblock  side 1   ; clk high, push 8-bit word when full
 *   in   pins, 1         side 0   ; clk low,  sample one bit from DIN
 */
static const uint16_t pdm_pio_program_instructions[] = {
	0x9040, /* 0: push iffull noblock  side 1 */
	0x4001, /* 1: in   pins, 1         side 0 */
};

static const struct pio_program pdm_pio_program = {
	.instructions = pdm_pio_program_instructions,
	.length       = 2,
	.origin       = -1,
};

static inline pio_sm_config pdm_pio_program_get_default_config(uint offset)
{
	pio_sm_config c = pio_get_default_sm_config();

	sm_config_set_wrap(&c, offset + pdm_pio_wrap_target, offset + pdm_pio_wrap);
	sm_config_set_sideset(&c, 1, false, false);
	return c;
}

/* Initialise and start the PIO state machine -------------------------------- */

static inline void pdm_pio_program_init(PIO pio, uint sm, uint offset,
					 uint clk_pin, uint din_pin,
					 float clk_div)
{
	pio_sm_config c = pdm_pio_program_get_default_config(offset);

	/* Side-set drives the clock pin */
	sm_config_set_sideset_pins(&c, clk_pin);
	/* IN reads from the DIN pin */
	sm_config_set_in_pins(&c, din_pin);
	/* Shift left, no autopush, push threshold = 8 bits */
	sm_config_set_in_shift(&c, false, false, 8);

	sm_config_set_clkdiv(&c, clk_div);

	/* DIN = input, CLK = output */
	pio_sm_set_consecutive_pindirs(pio, sm, din_pin, 1, false);
	pio_sm_set_consecutive_pindirs(pio, sm, clk_pin, 1, true);

	/* Initialise CLK low */
	pio_sm_set_pins_with_mask(pio, sm, 0, 1u << clk_pin);

	pio_gpio_init(pio, clk_pin);

	pio_sm_init(pio, sm, offset, &c);
	pio_sm_set_enabled(pio, sm, true);
}

#endif /* _DMIC_RP2040_PIO_H_ */
