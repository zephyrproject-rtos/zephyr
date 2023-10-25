/* Copyright 2021 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* BMA422 gsensor module for Chrome EC */

/* Chip ID of BMA422 */
#define BMA422_CHIP_ID 0x12

#ifndef __CROS_EC_ACCEL_BMA422_H
#define __CROS_EC_ACCEL_BMA422_H

#include "config.h"

#define BMA422_I2C_ADDR_PRIMARY 0x18
#define BMA422_I2C_ADDR_SECONDARY 0x19
#define BMA422_I2C_BMM150_ADDR 0x10

/* Chip-specific registers */
#define BMA422_CHIP_ID_ADDR 0x00
#define BMA422_CHIP_ID_MIN 0x10
#define BMA422_CHIP_ID_MAX 0x15

#define BMA422_ERROR_ADDR 0x02
#define BMA422_FATAL_ERR_MSK 0x01
#define BMA422_CMD_ERR_POS 1
#define BMA422_CMD_ERR_MSK 0x02
#define BMA422_ERR_CODE_POS 2
#define BMA422_ERR_CODE_MSK 0x1C
#define BMA422_FIFO_ERR_POS 6
#define BMA422_FIFO_ERR_MSK 0x40
#define BMA422_AUX_ERR_POS 7
#define BMA422_AUX_ERR_MSK 0x80

#define BMA422_STATUS_ADDR 0x03
#define BMA422_STAT_DATA_RDY_ACCEL_POS 7
#define BMA422_STAT_DATA_RDY_ACCEL_MSK 0x80

#define BMA422_DATA_0_ADDR 0x0A
#define BMA422_DATA_8_ADDR 0x12

#define BMA422_SENSORTIME_0_ADDR 0x18
#define BMA422_INT_STAT_0_ADDR 0x1C
#define BMA422_INT_STAT_1_ADDR 0x1D
#define BMA422_STEP_CNT_OUT_0_ADDR 0x1E
#define BMA422_HIGH_G_OUT_ADDR 0x1F
#define BMA422_TEMPERATURE_ADDR 0x22

#define BMA422_INT_STATUS_1 0x1D
#define BMA422_FFULL_INT BIT(0)
#define BMA422_FWM_INT BIT(1)
#define BMA422_ACC_DRDY_INT BIT(7)

#define BMA422_FIFO_LENGTH_0_ADDR 0x24
#define BMA422_FIFO_DATA_ADDR 0x26
#define BMA422_ACTIVITY_OUT_ADDR 0x27
#define BMA422_ORIENTATION_OUT_ADDR 0x28

#define BMA422_INTERNAL_STAT 0x2A
#define BMA422_ASIC_INITIALIZED 0x01

#define BMA422_ACCEL_CONFIG_ADDR 0x40
#define BMA422_ACCEL_ODR_POS 0
#define BMA422_ACCEL_ODR_MSK 0x0F
#define BMA422_ACCEL_BW_POS 4
#define BMA422_ACCEL_BW_MSK 0x70
#define BMA422_ACCEL_PERFMODE_POS 7
#define BMA422_ACCEL_PERFMODE_MSK 0x80
#define BMA422_OUTPUT_DATA_RATE_0_78HZ 0x01
#define BMA422_OUTPUT_DATA_RATE_1_56HZ 0x02
#define BMA422_OUTPUT_DATA_RATE_3_12HZ 0x03
#define BMA422_OUTPUT_DATA_RATE_6_25HZ 0x04
#define BMA422_OUTPUT_DATA_RATE_12_5HZ 0x05
#define BMA422_OUTPUT_DATA_RATE_25HZ 0x06
#define BMA422_OUTPUT_DATA_RATE_50HZ 0x07
#define BMA422_OUTPUT_DATA_RATE_100HZ 0x08
#define BMA422_OUTPUT_DATA_RATE_200HZ 0x09
#define BMA422_OUTPUT_DATA_RATE_400HZ 0x0A
#define BMA422_OUTPUT_DATA_RATE_800HZ 0x0B
#define BMA422_OUTPUT_DATA_RATE_1600HZ 0x0C
#define BMA422_ACCEL_OSR4_AVG1 0
#define BMA422_ACCEL_OSR2_AVG2 1
#define BMA422_ACCEL_NORMAL_AVG4 2
#define BMA422_ACCEL_CIC_AVG8 3
#define BMA422_ACCEL_RES_AVG16 4
#define BMA422_ACCEL_RES_AVG32 5
#define BMA422_ACCEL_RES_AVG64 6
#define BMA422_ACCEL_RES_AVG128 7
#define BMA422_CIC_AVG_MODE 0
#define BMA422_CONTINUOUS_MODE 1

#define BMA422_ACCEL_RANGE_ADDR 0x41
#define BMA422_ACCEL_RANGE_POS 0
#define BMA422_ACCEL_RANGE_MSK 0x03
#define BMA422_ACCEL_RANGE_2G 0
#define BMA422_ACCEL_RANGE_4G 1
#define BMA422_ACCEL_RANGE_8G 2
#define BMA422_ACCEL_RANGE_16G 3

#define BMA422_FIFO_CONFIG_0_ADDR 0x48
#define BMA422_FIFO_STOP_ON_FULL BIT(0)
#define BMA422_FIFO_TIME_EN BIT(1)

