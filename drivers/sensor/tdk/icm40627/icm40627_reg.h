/*
 * Copyright (c) 2025 PHYTEC America LLC
 * Author: Florijan Plohl <florijan.plohl@norik.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM40627_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM40627_REG_H_

#include <zephyr/sys/util.h>

/* Helper macros for addressing registers in BANKs 1-4 */
#define REG_ADDRESS_MASK GENMASK(7, 0)
#define REG_BANK_MASK    GENMASK(15, 8)

/* BANK 0 */
#define REG_DEVICE_CONFIG      0x11
#define REG_DRIVE_CONFIG       0x13
#define REG_INT_CONFIG         0x14
#define REG_FIFO_CONFIG        0x16
#define REG_TEMP_DATA1         0x1D
#define REG_ACCEL_DATA_X1      0x1F
#define REG_GYRO_DATA_X1       0x25
#define REG_TMST_FSYNCH        0x2B
#define REG_INT_STATUS         0x2D
#define REG_FIFO_COUNTH        0x2E
#define REG_FIFO_COUNTL        0x2F
#define REG_FIFO_DATA          0x30
#define REG_APEX_DATA0         0x31
#define REG_APEX_DATA1         0x32
#define REG_APEX_DATA2         0x33
#define REG_APEX_DATA3         0x34
#define REG_APEX_DATA4         0x35
#define REG_APEX_DATA5         0x36
#define REG_INT_STATUS2        0x37
#define REG_INT_STATUS3        0x38
#define REG_SIGNAL_PATH_RESET  0x4B
#define REG_INTF_CONFIG0       0x4C
#define REG_INTF_CONFIG1       0x4D
#define REG_PWR_MGMT0          0x4E
#define REG_GYRO_CONFIG0       0x4F
#define REG_ACCEL_CONFIG0      0x50
#define REG_GYRO_CONFIG1       0x51
#define REG_ACCEL_GYRO_CONFIG0 0x52
#define REG_ACCEL_CONFIG1      0x53
#define REG_TMST_CONFIG        0x54
#define REG_APEX_CONFIG0       0x56
#define REG_SMD_CONFIG         0x57
#define REG_FIFO_CONFIG1       0x5F
#define REG_FIFO_CONFIG2       0x60
#define REG_FSYNC_CONFIG       0x62
#define REG_INT_CONFIG0        0x63
#define REG_INT_CONFIG1        0x64
#define REG_INT_SOURCE0        0x65
#define REG_INT_SOURCE1        0x66
#define REG_INT_SOURCE2        0x67
#define REG_INT_SOURCE3        0x68
#define REG_INT_SOURCE4        0x69
#define REG_INT_SOURCE5        0x6A
#define REG_FIFO_LOST_PKT0     0x6C
#define REG_SELF_TEST_CONFIG   0x70
#define REG_WHO_AM_I           0x75
#define REG_REG_BANK_SEL       0x76

/* BANK 1 */
#define REG_GYRO_CONFIG_STATIC2_B1 0x0B
#define REG_GYRO_CONFIG_STATIC3_B1 0x0C
#define REG_GYRO_CONFIG_STATIC4_B1 0x0D
#define REG_GYRO_CONFIG_STATIC5_B1 0x0E
#define REG_XG_ST_DATA_B1          0x5F
#define REG_YG_ST_DATA_B1          0x60
#define REG_ZG_ST_DATA_B1          0x61
#define REG_TMST_VAL0_B1           0x62
#define REG_INTF_CONFIG4_B1        0x7A
#define REG_INTF_CONFIG5_B1        0x7B
#define REG_INTF_CONFIG6_B1        0x7C

/* BANK 2 */
#define REG_ACCEL_CONFIG_STATIC2_B2 0x03
#define REG_ACCEL_CONFIG_STATIC3_B2 0x04
#define REG_ACCEL_CONFIG_STATIC4_B2 0x05
#define REG_ACCEL_CONFIG_STATIC0_B2 0x39
#define REG_XA_ST_DATA_B2           0x3B
#define REG_YA_ST_DATA_B2           0x3C
#define REG_ZA_ST_DATA_B2           0x3D

