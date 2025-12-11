/*
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA4XX_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA4XX_REG_H_

#include <zephyr/sys/util.h>

/*
 * Register definitions
 */

#define BMA4XX_REG_CHIP_ID         (0x00)
#define BMA4XX_REG_ERROR           (0x02)
#define BMA4XX_REG_STATUS          (0x03)
#define BMA4XX_REG_DATA_0          (0x0A)
#define BMA4XX_REG_DATA_1          (0x0B)
#define BMA4XX_REG_DATA_2          (0x0C)
#define BMA4XX_REG_DATA_3          (0x0D)
#define BMA4XX_REG_DATA_4          (0x0E)
#define BMA4XX_REG_DATA_5          (0x0F)
#define BMA4XX_REG_DATA_6          (0x10)
#define BMA4XX_REG_DATA_7          (0x11)
#define BMA4XX_REG_DATA_8          (0x12)
#define BMA4XX_REG_DATA_9          (0x13)
#define BMA4XX_REG_DATA_10         (0x14)
#define BMA4XX_REG_DATA_11         (0x15)
#define BMA4XX_REG_DATA_12         (0x16)
#define BMA4XX_REG_DATA_13         (0x17)
#define BMA4XX_REG_SENSORTIME_0    (0x18)
#define BMA4XX_REG_EVENT           (0x1B)
#define BMA4XX_REG_INT_STAT_0      (0x1C)
#define BMA4XX_REG_INT_STAT_1      (0x1D)
#define BMA4XX_REG_STEP_CNT_OUT_0  (0x1E)
#define BMA4XX_REG_HIGH_G_OUT      (0x1F)
#define BMA4XX_REG_TEMPERATURE     (0x22)
#define BMA4XX_REG_FIFO_LENGTH_0   (0x24)
#define BMA4XX_REG_FIFO_LENGTH_1   (0x25)
#define BMA4XX_REG_FIFO_DATA       (0x26)
#define BMA4XX_REG_ACTIVITY_OUT    (0x27)
#define BMA4XX_REG_ORIENTATION_OUT (0x28)
#define BMA4XX_REG_ACCEL_CONFIG    (0x40)
#define BMA4XX_REG_ACCEL_RANGE     (0x41)
#define BMA4XX_REG_AUX_CONFIG      (0x44)
#define BMA4XX_REG_FIFO_DOWN       (0x45)
#define BMA4XX_REG_FIFO_WTM_0      (0x46)
#define BMA4XX_REG_FIFO_WTM_1      (0x47)
#define BMA4XX_REG_FIFO_CONFIG_0   (0x48)
#define BMA4XX_REG_FIFO_CONFIG_1   (0x49)
#define BMA4XX_REG_AUX_DEV_ID      (0x4B)
#define BMA4XX_REG_AUX_IF_CONF     (0x4C)
#define BMA4XX_REG_AUX_RD          (0x4D)
#define BMA4XX_REG_AUX_WR          (0x4E)
#define BMA4XX_REG_AUX_WR_DATA     (0x4F)
#define BMA4XX_REG_INT1_IO_CTRL    (0x53)
#define BMA4XX_REG_INT2_IO_CTRL    (0x54)
#define BMA4XX_REG_INT_LATCH       (0x55)
#define BMA4XX_REG_INT_MAP_1       (0x56)
#define BMA4XX_REG_INT_MAP_2       (0x57)
#define BMA4XX_REG_INT_MAP_DATA    (0x58)
#define BMA4XX_REG_INIT_CTRL       (0x59)
#define BMA4XX_REG_RESERVED_REG_5B (0x5B)
#define BMA4XX_REG_RESERVED_REG_5C (0x5C)
#define BMA4XX_REG_FEATURE_CONFIG  (0x5E)
#define BMA4XX_REG_IF_CONFIG       (0x6B)
#define BMA4XX_REG_ACC_SELF_TEST   (0x6D)
#define BMA4XX_REG_NV_CONFIG       (0x70)
#define BMA4XX_REG_OFFSET_0        (0x71)
#define BMA4XX_REG_OFFSET_1        (0x72)
#define BMA4XX_REG_OFFSET_2        (0x73)
#define BMA4XX_REG_POWER_CONF      (0x7C)
#define BMA4XX_REG_POWER_CTRL      (0x7D)
#define BMA4XX_REG_CMD             (0x7E)

