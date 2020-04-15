/*
 * Copyright (c) 2020 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA400_BMA400_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA400_BMA400_H_

#include <device.h>
#include <sys/util.h>
#include <zephyr/types.h>
#include <drivers/sensor.h>

#define BMA400_REG_CHIP_ID          0x00
#define BMA400_REG_ERROR            0x02
#define BMA400_REG_STATUS           0x03
#define BMA400_REG_ACC_X_LSB        0x04
#define BMA400_REG_SENSOR_TIME0     0x0A
#define BMA400_REG_EVENT            0x0D
#define BMA400_REG_INT_STAT0        0x0E
#define BMA400_REG_TEMP             0x11
#define BMA400_REG_FIFO_LENGTH0     0x12
#define BMA400_REG_FIFO_DATA        0x14
#define BMA400_REG_STEP_CNT0        0x15
#define BMA400_REG_STEP_STAT        0x18
#define BMA400_REG_ACC_CONFIG0      0x19
#define BMA400_REG_INT_CONFIG0      0x1F
#define BMA400_REG_INT1_MAP         0x21
#define BMA400_REG_INT12_IO_CTRL    0x24
#define BMA400_REG_FIFO_CONFIG0     0x26
#define BMA400_REG_FIFO_PWR_CONFIG  0x29
#define BMA400_REG_AUTOLOWPOW_0     0x2A
#define BMA400_REG_AUTOWAKEUP_1     0x2C
#define BMA400_REG_WKUP_IN_CONFIG0  0x2F
#define BMA400_REG_ORIENTCH_CONFIG0 0x35
#define BMA400_REG_GEN1INT_CONFIG0  0x3F
#define BMA400_REG_GEN2INT_CONFIG0  0x4A
#define BMA400_REG_ACTH_CONFIG0     0x55
#define BMA400_REG_TAP_CONFIG       0x57
#define BMA400_REG_IF_CONFIG        0x7C
#define BMA400_REG_SELF_TEST        0x7D
#define BMA400_REG_CMD              0x7E

#define BMA400_CHIP_ID 0x90

#define BMA400_CMD_SOFT_RESET 0xB6
#define BMA400_CMD_FIFO_FLUSH OxB0

#define BMA400_PERIOD_START_UP   1000U
#define BMA400_PERIOD_SOFT_RESET 5000U

/* ODR configurations  */
#define BMA400_ODR_MSK    0x0F
#define BMA400_ODR_12_5HZ 0x05
#define BMA400_ODR_25HZ   0x06
#define BMA400_ODR_50HZ   0x07
#define BMA400_ODR_100HZ  0x08
#define BMA400_ODR_200HZ  0x09
#define BMA400_ODR_400HZ  0x0A
#define BMA400_ODR_800HZ  0x0B

/* Range configurations */
#define BMA400_RANGE_MSK 0xC0
#define BMA400_RANGE_POS 6
#define BMA400_RANGE_2G  0x00
#define BMA400_RANGE_4G  0x01
#define BMA400_RANGE_8G  0x02
#define BMA400_RANGE_16G 0x03

/* Data source configurations */
#define BMA400_DATA_SRC_MSK          0x0C
#define BMA400_DATA_SRC_POS          2
#define BMA400_DATA_SRC_VAR_ODR      0x00
#define BMA400_DATA_SRC_100HZ        0x01
#define BMA400_DATA_SRC_100HZ_BW_1HZ 0x02

/* Oversampling ratio (OSR,OSR_LP) configurations */
#define BMA400_OSR_MSK     0x30
#define BMA400_OSR_POS     4
#define BMA400_OSR_LP_MSK  0x60
#define BMA400_OSR_LP_POS  5
#define BMA400_OSR_LOWEST  0x00
#define BMA400_OSR_LOW     0x01
#define BMA400_OSR_HIGH    0x02
#define BMA400_OSR_HIGHEST 0x03

/* Filter1_bandwidth configurations */
#define BMA400_FILT1_BW_MSK      0x80
#define BMA400_FILT1_BW_POS      7
#define BMA400_FILT1_BW_0_48_ODR 0x00
#define BMA400_FILT1_BW_0_24_ODR 0x01

/* Power mode configurations */
#define BMA400_POWER_MODE_MSK       0x03
#define BMA400_POWER_MODE_NORMAL    0x02
#define BMA400_POWER_MODE_SLEEP     0x00
#define BMA400_POWER_MODE_LOW_POWER 0x01

#define BMA400_SET_BITS(reg_data, bitname, data)       \
	((reg_data & ~(bitname##_MSK)) | ((data << bitname##_POS) \
	& bitname##_MSK))
#define BMA400_SET_BITS_POS_0(reg_data, bitname, data) \
	((reg_data & ~(bitname##_MSK)) | (data & bitname##_MSK))

/*
 * Accelerometer configuration
 */
struct bma400_acc_cfg {
	u8_t osr_lp;
	u8_t filt1_bw;
	u8_t odr;
	u8_t osr;
	u8_t range;
	u8_t data_src;
};

struct bma400_data {
	s16_t x, y, z;
};

struct bma400_dev {
	struct device *i2c;
	u8_t dev_id;
	u8_t range;
	double last_odr, last_range, last_osr;
	struct bma400_data data;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA400_BMA400_H_ */
