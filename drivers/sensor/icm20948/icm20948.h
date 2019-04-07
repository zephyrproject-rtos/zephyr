/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM_20498_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM_20498_H_

#include <zephyr/types.h>
#include <device.h>

/* banks */
#define BANK_0                  (0 << 7)
#define BANK_1                  (1 << 7)
#define BANK_2                  (2 << 7)
#define BANK_3                  (3 << 7)

/* registers */
#define ICM20948_REG_WHO_AM_I                   (BANK_0 | 0x00)
#define ICM20948_REG_LPF                        (BANK_0 | 0x01)
#define ICM20948_REG_USER_CTRL                  (BANK_0 | 0x03)
#define ICM20948_REG_LP_CONFIG                  (BANK_0 | 0x05)
#define ICM20948_REG_PWR_MGMT_1                 (BANK_0 | 0x06)
#define ICM20948_REG_PWR_MGMT_2                 (BANK_0 | 0x07)
#define ICM20948_REG_INT_PIN_CFG                (BANK_0 | 0x0F)
#define ICM20948_REG_INT_ENABLE                 (BANK_0 | 0x10)
#define ICM20948_REG_INT_ENABLE_1               (BANK_0 | 0x11)
#define ICM20948_REG_INT_ENABLE_2               (BANK_0 | 0x12)
#define ICM20948_REG_INT_ENABLE_3               (BANK_0 | 0x13)
#define ICM20948_REG_DMP_INT_STATUS             (BANK_0 | 0x18)
#define ICM20948_REG_INT_STATUS                 (BANK_0 | 0x19)
#define ICM20948_REG_INT_STATUS_1               (BANK_0 | 0x1A)
#define ICM20948_REG_INT_STATUS_2               (BANK_0 | 0x1B)
#define ICM20948_REG_SINGLE_FIFO_PRIORITY_SEL   (BANK_0 | 0x26)
#define ICM20948_REG_GYRO_XOUT_H_SH             (BANK_0 | 0x33)
#define ICM20948_REG_TEMPERATURE                (BANK_0 | 0x39)
#define ICM20948_REG_TEMP_CONFIG                (BANK_0 | 0x53)
#define ICM20948_REG_EXT_SLV_SENS_DATA_00       (BANK_0 | 0x3B)
#define ICM20948_REG_EXT_SLV_SENS_DATA_08       (BANK_0 | 0x43)
#define ICM20948_REG_EXT_SLV_SENS_DATA_09       (BANK_0 | 0x44)
#define ICM20948_REG_EXT_SLV_SENS_DATA_10       (BANK_0 | 0x45)
#define ICM20948_REG_FIFO_EN                    (BANK_0 | 0x66)
#define ICM20948_REG_FIFO_EN_2                  (BANK_0 | 0x67)
#define ICM20948_REG_FIFO_RST                   (BANK_0 | 0x68)
#define ICM20948_REG_FIFO_COUNT_H               (BANK_0 | 0x70)
#define ICM20948_REG_FIFO_R_W                   (BANK_0 | 0x72)
#define ICM20948_REG_HW_FIX_DISABLE             (BANK_0 | 0x75)
#define ICM20948_REG_FIFO_CFG                   (BANK_0 | 0x76)
#define ICM20948_REG_ACCEL_XOUT_H_SH            (BANK_0 | 0x2D)
#define ICM20948_REG_ACCEL_XOUT_L_SH            (BANK_0 | 0x2E)
#define ICM20948_REG_ACCEL_YOUT_H_SH            (BANK_0 | 0x2F)
#define ICM20948_REG_ACCEL_YOUT_L_SH            (BANK_0 | 0x30)
#define ICM20948_REG_ACCEL_ZOUT_H_SH            (BANK_0 | 0x31)
#define ICM20948_REG_ACCEL_ZOUT_L_SH            (BANK_0 | 0x32)
#define ICM20948_REG_MEM_START_ADDR             (BANK_0 | 0x7C)
#define ICM20948_REG_MEM_R_W                    (BANK_0 | 0x7D)
#define ICM20948_REG_MEM_BANK_SEL               (BANK_0 |z 0x7E)
#define ICM20948_REG_XA_OFFS_H                  (BANK_1 | 0x14)
#define ICM20948_REG_YA_OFFS_H                  (BANK_1 | 0x17)
#define ICM20948_REG_ZA_OFFS_H                  (BANK_1 | 0x1A)
#define ICM20948_REG_TIMEBASE_CORRECTION_PLL    (BANK_1 | 0x28)
#define ICM20948_REG_TIMEBASE_CORRECTION_RCOSC  (BANK_1 | 0x29)
#define ICM20948_REG_SELF_TEST1                 (BANK_1 | 0x02)
#define ICM20948_REG_SELF_TEST2                 (BANK_1 | 0x03)
#define ICM20948_REG_SELF_TEST3                 (BANK_1 | 0x04)
#define ICM20948_REG_SELF_TEST4                 (BANK_1 | 0x0E)
#define ICM20948_REG_SELF_TEST5                 (BANK_1 | 0x0F)
#define ICM20948_REG_SELF_TEST6                 (BANK_1 | 0x10)
#define ICM20948_REG_GYRO_SMPLRT_DIV            (BANK_2 | 0x00)
#define ICM20948_REG_GYRO_CONFIG_1              (BANK_2 | 0x01)
#define ICM20948_REG_GYRO_CONFIG_2              (BANK_2 | 0x02)
#define ICM20948_REG_XG_OFFS_USR_H              (BANK_2 | 0x03)
#define ICM20948_REG_YG_OFFS_USR_H              (BANK_2 | 0x05)
#define ICM20948_REG_ZG_OFFS_USR_H              (BANK_2 | 0x07)
#define ICM20948_REG_ACCEL_SMPLRT_DIV_1         (BANK_2 | 0x10)
#define ICM20948_REG_ACCEL_SMPLRT_DIV_2         (BANK_2 | 0x11)
#define ICM20948_REG_ACCEL_CONFIG               (BANK_2 | 0x14)
#define ICM20948_REG_ACCEL_CONFIG_2             (BANK_2 | 0x15)
#define ICM20948_REG_PRS_ODR_CONFIG             (BANK_2 | 0x20)
#define ICM20948_REG_PRGM_START_ADDRH           (BANK_2 | 0x50)
#define ICM20948_REG_MOD_CTRL_USR               (BANK_2 | 0x54)
#define ICM20948_REG_I2C_MST_ODR_CONFIG         (BANK_3 | 0x0)
#define ICM20948_REG_I2C_MST_CTRL               (BANK_3 | 0x01)
#define ICM20948_REG_I2C_MST_DELAY_CTRL         (BANK_3 | 0x02)
#define ICM20948_REG_I2C_SLV0_ADDR              (BANK_3 | 0x03)
#define ICM20948_REG_I2C_SLV0_REG               (BANK_3 | 0x04)
#define ICM20948_REG_I2C_SLV0_CTRL              (BANK_3 | 0x05)
#define ICM20948_REG_I2C_SLV0_DO                (BANK_3 | 0x06)
#define ICM20948_REG_I2C_SLV1_ADDR              (BANK_3 | 0x07)
#define ICM20948_REG_I2C_SLV1_REG               (BANK_3 | 0x08)
#define ICM20948_REG_I2C_SLV1_CTRL              (BANK_3 | 0x09)
#define ICM20948_REG_I2C_SLV1_DO                (BANK_3 | 0x0A)
#define ICM20948_REG_I2C_SLV2_ADDR              (BANK_3 | 0x0B)
#define ICM20948_REG_I2C_SLV2_REG               (BANK_3 | 0x0C)
#define ICM20948_REG_I2C_SLV2_CTRL              (BANK_3 | 0x0D)
#define ICM20948_REG_I2C_SLV2_DO                (BANK_3 | 0x0E)
#define ICM20948_REG_I2C_SLV3_ADDR              (BANK_3 | 0x0F)
#define ICM20948_REG_I2C_SLV3_REG               (BANK_3 | 0x10)
#define ICM20948_REG_I2C_SLV3_CTRL              (BANK_3 | 0x11)
#define ICM20948_REG_I2C_SLV3_DO                (BANK_3 | 0x12)
#define ICM20948_REG_I2C_SLV4_CTRL              (BANK_3 | 0x15)

/* ICM20498_REG_WHO_AM_I */

struct icm20948_data
{
    s16_t x_sample;
    s16_t y_sample;
    s16_t z_sample;

};

#endif /* ZEPHYR_DRIVERS_SENSOR_BME280_BME280_H_ */
