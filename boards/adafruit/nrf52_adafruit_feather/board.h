/*
 * Copyright (c) 2018 LEDCity AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

/* External edge connector pin mappings to nRF52 GPIO pin numbers.
 * More information:
 * https://learn.adafruit.com/bluefruit-nrf52-feather-learning-guide?view=all#device-pinout
 * https://cdn-learn.adafruit.com/assets/assets/000/039/913/original/microcontrollers_BluefruitnRF52Feather_Rev-F.png?1488978189
 */
#define EXT_A0_GPIO_PIN     2   /* P2, Analog in */
#define EXT_A1_GPIO_PIN     3   /* P3, Analog in */
#define EXT_A2_GPIO_PIN     4   /* P4, Analog in */
#define EXT_A3_GPIO_PIN     5   /* P5, Analog in */
#define EXT_A4_GPIO_PIN     28  /* P28, Analog in */
#define EXT_A5_GPIO_PIN     29  /* P29, Analog in */
#define EXT_A6_GPIO_PIN     30  /* P30, Analog in */
#define EXT_A7_GPIO_PIN     31  /* P31, Analog in */
#define EXT_SCK_GPIO_PIN    12  /* P12 */
#define EXT_MOSI_GPIO_PIN   13  /* P13 */
#define EXT_MISO_GPIO_PIN   14  /* P14 */
#define EXT_RXD_GPIO_PIN    8   /* P8 */
#define EXT_TXD_GPIO_PIN    6   /* P6 */
#define EXT_P16_GPIO_PIN    16  /* P16 */
#define EXT_P15_GPIO_PIN    15  /* P15 */
#define EXT_P7_GPIO_PIN     7   /* P7 */
#define EXT_P11_GPIO_PIN    11  /* P11 */
#define EXT_P27_GPIO_PIN    27  /* P27 */
#define EXT_SCL_GPIO_PIN    26  /* P26 */
#define EXT_SDA_GPIO_PIN    25  /* P25 */


#endif /* __INC_BOARD_H */
