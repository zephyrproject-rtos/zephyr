/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_APDS9253_APDS9253_H_
#define ZEPHYR_DRIVERS_SENSOR_APDS9253_APDS9253_H_

#include <zephyr/drivers/gpio.h>

#define APDS9253_MAIN_CTRL_REG      0x00
#define APDS9253_MAIN_CTRL_REG_MASK GENMASK(5, 0)
#define APDS9253_MAIN_CTRL_SAI_LS   BIT(5)
#define APDS9253_MAIN_CTRL_SW_RESET BIT(4)
#define APDS9253_MAIN_CTRL_RGB_MODE BIT(2)
#define APDS9253_MAIN_CTRL_LS_EN    BIT(1)

#define APDS9253_LS_MEAS_RATE_REG             0x04
#define APDS9253_LS_MEAS_RATE_RES_MASK        GENMASK(6, 4)
#define APDS9253_LS_MEAS_RATE_RES_20BIT_400MS 0
#define APDS9253_LS_MEAS_RATE_RES_19BIT_200MS BIT(4)
#define APDS9253_LS_MEAS_RATE_RES_18BIT_100MS BIT(5) /* default */
#define APDS9253_LS_MEAS_RATE_RES_17BIT_50MS  (BIT(5) | BIT(4))
#define APDS9253_LS_MEAS_RATE_RES_16BIT_25MS  BIT(6)
#define APDS9253_LS_MEAS_RATE_RES_13_3MS      (BIT(6) | BIT(4))
#define APDS9253_LS_MEAS_RATE_MES_MASK        GENMASK(2, 0)
#define APDS9253_LS_MEAS_RATE_MES_2000MS      (BIT(2) | BIT(1) | BIT(0))
#define APDS9253_LS_MEAS_RATE_MES_1000MS      (BIT(2) | BIT(0))
#define APDS9253_LS_MEAS_RATE_MES_500MS       BIT(2)
#define APDS9253_LS_MEAS_RATE_MES_200MS       (BIT(1) | BIT(0))
#define APDS9253_LS_MEAS_RATE_MES_100MS       BIT(1) /* default */
#define APDS9253_LS_MEAS_RATE_MES_50MS        BIT(0)
#define APDS9253_LS_MEAS_RATE_MES_25MS        0

#define APDS9253_LS_GAIN_REG      0x05
#define APDS9253_LS_GAIN_MASK     GENMASK(2, 0)
#define APDS9253_LS_GAIN_RANGE_18 BIT(2)
#define APDS9253_LS_GAIN_RANGE_9  (BIT(1) | BIT(0))
#define APDS9253_LS_GAIN_RANGE_6  BIT(1)
#define APDS9253_LS_GAIN_RANGE_3  BIT(0) /* default */
#define APDS9253_LS_GAIN_RANGE_1  0

#define APDS9253_PART_ID          0x06
#define APDS9253_DEVICE_PART_ID   0xC0
#define APDS9253_PART_ID_REV_MASK GENMASK(3, 0)
#define APDS9253_PART_ID_ID_MASK  GENMASK(7, 4)

#define APDS9253_MAIN_STATUS_REG          0x07
#define APDS9253_MAIN_STATUS_POWER_ON     BIT(5)
#define APDS9253_MAIN_STATUS_LS_INTERRUPT BIT(4)
#define APDS9253_MAIN_STATUS_LS_STATUS    BIT(3)

/* Channels data */
#define APDS9253_LS_DATA_BASE    0x0A
#define APDS9253_LS_DATA_IR_0    0x0A
#define APDS9253_LS_DATA_IR_1    0x0B
#define APDS9253_LS_DATA_IR_2    0x0C
#define APDS9253_LS_DATA_GREEN_0 0x0D
#define APDS9253_LS_DATA_GREEN_1 0x0E
#define APDS9253_LS_DATA_GREEN_2 0x0F
#define APDS9253_LS_DATA_BLUE_0  0x10
#define APDS9253_LS_DATA_BLUE_1  0x11
#define APDS9253_LS_DATA_BLUE_2  0x12
#define APDS9253_LS_DATA_RED_0   0x13
#define APDS9253_LS_DATA_RED_1   0x14
#define APDS9253_LS_DATA_RED_2   0x15

#define APDS9253_INT_CFG                 0x19
#define APDS9253_INT_CFG_LS_INT_SEL_IR   0
#define APDS9253_INT_CFG_LS_INT_SEL_ALS  BIT(4) /* default */
#define APDS9253_INT_CFG_LS_INT_SEL_RED  BIT(5)
#define APDS9253_INT_CFG_LS_INT_SEL_BLUE (BIT(5) | BIT(4))
#define APDS9253_INT_CFG_LS_VAR_MODE_EN  BIT(3)
#define APDS9253_INT_CFG_LS_INT_MODE_EN  BIT(3)

#define APDS9253_INT_PST        0x1A
#define APDS9253_LS_THRES_UP_0  0x21
#define APDS9253_LS_THRES_UP_1  0x22
#define APDS9253_LS_THRES_UP_2  0x23
#define APDS9253_LS_THRES_LOW_0 0x24
#define APDS9253_LS_THRES_LOW_1 0x25
#define APDS9253_LS_THRES_LOW_2 0x26
#define APDS9253_LS_THRES_VAR   0x27
#define APDS9253_DK_CNT_STOR    0x29

struct apds9253_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int_gpio;
	uint8_t ls_gain;
	uint8_t ls_rate;
	uint8_t ls_resolution;
	bool interrupt_enabled;
};

struct apds9253_data {
	struct gpio_callback gpio_cb;
	struct k_work work;
	const struct device *dev;
	uint32_t sample_crgb[4];
	uint8_t pdata;
	struct k_sem data_sem;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_APDS9253_APDS9253_H_*/