/* BMA4xx I2C register */
#define BMA4XX_REG_I2C_SDO_HIGH (0x19)
#define BMA4XX_REG_I2C_SDO_LOW  (0x18)

/*
 * Bit positions and masks
 */

#define BMA4XX_REG_ADDRESS_MASK GENMASK(7, 0)

#define BMA4XX_BIT_ADV_PWR_SAVE BIT(0)

#define BMA4XX_MASK_ACC_CONF_ODR  GENMASK(3, 0)
#define BMA4XX_MASK_ACC_CONF_BWP  GENMASK(6, 4)
#define BMA4XX_SHIFT_ACC_CONF_BWP (4)

#define BMA4XX_MASK_ACC_RANGE GENMASK(1, 0)

#define BMA4XX_BIT_ACC_PERF_MODE BIT(7)

/* CMD: Clears all data in FIFO, does not change FIFO_CONFIG and FIFO_DOWNS register */
#define BMA4XX_CMD_FIFO_FLUSH (0xB0)

/* BMA4xx EVENT bit definition */
#define BMA4XX_BIT_POR_DETECTED BIT(0)

/* BMA4xx INT1_IO_CTRL bit definition */
#define BMA4XX_BIT_INT1_IO_CTRL_EDGE_CTRL BIT(0)
#define BMA4XX_BIT_INT1_IO_CTRL_LVL       BIT(1)
#define BMA4XX_BIT_INT1_IO_CTRL_OD        BIT(2)
#define BMA4XX_BIT_INT1_IO_CTRL_OUTPUT_EN BIT(3)
#define BMA4XX_BIT_INT1_IO_CTRL_INPUT_EN  BIT(4)

/* BMA4xx INT_MAP_DATA bit definition */
#define BMA4XX_BIT_INT_MAP_DATA_INT1_FFUL BIT(0)
#define BMA4XX_BIT_INT_MAP_DATA_INT1_FWM  BIT(1)
#define BMA4XX_BIT_INT_MAP_DATA_INT1_DRDY BIT(2)

/* BMA4xx INT_STATUS_1 bit definition */
#define BMA4XX_BIT_INT_STAT_1_FFULL_INT    BIT(0)
#define BMA4XX_BIT_INT_STAT_1_FWM_INT      BIT(1)
#define BMA4XX_BIT_INT_STAT_1_ACC_DRDY_INT BIT(7)

/* BMA4xx PWR_CTRL bit definition */
#define BMA4XX_BIT_POWER_CTRL_AUX_EN BIT(0)
#define BMA4XX_BIT_POWER_CTRL_ACC_EN BIT(2)

/* BMA4xx PWR_CONF bit definition */
#define BMA4XX_BIT_POWER_CONF_ADV_PWR_SAVE BIT(0)

/* BMA4xx EVENT bit definition */
#define BMA4XX_BIT_EVENT_POR_DETECTED BIT(0)

/* BMA4xx FIFO_CONFIG_1 bit definition */
#define BMA4XX_FIFO_ACC_EN    BIT(6)
#define BMA4XX_FIFO_AUX_EN    BIT(5)
#define BMA4XX_FIFO_HEADER_EN BIT(4)

/* Bandwidth parameters */
#define BMA4XX_BWP_OSR4_AVG1  (0x0)
#define BMA4XX_BWP_OSR2_AVG2  (0x1)
#define BMA4XX_BWP_NORM_AVG4  (0x2)
#define BMA4XX_BWP_CIC_AVG8   (0x3)
#define BMA4XX_BWP_RES_AVG16  (0x4)
#define BMA4XX_BWP_RES_AVG32  (0x5)
#define BMA4XX_BWP_RES_AVG64  (0x6)
#define BMA4XX_BWP_RES_AVG128 (0x7)

