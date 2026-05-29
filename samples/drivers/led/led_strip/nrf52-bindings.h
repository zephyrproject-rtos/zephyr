/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_DRIVERS_LED_WS2812_H_
#define ZEPHYR_SAMPLES_DRIVERS_LED_WS2812_H_

/*
 * At 4 MHz, 1 bit is 250 ns, so 3 bits is 750 ns.
 *
 * That's cutting it a bit close to the edge of the timing parameters,
 * but it seems to work on AdaFruit LED rings.
 */
#define SPI_FREQ    4000000
#define ZERO_FRAME  0x40
#define ONE_FRAME   0x70

#endif