/* BANK 4 */
#define REG_APEX_CONFIG1_B4    0x40
#define REG_APEX_CONFIG2_B4    0x41
#define REG_APEX_CONFIG3_B4    0x42
#define REG_APEX_CONFIG4_B4    0x43
#define REG_APEX_CONFIG5_B4    0x44
#define REG_APEX_CONFIG6_B4    0x45
#define REG_APEX_CONFIG7_B4    0x46
#define REG_APEX_CONFIG8_B4    0x47
#define REG_APEX_CONFIG9_B4    0x48
#define REG_ACCEL_WOM_X_THR_B4 0x4A
#define REG_ACCEL_WOM_Y_THR_B4 0x4B
#define REG_ACCEL_WOM_Z_THR_B4 0x4C
#define REG_INT_SOURCE6_B4     0x4D
#define REG_INT_SOURCE7_B4     0x4E
#define REG_INT_SOURCE8_B4     0x4F
#define REG_INT_SOURCE9_B4     0x50
#define REG_INT_SOURCE10_B4    0x51
#define REG_OFFSET_USER_0_B4   0x77
#define REG_OFFSET_USER_1_B4   0x78
#define REG_OFFSET_USER_2_B4   0x79
#define REG_OFFSET_USER_3_B4   0x7A
#define REG_OFFSET_USER_4_B4   0x7B
#define REG_OFFSET_USER_5_B4   0x7C
#define REG_OFFSET_USER_6_B4   0x7D
#define REG_OFFSET_USER_7_B4   0x7E
#define REG_OFFSET_USER_8_B4   0x7F

/* Bank0 REG_SIGNAL_PATH_RESET */
#define BIT_FIFO_FLUSH BIT(2)
#define BIT_SOFT_RESET BIT(0)

/* Bank0 REG_INST_STATUS */
#define BIT_STATUS_RESET_DONE_INT BIT(4)

/* Bank0 REG_INT_CONFIG */
#define BIT_INT1_POLARITY      BIT(0)
#define BIT_INT1_DRIVE_CIRCUIT BIT(1)
#define BIT_INT1_MODE          BIT(2)
#define BIT_INT2_POLARITY      BIT(3)
#define BIT_INT2_DRIVE_CIRCUIT BIT(4)
#define BIT_INT2_MODE          BIT(5)

/* Bank0 REG_PWR_MGMT_0 */
#define MASK_ACCEL_MODE      GENMASK(1, 0)
#define BIT_ACCEL_MODE_OFF   0x00
#define BIT_ACCEL_MODE_LPM   0x02
#define BIT_ACCEL_MODE_LNM   0x03
#define MASK_GYRO_MODE       GENMASK(3, 2)
#define BIT_GYRO_MODE_OFF    0x00
#define BIT_GYRO_MODE_STBY   0x01
#define BIT_GYRO_MODE_LNM    0x03
#define BIT_IDLE             BIT(4)
#define BIT_ACCEL_LP_CLK_SEL BIT(7)

/* Bank0 REG_INT_SOURCE0 */
#define BIT_INT_AGC_RDY_INT1_EN    BIT(0)
#define BIT_INT_FIFO_FULL_INT1_EN  BIT(1)
#define BIT_INT_FIFO_THS_INT1_EN   BIT(2)
#define BIT_INT_DRDY_INT1_EN       BIT(3)
#define BIT_INT_RESET_DONE_INT1_EN BIT(4)
#define BIT_INT_PLL_RDY_INT1_EN    BIT(5)
#define BIT_INT_FSYNC_INT1_EN      BIT(6)
#define BIT_INT_ST_INT1_EN         BIT(7)

/* Bank0 REG_INT_STATUS_DATA_RDY_INT */
#define BIT_INT_STATUS_DATA_RDY_INT BIT(3)

/* Bank9 REG_INTF_CONFIG1 */
#define BIT_ACCELL_LP_CLK_SEL BIT(3)
#define MASK_CLKSEL           GENMASK(1, 0)
#define BIT_CLKSEL_INT_RC     0x00
#define BIT_CLKSEL_PLL_OR_RC  0x01
#define BIT_CLKSEL_DISABLE    0x11

