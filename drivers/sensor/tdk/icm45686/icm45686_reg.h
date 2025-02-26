/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM45686_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM45686_REG_H_

#include <zephyr/sys/util.h>

/* ICM45688 Registers */
#define REG_MISC2     0x7F
/* REG_REG_MISC2_DREG_BANK1 */
#define BIT_SOFT_RST  0x02
#define BIT_IREG_DONE 0x01

#define REG_INT1_STATUS0 0x19

/* Bank0 REG_INT_STATUS_DRDY */
#define BIT_INT_STATUS_DATA_DRDY BIT(2)

#define REG_WHO_AM_I       0x72
#define WHO_AM_I_ICM45688S 0xDC//0xEE

#define REG_INT1_CONFIG2                            0x18
/*
 * int2_status_reset_done
 * For UI/AP interface, If this interrupt status bit is enabled, this bit is to flag the occurance
 * of Reset event.
 *
 * 1: Interrupt occurred.
 * 0: Interrupt did not occur.
 *
 * “The interrupt pin de-assertion operation assumes the interrupt status registers assigned to
 * consecutive addresses are read in one single burst transaction.”
 */
#define REG_INT2_STATUS0_INT2_STATUS_RESET_DONE_POS 0x07
#define REG_INT2_STATUS0_INT2_STATUS_RESET_DONE_MASK                                               \
	(0x01 << REG_INT2_STATUS0_INT2_STATUS_RESET_DONE_POS)

#define INT_POLARITY      1
#define INT_DRIVE_CIRCUIT 1
#define INT_MODE          0

/* REG_INT1_CONFIG2_DREG_BANK1 */
#define SHIFT_INT1_MODE     0x01
#define SHIFT_INT1_POLARITY 0x00

#define REG_PWR_MGMT0 0x10

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

#define REG_ACCEL_CONFIG0 0x1b

/* Bank0 REG_ACCEL_CONFIG0 */
#define MASK_ACCEL_UI_FS_SEL GENMASK(6, 4)
#define BIT_ACCEL_UI_FS_32   0x00
#define BIT_ACCEL_UI_FS_16   0x01
#define BIT_ACCEL_UI_FS_8    0x02
#define BIT_ACCEL_UI_FS_4    0x03
#define BIT_ACCEL_UI_FS_2    0x04

#define MASK_ACCEL_ODR     GENMASK(3, 0)
#define BIT_ACCEL_ODR_6400 0x03
#define BIT_ACCEL_ODR_3200 0x04
#define BIT_ACCEL_ODR_1600 0x05
#define BIT_ACCEL_ODR_800  0x06
#define BIT_ACCEL_ODR_400  0x07
#define BIT_ACCEL_ODR_200  0x08
#define BIT_ACCEL_ODR_100  0x09
#define BIT_ACCEL_ODR_50   0x0A
#define BIT_ACCEL_ODR_25   0x0B
#define BIT_ACCEL_ODR_12   0x0C
#define BIT_ACCEL_ODR_6    0x0D
#define BIT_ACCEL_ODR_3    0x0E
#define BIT_ACCEL_ODR_1    0x0F

#define REG_GYRO_CONFIG0 0x1c

/* Bank0 REG_GYRO_CONFIG0 */
#define MASK_GYRO_UI_FS_SEL   GENMASK(7, 4)
#define BIT_GYRO_UI_FS_4000   0x00
#define BIT_GYRO_UI_FS_2000   0x01
#define BIT_GYRO_UI_FS_1000   0x02
#define BIT_GYRO_UI_FS_500    0x03
#define BIT_GYRO_UI_FS_250    0x04
#define BIT_GYRO_UI_FS_125    0x05
#define BIT_GYRO_UI_FS_62_5   0x06
#define BIT_GYRO_UI_FS_31_25  0x07
#define BIT_GYRO_UI_FS_16_625 0x08

#define MASK_GYRO_ODR     GENMASK(3, 0)
#define BIT_GYRO_ODR_6400 0x03
#define BIT_GYRO_ODR_3200 0x04
#define BIT_GYRO_ODR_1600 0x05
#define BIT_GYRO_ODR_800  0x06
#define BIT_GYRO_ODR_400  0x07
#define BIT_GYRO_ODR_200  0x08
#define BIT_GYRO_ODR_100  0x09
#define BIT_GYRO_ODR_50   0x0A
#define BIT_GYRO_ODR_25   0x0B
#define BIT_GYRO_ODR_12   0x0C

#define REG_INT1_CONFIG0 0x16

/* REG_INT1_CONFIG0_DREG_BANK1 */
#define BIT_INT1_STATUS_EN_RESET_DONE   0x80
#define BIT_INT1_STATUS_EN_AUX1_AGC_RDY 0x40
#define BIT_INT1_STATUS_EN_AP_AGC_RDY   0x20
#define BIT_INT1_STATUS_EN_AP_FSYNC     0x10
#define BIT_INT1_STATUS_EN_AUX1_DRDY    0x08
#define BIT_INT1_STATUS_EN_DRDY         0x04
#define BIT_INT1_STATUS_EN_FIFO_THS     0x02
#define BIT_INT1_STATUS_EN_FIFO_FULL    0x01

#define REG_SPI_READ_BIT  BIT(7)
#define REG_ADDRESS_MASK  GENMASK(7, 0)
#define REG_ACCEL_DATA_X1 0x00
#define REG_GYRO_DATA_X1  0x06

/* misc. defines */
#define WHO_AM_I_ICM45686     0x0A
#define MIN_ACCEL_SENS_SHIFT  10
#define ACCEL_DATA_SIZE       6
#define GYRO_DATA_SIZE        6
#define TEMP_DATA_SIZE        2
#define MCLK_POLL_INTERVAL_US 250
#define MCLK_POLL_ATTEMPTS    100
#define SOFT_RESET_TIME_MS    2 /* 1ms + elbow room */

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM45686_REG_H_ */
