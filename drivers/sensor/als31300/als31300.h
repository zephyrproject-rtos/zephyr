/*
 * Copyright (c) 2025 Croxel
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ALS31300_H_
#define ZEPHYR_DRIVERS_SENSOR_ALS31300_H_

#include <zephyr/sys/util.h>

/* ALS31300 Register Definitions */
#define ALS31300_REG_EEPROM_02      0x02
#define ALS31300_REG_EEPROM_03      0x03
#define ALS31300_REG_VOLATILE_27    0x27
#define ALS31300_REG_DATA_28        0x28
#define ALS31300_REG_DATA_29        0x29

/* Customer Access Code */
#define ALS31300_ACCESS_ADDR        0x35
#define ALS31300_ACCESS_CODE        0x2C413534

/* Register 0x02 bit definitions */
#define ALS31300_BW_SELECT_MASK     GENMASK(23, 21)
#define ALS31300_BW_SELECT_SHIFT    21
#define ALS31300_HALL_MODE_MASK     GENMASK(20, 19)
#define ALS31300_HALL_MODE_SHIFT    19
#define ALS31300_CHAN_Z_EN          BIT(8)
#define ALS31300_CHAN_Y_EN          BIT(7)
#define ALS31300_CHAN_X_EN          BIT(6)

/* Register 0x27 bit definitions */
#define ALS31300_SLEEP_MASK         GENMASK(1, 0)
#define ALS31300_SLEEP_ACTIVE       0
#define ALS31300_SLEEP_MODE         1
#define ALS31300_SLEEP_LPDCM        2

/* Register 0x28 bit definitions */
#define ALS31300_X_MSB_MASK         GENMASK(31, 24)
#define ALS31300_Y_MSB_MASK         GENMASK(23, 16)
#define ALS31300_Z_MSB_MASK         GENMASK(15, 8)
#define ALS31300_NEW_DATA_FLAG      BIT(7)
#define ALS31300_INT_FLAG           BIT(6)
#define ALS31300_TEMP_MSB_MASK      GENMASK(5, 0)

/* Register 0x29 bit definitions */
#define ALS31300_X_LSB_MASK         GENMASK(19, 16)
#define ALS31300_Y_LSB_MASK         GENMASK(15, 12)
#define ALS31300_Z_LSB_MASK         GENMASK(11, 8)
#define ALS31300_TEMP_LSB_MASK      GENMASK(5, 0)

/* Register 0x28 bit fields */
#define ALS31300_REG28_TEMP_MSB_MASK            0x0000003F  /* Bits 5:0 */
#define ALS31300_REG28_TEMP_MSB_SHIFT           0

#define ALS31300_REG28_INTERRUPT_MASK           0x00000040  /* Bit 6 */
#define ALS31300_REG28_INTERRUPT_SHIFT          6

#define ALS31300_REG28_NEW_DATA_MASK            0x00000080  /* Bit 7 */
#define ALS31300_REG28_NEW_DATA_SHIFT           7

#define ALS31300_REG28_Z_AXIS_MSB_MASK          0x0000FF00  /* Bits 15:8 */
#define ALS31300_REG28_Z_AXIS_MSB_SHIFT         8

#define ALS31300_REG28_Y_AXIS_MSB_MASK          0x00FF0000  /* Bits 23:16 */
#define ALS31300_REG28_Y_AXIS_MSB_SHIFT         16

#define ALS31300_REG28_X_AXIS_MSB_MASK          0xFF000000  /* Bits 31:24 */
#define ALS31300_REG28_X_AXIS_MSB_SHIFT         24

/* Register 0x29 bit fields */
#define ALS31300_REG29_TEMP_LSB_MASK            0x0000003F  /* Bits 5:0 */
#define ALS31300_REG29_TEMP_LSB_SHIFT           0

#define ALS31300_REG29_HALL_MODE_STATUS_MASK    0x000000C0  /* Bits 7:6 */
#define ALS31300_REG29_HALL_MODE_STATUS_SHIFT   6

#define ALS31300_REG29_Z_AXIS_LSB_MASK          0x00000F00  /* Bits 11:8 */
#define ALS31300_REG29_Z_AXIS_LSB_SHIFT         8