/* Bank0 REG_INT_STATUS */
#define BIT_INT_STATUS_AGC_RDY    BIT(0)
#define BIT_INT_STATUS_FIFO_FULL  BIT(1)
#define BIT_INT_STATUS_FIFO_THS   BIT(2)
#define BIT_INT_STATUS_RESET_DONE BIT(4)
#define BIT_INT_STATUS_PLL_RDY    BIT(5)
#define BIT_INT_STATUS_FSYNC      BIT(6)
#define BIT_INT_STATUS_ST         BIT(7)

/* Bank0 REG_INT_STATUS2 */
#define BIT_INT_STATUS_WOM_Z BIT(0)
#define BIT_INT_STATUS_WOM_Y BIT(1)
#define BIT_INT_STATUS_WOM_X BIT(2)
#define BIT_INT_STATUS_SMD   BIT(3)

/* Bank0 REG_INT_STATUS3 */
#define BIT_INT_STATUS_LOWG_DET      BIT(1)
#define BIT_INT_STATUS_FF_DET        BIT(2)
#define BIT_INT_STATUS_TILT_DET      BIT(3)
#define BIT_INT_STATUS_STEP_CNT_OVFL BIT(4)
#define BIT_INT_STATUS_STEP_DET      BIT(5)

/* Bank0 REG_ACCEL_CONFIG0 */
#define MASK_ACCEL_UI_FS_SEL GENMASK(6, 5)
#define BIT_ACCEL_UI_FS_16   0x00
#define BIT_ACCEL_UI_FS_8    0x01
#define BIT_ACCEL_UI_FS_4    0x02
#define BIT_ACCEL_UI_FS_2    0x03
#define MASK_ACCEL_ODR       GENMASK(3, 0)
#define BIT_ACCEL_ODR_8000   0x03
#define BIT_ACCEL_ODR_4000   0x04
#define BIT_ACCEL_ODR_2000   0x05
#define BIT_ACCEL_ODR_1000   0x06
#define BIT_ACCEL_ODR_200    0x07
#define BIT_ACCEL_ODR_100    0x08
#define BIT_ACCEL_ODR_50     0x09
#define BIT_ACCEL_ODR_25     0x0A
#define BIT_ACCEL_ODR_12     0x0B
#define BIT_ACCEL_ODR_6      0x0C
#define BIT_ACCEL_ODR_3      0x0D
#define BIT_ACCEL_ODR_1      0x0E
#define BIT_ACCEL_ODR_500    0x0F

/* Bank0 REG_GYRO_CONFIG0 */
#define MASK_GYRO_UI_FS_SEL GENMASK(6, 5)
#define BIT_GYRO_UI_FS_2000 0x00
#define BIT_GYRO_UI_FS_1000 0x01
#define BIT_GYRO_UI_FS_500  0x02
#define BIT_GYRO_UI_FS_250  0x03
#define BIT_GYRO_UI_FS_125  0x04
#define BIT_GYRO_UI_FS_62   0x05
#define BIT_GYRO_UI_FS_31   0x06
#define BIT_GYRO_UI_FS_15   0x07
#define MASK_GYRO_ODR       GENMASK(3, 0)
#define BIT_GYRO_ODR_8000   0x03
#define BIT_GYRO_ODR_4000   0x04
#define BIT_GYRO_ODR_2000   0x05
#define BIT_GYRO_ODR_1000   0x06
#define BIT_GYRO_ODR_200    0x07
#define BIT_GYRO_ODR_100    0x08
#define BIT_GYRO_ODR_50     0x09
#define BIT_GYRO_ODR_25     0x0A
#define BIT_GYRO_ODR_12     0x0B
#define BIT_GYRO_ODR_500    0x0F

/* misc. defines */
#define WHO_AM_I_ICM40627     0x4E
#define MIN_ACCEL_SENS_SHIFT  10
#define ACCEL_DATA_SIZE       6
#define GYRO_DATA_SIZE        6
#define TEMP_DATA_SIZE        2
#define MCLK_POLL_INTERVAL_US 250
#define MCLK_POLL_ATTEMPTS    100
#define SOFT_RESET_TIME_MS    2

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM40627_REG_H_ */
