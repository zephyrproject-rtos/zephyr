/*
 * Copyright (c) 2021 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_DRIVERS_LED_WS2812_F070RB_BINDINGS_H
#define ZEPHYR_SAMPLES_DRIVERS_LED_WS2812_F070RB_BINDINGS_H

/*
 * Everlight B1414 LED controller;
 *
 * Each bit of the control signal (waveform) is described with a 1.2 us pulse:
 * 0 bit: 300 ns high and 900 ns low.
 * 1 bit: 900 ns high and 300 ns low.
 *
 * At 6 MHz, one bit represents 166.666 ns.
 * 1200 ns ->  7.2  bits
 *  300 ns ->  1.8  bits
 *  900 ns ->  5.4  bits
 */
#define B1414_SPI_FREQ		6000000
#define B1414_ZERO_FRAME	0x60
#define B1414_ONE_FRAME		0x7C

#endif