/* Full-scale ranges */
#define BMA4XX_RANGE_2G  (0x0)
#define BMA4XX_RANGE_4G  (0x1)
#define BMA4XX_RANGE_8G  (0x2)
#define BMA4XX_RANGE_16G (0x3)

/* Output data rates (ODR) */
#define BMA4XX_ODR_RESERVED (0x00)
#define BMA4XX_ODR_0_78125  (0x01)
#define BMA4XX_ODR_1_5625   (0x02)
#define BMA4XX_ODR_3_125    (0x03)
#define BMA4XX_ODR_6_25     (0x04)
#define BMA4XX_ODR_12_5     (0x05)
#define BMA4XX_ODR_25       (0x06)
#define BMA4XX_ODR_50       (0x07)
#define BMA4XX_ODR_100      (0x08)
#define BMA4XX_ODR_200      (0x09)
#define BMA4XX_ODR_400      (0x0a)
#define BMA4XX_ODR_800      (0x0b)
#define BMA4XX_ODR_1600     (0x0c)
#define BMA4XX_ODR_3200     (0x0d)
#define BMA4XX_ODR_6400     (0x0e)
#define BMA4XX_ODR_12800    (0x0f)

/* BMA4xx soft-reset commands */
#define BMA4XX_CMD_SOFT_RESET (0xB6)

/* BMA4xx chip id */
#define BMA4XX_CHIP_ID_BMA422 (0x12)
#define BMA4XX_CHIP_ID_BMA423 (0x13)

#define BMA4XX_REG_I2C_READ_BIT BIT(7)

/*
 * BMA4xx FIFO Header bit definition
 */

/* fh_parm, regular */
#define BMA4XX_BIT_FIFO_HEADER_ACCEL BIT(2)
#define BMA4XX_BIT_FIFO_HEADER_AUX   BIT(4)

/* fh_mode */
#define BMA4XX_BIT_FIFO_HEADER_CONTROL BIT(6)
#define BMA4XX_BIT_FIFO_HEADER_REGULAR BIT(7)

/* fh_parm, control */
#define BMA4XX_BIT_FIFO_HEADER_SENSORTIME  BIT(2)
#define BMA4XX_BIT_FIFO_HEAD_OVER_READ_MSB BIT(5)

/*
 * Other constants
 */

/* BMA4xx FIFO Length definition */
#define BMA4XX_FIFO_HEADER_LENGTH UINT8_C(1)
#define BMA4XX_FIFO_A_LENGTH      UINT8_C(6)
#define BMA4XX_FIFO_M_LENGTH      UINT8_C(8)
#define BMA4XX_FIFO_MA_LENGTH     UINT8_C(14)
#define BMA4XX_FIFO_ST_LENGTH     UINT8_C(3)
#define BMA4XX_FIFO_CF_LENGTH     UINT8_C(1)
#define BMA4XX_FIFO_DATA_LENGTH   UINT8_C(2)

/* Each bit count is 3.9mG or 3900uG */
#define BMA4XX_OFFSET_MICROG_PER_BIT (3900)
#define BMA4XX_OFFSET_MICROG_MIN     (INT8_MIN * BMA4XX_OFFSET_MICROG_PER_BIT)
#define BMA4XX_OFFSET_MICROG_MAX     (INT8_MAX * BMA4XX_OFFSET_MICROG_PER_BIT)

/* Range is -104C to 150C. Use shift of 8 (+/-256) */
#define BMA4XX_TEMP_SHIFT (8)

/* The total number of used registers specified in bma422 datasheet is 7E */
#define BMA4XX_NUM_REGS 0x7E

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA4XX_REG_H_ */