#define ALS31300_REG29_Y_AXIS_LSB_MASK          0x0000F000  /* Bits 15:12 */
#define ALS31300_REG29_Y_AXIS_LSB_SHIFT         12

#define ALS31300_REG29_X_AXIS_LSB_MASK          0x000F0000  /* Bits 19:16 */
#define ALS31300_REG29_X_AXIS_LSB_SHIFT         16

#define ALS31300_REG29_INTERRUPT_WRITE_MASK     0x00100000  /* Bit 20 */
#define ALS31300_REG29_INTERRUPT_WRITE_SHIFT    20

#define ALS31300_REG29_RESERVED_MASK            0xFFE00000  /* Bits 31:21 */
#define ALS31300_REG29_RESERVED_SHIFT           21

/* ALS31300 sensitivity and conversion constants */
#define ALS31300_FULL_SCALE_RANGE_GAUSS         500.0f  /* ±500 gauss full scale */
#define ALS31300_12BIT_RESOLUTION               4096.0f /* 2^12 for 12-bit resolution */
#define ALS31300_12BIT_MAX_POSITIVE             2047    /* 2^11 - 1 */
#define ALS31300_12BIT_MAX_NEGATIVE             -2048   /* -2^11 */

/* ALS31300 EEPROM Register 0x02 bit field definitions */
#define ALS31300_EEPROM_CUSTOMER_EE_MASK        0x0000001F  /* Bits 4:0 */
#define ALS31300_EEPROM_CUSTOMER_EE_SHIFT       0

#define ALS31300_EEPROM_INT_LATCH_EN_MASK       0x00000020  /* Bit 5 */
#define ALS31300_EEPROM_INT_LATCH_EN_SHIFT      5

#define ALS31300_EEPROM_CHANNEL_X_EN_MASK       0x00000040  /* Bit 6 */
#define ALS31300_EEPROM_CHANNEL_X_EN_SHIFT      6

#define ALS31300_EEPROM_CHANNEL_Y_EN_MASK       0x00000080  /* Bit 7 */
#define ALS31300_EEPROM_CHANNEL_Y_EN_SHIFT      7

#define ALS31300_EEPROM_CHANNEL_Z_EN_MASK       0x00000100  /* Bit 8 */
#define ALS31300_EEPROM_CHANNEL_Z_EN_SHIFT      8

#define ALS31300_EEPROM_I2C_THRESHOLD_MASK      0x00000200  /* Bit 9 */
#define ALS31300_EEPROM_I2C_THRESHOLD_SHIFT     9

#define ALS31300_EEPROM_SLAVE_ADDR_MASK         0x0001FC00  /* Bits 16:10 */
#define ALS31300_EEPROM_SLAVE_ADDR_SHIFT        10

#define ALS31300_EEPROM_DISABLE_SLAVE_ADC_MASK  0x00020000  /* Bit 17 */
#define ALS31300_EEPROM_DISABLE_SLAVE_ADC_SHIFT 17

#define ALS31300_EEPROM_I2C_CRC_EN_MASK         0x00040000  /* Bit 18 */
#define ALS31300_EEPROM_I2C_CRC_EN_SHIFT        18

#define ALS31300_EEPROM_HALL_MODE_MASK          0x00180000  /* Bits 20:19 */
#define ALS31300_EEPROM_HALL_MODE_SHIFT         19

#define ALS31300_EEPROM_BW_SELECT_MASK          0x00E00000  /* Bits 23:21 */
#define ALS31300_EEPROM_BW_SELECT_SHIFT         21

#define ALS31300_EEPROM_RESERVED_MASK           0xFF000000  /* Bits 31:24 */
#define ALS31300_EEPROM_RESERVED_SHIFT          24

/* Timing constants */
#define ALS31300_POWER_ON_DELAY_US  600
#define ALS31300_REG_WRITE_DELAY_MS 50

/* ALS31300 sensitivity and conversion constants */
#define ALS31300_12BIT_RESOLUTION               4096.0f /* 2^12 for 12-bit resolution */

/* Sensitivity values (LSB/G) based on full scale range */
#define ALS31300_SENS_500G 500.0f
#define ALS31300_SENS_1000G 1000.0f
#define ALS31300_SENS_2000G 2000.0f

#endif /* ZEPHYR_DRIVERS_SENSOR_ALS31300_H_ */
