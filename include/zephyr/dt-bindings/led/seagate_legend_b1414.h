/*
 * Copyright (c) 2021, Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_DRIVERS_LED_B1414_H_
#define ZEPHYR_SAMPLES_DRIVERS_LED_B1414_H_

/*
 * At 6 MHz: 1 bit in 166.666 ns
 * 1200 ns ->  7.2  bits
 *  300 ns ->  1.8  bits
 *  900 ns ->  5.4  bits
 */
#define SPI_FREQ	6000000
#define ZERO_FRAME	0x60
#define ONE_FRAME	0x7C

#endif
