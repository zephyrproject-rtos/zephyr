/*
 * Copyright (c) 2021 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SHTCX_SHTCX_H_
#define ZEPHYR_DRIVERS_SENSOR_SHTCX_SHTCX_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* common cmds */
#define SHTCX_CMD_READ_ID		0xEFC8
#define SHTCX_CMD_SOFT_RESET		0x805D
/* shtc3 only: */
#define SHTCX_CMD_SLEEP			0xB098
#define SHTCX_CMD_WAKEUP		0x3517

#define SHTCX_POWER_UP_TIME_US		240U
/* Soft reset time is 230us for shtc1 and 240us for shtc3 */
#define SHTCX_SOFT_RESET_TIME_US	240U

#define SHTCX_MAX_READ_LEN		6
#define SHTCX_WORD_LEN			2
#define SHTCX_CRC8_LEN			1

#define SHTC3_ID_MASK			0x083F
#define SHTC3_ID_VALUE			0x0807
#define SHTC1_ID_MASK			0x083F
#define SHTC1_ID_VALUE			0x0007

/* defines matching the related enums DT_ENUM_IDX: */
#define CHIP_SHTC1		0
#define CHIP_SHTC3		1
#define MEASURE_MODE_NORMAL	0
#define MEASURE_MODE_LOW_POWER	1

enum shtcx_chip {
	SHTC1 = CHIP_SHTC1,
	SHTC3 = CHIP_SHTC3,
};

enum shtcx_measure_mode {
	NORMAL = MEASURE_MODE_NORMAL,
	LOW_POWER = MEASURE_MODE_LOW_POWER,
};

struct shtcx_sample {
	uint16_t temp;
	uint16_t humidity;
} __packed __aligned(2);

struct shtcx_config {
	const struct device *bus;
	uint8_t base_address;
	enum shtcx_chip chip;
	enum shtcx_measure_mode measure_mode;
	bool clock_stretching;
};

struct shtcx_data {
	struct shtcx_sample sample;
};

static inline uint8_t shtcx_i2c_address(const struct device *dev)
{
	const struct shtcx_config *dcp = dev->config;

	return dcp->base_address;
}

static inline const struct device *shtcx_i2c_bus(const struct device *dev)
{
	const struct shtcx_config *dcp = dev->config;

	return dcp->bus;
}

#endif /* ZEPHYR_DRIVERS_SENSOR_SHTCX_SHTCX_H_ */
