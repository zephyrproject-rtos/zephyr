/*
 * Copyright (c) 2020 Jefferson Lee.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
/*
 * This file will map the arduino pin names to their pin number
 * Definitions are copied from here:
 * https://github.com/arduino/ArduinoCore-nRF528x-mbedos/blob/6216632cc70271619ad43547c804dabb4afa4a00/variants/ARDUINO_NANO33BLE/variant.cpp#L77
 * (in the array g_APinDescription)
 * The pin number is derived using the formula:
 * Given GPIO pin PB.A
 * Pin Number = B*32 + A
 * ex. P1.3 => 35
 */


#define ARDUINO_D0_TX (32 * 1 + 3)                      /* P1.3 */
#define ARDUINO_D1_RX (32 * 1 + 10)                     /* P1.10 */
#define ARDUINO_D2 (32 * 1 + 11)                        /* P1.11 */
#define ARDUINO_D3 (32 * 1 + 12)                        /* P1.12 */
#define ARDUINO_D4 (32 * 1 + 15)                        /* P1.15 */
#define ARDUINO_D5 (32 * 1 + 13)                        /* P1.13 */
#define ARDUINO_D6 (32 * 1 + 14)                        /* P1.14 */
#define ARDUINO_D7 (32 * 0 + 23)                        /* P0.23 */

#define ARDUINO_D8 (32 * 0 + 21)                        /* P0.21 */
#define ARDUINO_D9 (32 * 0 + 27)                        /* P0.27 */
#define ARDUINO_D10 (32 * 1 + 2)                        /* P1.2 */
#define ARDUINO_D11_MOSI (32 * 1 + 1)                   /* P1.1 */
#define ARDUINO_D12_MISO (32 * 1 + 8)                   /* P1.8 */
#define ARDUINO_D13_SCK (32 * 0 + 13)                   /* P0.13 */

#define ARDUINO_A0 (32 * 0 + 24)                        /* P0.24 */
#define ARDUINO_A1 (32 * 0 + 5)                         /* P0.5 */
#define ARDUINO_A2 (32 * 0 + 30)                        /* P0.30 */
#define ARDUINO_A3 (32 * 0 + 29)                        /* P0.29 */
#define ARDUINO_A4_SDA (32 * 0 + 31)                    /* P0.31 */
#define ARDUINO_A5_SCL (32 * 0 + 2)                     /* P0.2 */
#define ARDUINO_A6 (32 * 0 + 28)                        /* P0.28 */
#define ARDUINO_A7 (32 * 0 + 3)                         /* P0.3 */

#define ARDUINO_LEDR (32 * 0 + 24)                      /* P0.24 */
#define ARDUINO_LEDG (32 * 0 + 16)                      /* P0.16 */
#define ARDUINO_LEDB (32 * 0 + 6)                       /* P0.6 */
#define ARDUINO_LEDPWR (32 * 1 + 9)                     /* P1.9 */

#define ARDUINO_INT_APDS (32 * 0 + 19)                  /* P0.19 */

#define ARDUINO_PDM_PWR (32 * 0 + 17)                   /* P0.17 */
#define ARDUINO_PDM_CLK (32 * 0 + 26)                   /* P0.26 */
#define ARDUINO_PDM_DIN (32 * 0 + 25)                   /* P0.25 */

#define ARDUINO_SDA1 (32 * 0 + 14)                      /* P0.14 */
#define ARDUINO_SCL1 (32 * 0 + 15)                      /* P0.15 */
#define ARDUINO_INTERNAL_I2C_PULLUP (32 * 1 + 0)        /* P1.0 */
#define ARDUINO_INTERNAL_VDD_ENV_ENABLE (32 * 0 + 22)   /* P0.22 */
