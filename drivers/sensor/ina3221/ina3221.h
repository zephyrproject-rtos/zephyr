/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>

#define INA3221_CONFIG		0x00
#define INA3221_SHUNT_V1	0x01
#define INA3221_BUS_V1		0x02
#define INA3221_SHUNT_V2	0x03
#define INA3221_BUS_V2		0x04
#define INA3221_SHUNT_V3	0x05
#define INA3221_BUS_V3		0x06

#define INA3221_MASK_ENABLE	0x0f
#define INA3221_MANUF_ID	0xfe
#define INA3221_MANUF_ID_VALUE	0x5449
#define INA3221_CHIP_ID		0xff
#define INA3221_CHIP_ID_VALUE	0x3220

#define INA3221_MASK_ENABLE_CONVERSION_READY	BIT(0)
#define INA3221_CONFIG_RST			BIT(15)
#define INA3221_CONFIG_CH1			BIT(14)
#define INA3221_CONFIG_CH2			BIT(13)
#define INA3221_CONFIG_CH3			BIT(12)
#define INA3221_CONFIG_AVG_Msk			GENMASK(11, 9)
#define INA3221_CONFIG_CT_VBUS_Msk		GENMASK(8, 6)
#define INA3221_CONFIG_CT_VSH_Msk		GENMASK(5, 3)
#define INA3221_CONFIG_CONTINUOUS		BIT(2)
#define INA3221_CONFIG_BUS			BIT(1)
#define INA3221_CONFIG_SHUNT			BIT(0)

#define INA3221_BUS_VOLTAGE_LSB			0.008f
#define INA3221_SHUNT_VOLTAGE_LSB		0.00004f

#define SENSOR_ATTR_INA3221_SELECTED_CHANNEL (SENSOR_ATTR_PRIV_START+1)

enum ina3221_avg_mode {
	INA3221_AVG_MODE_1 = 0,
	INA3221_AVG_MODE_4,
	INA3221_AVG_MODE_16,
	INA3221_AVG_MODE_64,
	INA3221_AVG_MODE_128,
	INA3221_AVG_MODE_256,
	INA3221_AVG_MODE_512,
	INA3221_AVG_MODE_1024,
};

enum ina3221_conv_time {
	INA3221_CONV_TIME_0_140ms = 0,
	INA3221_CONV_TIME_0_204ms,
	INA3221_CONV_TIME_0_332ms,
	INA3221_CONV_TIME_0_588ms,
	INA3221_CONV_TIME_1_100ms,
	INA3221_CONV_TIME_2_116ms,
	INA3221_CONV_TIME_4_156ms,
	INA3221_CONV_TIME_8_244ms,
};

static const int32_t avg_mode_samples[8] = {1, 4, 16, 64, 128, 256, 512, 1024};
static const int32_t conv_time_us[8] = {140, 204, 332, 588, 1100, 2116, 4156, 8244};

struct ina3221_config {
	struct i2c_dt_spec bus;
	enum ina3221_avg_mode avg_mode;
	enum ina3221_conv_time conv_time_bus;
	enum ina3221_conv_time conv_time_shunt;
	bool enable_channel[3];
	uint16_t shunt_r[3];
};

struct ina3221_data {
	size_t selected_channel;
	uint16_t config;
	int16_t v_bus[3];
	int16_t v_shunt[3];
};
