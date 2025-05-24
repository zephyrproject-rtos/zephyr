/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA745_INA745_H_
#define ZEPHYR_DRIVERS_SENSOR_INA745_INA745_H_

/* Config register shifts and masks */
#define INA745_REG_CONFIG		0x00
#define INA745_RST_MASK			0x1
#define INA745_RST_SHIFT		15
#define INA745_RSTACC_MASK		0x1
#define INA745_RSTACC_SHIFT		14
#define INA745_CONVDLY_MASK		0xff
#define INA745_CONVDLY_SHIFT	6

/* Mode fields */
/* Shutdown */
#define INA745_MODE_OFF			0x0
/* Triggered mode for temp, current and bus voltage, single shot */
#define INA745_MODE_TRIGGER		0x7
/* Continuous temperature, current, and bus voltage */
#define INA745_MODE_CONTI		0xf


/* ADC Config register shifts and masks */
#define INA745_REG_ADC_CONFIG	0x01
#define INA745_MODE_MASK		0xf
#define INA745_MODE_SHIFT		12
#define INA745_VBUSCT_MASK		0x7
#define INA745_VBUSCT_SHIFT		9
#define INA745_VSENCT_MASK		0x7
#define INA745_VSENCT_SHIFT		6
#define INA745_TCT_MASK			0x7
#define INA745_TCT_SHIFT		3
#define INA745_AVG_MASK			0x7
#define INA745_AVG_SHIFT		0


#define INA745_REG_BUS_VOLTAGE	0x05
#define INA745_REG_CURRENT		0x07
#define INA745_REG_DIE_TEMP		0x06
#define INA745_REG_DIE_TEMP_SHIFT	4
#define INA745_REG_POWER		0x08
#define INA745_REG_ENERGY		0x09
#define INA745_REG_CHARGE		0x03
#define INA745_REG_DIAG_ALRT	0x0b
#define INA745_REG_COL			0x0c
#define INA745_REG_CUL			0x0d
#define INA745_REG_BOVL			0x0e
#define INA745_REG_BUVL			0x0f
#define INA745_REG_TEMP_LIMIT	0x10
#define INA745_REG_PWR_LIMIT	0x11
#define INA745_REG_ID			0x3e

#define INA745_ID				0x5449
#define INA780_ID				0x5449

/* Others */
#define INA745_SIGN_BIT(x)		((x) >> 15) & 0x1
#define INA745_BUS_VOLTAGE_MUL	0.003125
#define INA745_CURRENT_MUL		0.0012
#define INA745_TEMP_SCALE       0.125
#define INA745_POWER_MUL		0.000240
#define INA745_WAIT_STARTUP		40

struct ina745_config {
	struct i2c_dt_spec bus;
	uint8_t mode;
	uint8_t convdly;
	uint8_t vbusct;
	uint8_t vsenct;
	uint8_t tct;
	uint8_t avg;
};

struct ina745_data {
	uint16_t config;
	uint8_t convdly;
	uint16_t adc_config;
	uint16_t v_bus;
	float die_temp;
	uint16_t current;
	uint64_t power;
	uint64_t energy;
	uint64_t charge;
	uint16_t col;
};

static int ina745_init(const struct device *dev);


#endif /* ZEPHYR_DRIVERS_SENSOR_INA745_INA745_H_ */
