/*
 * Copyright 2021 Leonard Pollak
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA2XX_INA219_H_
#define ZEPHYR_DRIVERS_SENSOR_INA2XX_INA219_H_

#include "ina2xx_common.h"

/* Device register addresses */
#define INA219_REG_CONF		0x00
#define INA219_REG_V_SHUNT	0x01
#define INA219_REG_V_BUS	0x02
#define INA219_REG_POWER	0x03
#define INA219_REG_CURRENT	0x04
#define INA219_REG_CALIB	0x05

/* Config register shifts and masks */
#define INA219_RST		BIT(15)
#define INA219_BRNG_MASK	0x1
#define INA219_BRNG_SHIFT	13
#define INA219_PG_MASK		0x3
#define INA219_PG_SHIFT		11
#define INA219_ADC_MASK		0xF
#define INA219_BADC_SHIFT	7
#define INA219_SADC_SHIFT	3
#define INA219_MODE_MASK	0x7

/* Bus voltage register */
#define INA219_VBUS_GET(x)	((x) >> 3) & 0x3FFF
#define INA219_CNVR_RDY(x)	((x) >> 1) & 0x1
#define INA219_OVF_STATUS(x)	x & 0x1

/* Mode fields */
#define INA219_MODE_NORMAL	0x3 /* shunt and bus, triggered */
#define INA219_MODE_SLEEP	0x4 /* ADC off */
#define INA219_MODE_OFF		0x0 /* Power off */

/* Others */
#define INA219_SIGN_BIT(x)	((x) >> 15) & 0x1
#define INA219_V_BUS_MUL	0.004
#define INA219_SI_MUL		0.00001
#define INA219_POWER_MUL	20
#define INA219_WAIT_STARTUP	40
#define INA219_WAIT_MSR_RETRY	100
#define INA219_SCALING_FACTOR	4096000

struct ina219_data {
	uint16_t v_bus;
	uint16_t power;
	uint16_t current;
	uint32_t msr_delay;
};

struct ina219_config {
	struct ina2xx_config common;
	uint8_t badc;
	uint8_t sadc;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_INA2XX_INA219_H_ */
