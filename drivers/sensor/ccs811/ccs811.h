/*
 * Copyright (c) 2018 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_CCS811_CCS811_H_
#define ZEPHYR_DRIVERS_SENSOR_CCS811_CCS811_H_

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>

/* Registers */
#define CCS811_REG_STATUS		0x00
#define CCS811_REG_MEAS_MODE		0x01
#define CCS811_REG_ALG_RESULT_DATA	0x02
#define CCS811_REG_RAW_DATA		0x03
#define CCS811_REG_HW_ID		0x20
#define CCS811_REG_HW_VERSION		0x21
#define CCS811_REG_HW_VERSION_MASK	0xF0
#define CCS811_REG_ERR			0xE0
#define CCS811_REG_APP_START		0xF4

#define CCS881_HW_ID			0x81
#define CCS811_HW_VERSION		0x10

/* Status register fields */
#define CCS811_STATUS_ERROR		BIT(0)
#define CCS811_STATUS_DATA_READY	BIT(3)
#define CCS811_STATUS_APP_VALID		BIT(4)
#define CCS811_STATUS_FW_MODE		BIT(7)

/* Measurement modes */
#define CCS811_MODE_IDLE		0x00
#define CCS811_MODE_IAQ_1SEC		0x10
#define CCS811_MODE_IAQ_10SEC		0x20
#define CCS811_MODE_IAQ_60SEC		0x30
#define CCS811_MODE_RAW_DATA		0x40

#define CCS811_VOLTAGE_SCALE		1613

#define CCS811_VOLTAGE_MASK		0x3FF

struct ccs811_data {
	struct device *i2c;
#ifdef CONFIG_CCS811_GPIO_WAKEUP
	struct device *gpio;
#endif
	u16_t co2;
	u16_t voc;
	u8_t status;
	u8_t error;
	u16_t resistance;
};

#endif /* _SENSOR_CCS811_ */