#define BMA422_FIFO_CONFIG_1_ADDR 0x49
#define BMA422_FIFO_TAG_INT2_EN BIT(2)
#define BMA422_FIFO_TAG_INT1_EN BIT(3)
#define BMA422_FIFO_HEADER_EN BIT(4)
#define BMA422_FIFO_AUX_EN BIT(5)
#define BMA422_FIFO_ACC_EN BIT(6)

#define BMA422_INT1_IO_CTRL_ADDR 0x53
#define BMA422_INT1_OUTPUT_EN BIT(3)

#define BMA422_INT_LATCH_ADDR 0x55
#define BMA422_INT_LATCH BIT(0)

#define BMA422_INT_MAP_DATA_ADDR 0x58
#define BMA422_INT2_DRDY BIT(6)
#define BMA422_INT2_FWM BIT(5)
#define BMA422_INT2_FFULL BIT(4)
#define BMA422_INT1_DRDY BIT(2)
#define BMA422_INT1_FWM BIT(1)
#define BMA422_INT1_FFULL BIT(0)

#define BMA422_RESERVED_REG_5B_ADDR 0x5B
#define BMA422_RESERVED_REG_5C_ADDR 0x5C
#define BMA422_FEATURE_CONFIG_ADDR 0x5E
#define BMA422_INTERNAL_ERROR 0x5F
#define BMA422_IF_CONFIG_ADDR 0x6B
#define BMA422_FOC_ACC_CONF_VAL 0xB7

#define BMA422_NV_CONFIG_ADDR 0x70
#define BMA422_NV_ACCEL_OFFSET_POS 3
#define BMA422_NV_ACCEL_OFFSET_MSK 0x08

#define BMA422_OFFSET_0_ADDR 0x71
#define BMA422_OFFSET_1_ADDR 0x72
#define BMA422_OFFSET_2_ADDR 0x73

#define BMA422_POWER_CONF_ADDR 0x7C
#define BMA422_ADVANCE_POWER_SAVE_POS 0
#define BMA422_ADVANCE_POWER_SAVE_MSK 0x01

#define BMA422_POWER_CTRL_ADDR 0x7D
#define BMA422_ACCEL_ENABLE_POS 2
#define BMA422_ACCEL_ENABLE_MSK 0x04
#define BMA422_ENABLE 0x01
#define BMA422_DISABLE 0x00

#define BMA422_CMD_ADDR 0x7E
#define BMA422_NVM_PROG 0xA0
#define BMA422_FIFO_FLUSH 0xB0
#define BMA422_SOFT_RESET 0xB6

/* Other definitions */
#define BMA422_X_AXIS 0
#define BMA422_Y_AXIS 1
#define BMA422_Z_AXIS 2

#define BMA422_12_BIT_RESOLUTION 12
#define BMA422_14_BIT_RESOLUTION 14
#define BMA422_16_BIT_RESOLUTION 16

/*
 * The max positive value of accel data is 0x07FF, equal to range(g)
 * So, in order to get +1g, divide the 0x07FF by range
 */
#define BMA422_ACC_DATA_PLUS_1G(range) (0x07FF / (range))

/* For offset registers 1LSB - 3.9mg */
#define BMA422_OFFSET_ACC_MULTI_MG (3900 * 1000)
#define BMA422_OFFSET_ACC_DIV_MG 1000000

#define BMA422_FOC_SAMPLE_LIMIT 32

/* Min and Max sampling frequency in mHz */
#define BMA422_ACCEL_MIN_FREQ 12500
#define BMA422_ACCEL_MAX_FREQ MOTION_MAX_SENSOR_FREQUENCY(1600000, 6250)

#define BMA422_RANGE_TO_REG(_range)                              \
	((_range) < 8 ? BMA422_ACCEL_RANGE_2G + ((_range) / 4) : \
			BMA422_ACCEL_RANGE_8G + ((_range) / 16))

#define BMA422_REG_TO_RANGE(_reg)                        \
	((_reg) < BMA422_ACCEL_RANGE_8G ? 2 + (_reg)*2 : \
					8 + ((_reg)-BMA422_ACCEL_RANGE_8G) * 8)

extern const struct accelgyro_drv bma422_accel_drv;

#if defined(CONFIG_ZEPHYR)
#include <zephyr/devicetree.h>

#if DT_NODE_EXISTS(DT_ALIAS(bma422_int))
/*
 * Get the motion sensor ID of the BMA4xx sensor that generates the interrupt.
 * The interrupt is converted to the event and transferred to motion
 * sense task that actually handles the interrupt.
 *
 * Here, we use alias to get the motion sensor ID
 *
 * e.g) base_accel is the label of a child node in /motionsense-sensors
 * aliases {
 *     bma4xx-int = &base_accel;
 * };
 */
#define CONFIG_ACCEL_BMA422_INT_EVENT \
	TASK_EVENT_MOTION_SENSOR_INTERRUPT(SENSOR_ID(DT_ALIAS(bma422_int)))

#include "gpio_signal.h"
void bma422_interrupt(enum gpio_signal signal);
#endif /* DT_NODE_EXISTS */
#endif /* CONFIG_ZEPHYR */

#endif /* __CROS_EC_ACCEL_BMA422_H */