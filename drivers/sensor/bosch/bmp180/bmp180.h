/* Bosch BMP180 pressure sensor
 *
 * Copyright (c) 2024 Chris Ruehl
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.mouser.hk/datasheet/2/783/BST-BMP180-DS000-1509579.pdf
 */
#ifndef ZEPHYR_DRIVER_SENSORS_BMP180_H
#define ZEPHYR_DRIVER_SENSORS_BMP180_H

/* Registers */
#define BMP180_REG_CHIPID       0xD0
#define BMP180_REG_CMD          0xE0
#define BMP180_REG_MEAS_CTRL    0xF4
#define BMP180_REG_MSB          0xF6
#define BMP180_REG_LSB          0xF7
#define BMP180_REG_XLSB         0xF8
#define BMP180_REG_CALIB0       0xAA
#define BMP180_REG_CALIB21      0xBF

/* BMP180_REG_CHIPID */
#define BMP180_CHIP_ID 0x55

/* BMP180_REG_STATUS */
#define BMP180_STATUS_CMD_RDY    BIT(5)

/* BMP180_REG_CMD */
#define BMP180_CMD_SOFT_RESET 0xB6
#define BMP180_CMD_GET_TEMPERATURE 0x2E
#define BMP180_CMD_GET_OSS0_PRESS  0x34
#define BMP180_CMD_GET_OSS1_PRESS  0x74 /* 0x34 | OSR<<6 */
#define BMP180_CMD_GET_OSS2_PRESS  0xB4
#define BMP180_CMD_GET_OSS3_PRESS  0xF4

/* command result waiting time in ms */
#define BMP180_CMD_GET_TEMP_DELAY  3
#define BMP180_CMD_GET_OSS0_DELAY  3
#define BMP180_CMD_GET_OSS1_DELAY  6
#define BMP180_CMD_GET_OSS2_DELAY  12
#define BMP180_CMD_GET_OSS3_DELAY  24

#define BMP180_ULTRALOWPOWER 0x00  /* oversampling 1x */
#define BMP180_STANDARD      0x01  /* oversampling 2x */
#define BMP180_HIGHRES       0x02  /* oversampling 4x */
#define BMP180_ULTRAHIGH     0x03  /* oversampling 8x */

#endif /* ZEPHYR_DRIVER_SENSORS_BMP180_H */
