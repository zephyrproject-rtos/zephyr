/*
 * Copyright (c) 2023 Martin Kiepfer <mrmarteng@teleschirm.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_LED_WORLDSEMI_WS2812C_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_LED_WORLDSEMI_WS2812C_H_

/*
 * At 7 MHz: 1 bit in 142.86 ns
 * 1090 ns ->  7.6  bits
 *  300 ns ->  2.1  bits
 *  790 ns ->  5.5  bits
 */
#define WS2812C_SPI_FREQ   (7000000U)
#define WS2812C_ZERO_FRAME (0xC0U)
#define WS2812C_ONE_FRAME  (0xFCU)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_LED_WORLDSEMI_WS2812C_H_ */
