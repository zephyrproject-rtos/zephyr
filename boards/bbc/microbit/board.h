/*
 * Copyright (c) 2016 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

/* External edge connector pin mappings to nRF51 GPIO pin numbers.
 * More information:
 * https://www.microbit.co.uk/device/pins
 * https://github.com/microbit-foundation/microbit-reference-design/tree/master/PDF/Schematic%20Print
 */
#define EXT_P0_GPIO_PIN     3   /* P0, Analog in */
#define EXT_P1_GPIO_PIN     2   /* P1, Analog in */
#define EXT_P2_GPIO_PIN     1   /* P2, Analog in */
#define EXT_P3_GPIO_PIN     4   /* P3, Analog in, LED Col 1 */
#define EXT_P4_GPIO_PIN     5   /* P4, Analog in, LED Col 2 */
#define EXT_P5_GPIO_PIN     17  /* P5, Button A */
#define EXT_P6_GPIO_PIN     12  /* P6, LED Col 9 */
#define EXT_P7_GPIO_PIN     11  /* P7, LED Col 8 */
#define EXT_P8_GPIO_PIN     18  /* P8 */
#define EXT_P9_GPIO_PIN     10  /* P9, LED Col 7 */
#define EXT_P10_GPIO_PIN    6   /* P10, Analog in, LED Col 3 */
#define EXT_P11_GPIO_PIN    26  /* P11, Button B */
#define EXT_P12_GPIO_PIN    20  /* P12 */
#define EXT_P13_GPIO_PIN    23  /* P13, SPI1 SCK */
#define EXT_P14_GPIO_PIN    22  /* P14, SPI1 MISO */
#define EXT_P15_GPIO_PIN    21  /* P15, SPI1 MOSI */
#define EXT_P16_GPIO_PIN    16  /* P16 */
/* 17 and 18 are just 3.3V pins */
#define EXT_P19_GPIO_PIN    0   /* P19, I2C1 SCL */
#define EXT_P20_GPIO_PIN    30  /* P20, I2C1 SDA */

#endif /* __INC_BOARD_H */
