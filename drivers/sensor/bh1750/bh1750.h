/* Rohm BH1750 16 bit ambient light sensor
 *
 * Copyright (c) 2022 Lucas Heitzmann Gabrielli
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BH1750_H
#define __BH1750_H

#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <sys/util.h>

#define BH1750_RESET 0x07

#define BH1750_CMD_HRES 0x20
#define BH1750_CMD_HRES2 0x21
#define BH1750_CMD_LRES 0x23

#if defined(CONFIG_BH1750_NORMAL_RES)
#define BH1750_SINGLE_MEASUREMENT BH1750_CMD_HRES
#define BH1750_TIME_FACTOR 2610U
#elif defined(CONFIG_BH1750_HIGH_RES)
#define BH1750_SINGLE_MEASUREMENT BH1750_CMD_HRES2
#define BH1750_TIME_FACTOR 2610U
#elif defined(CONFIG_BH1750_LOW_RES)
#define BH1750_SINGLE_MEASUREMENT BH1750_CMD_LRES
#define BH1750_TIME_FACTOR 350U
#endif

#define BH1750_MEAS_TIME_MSB 0x40
#define BH1750_MEAS_TIME_LSB 0x60

#define BH1750_MEAS_TIME_MIN 31
#define BH1750_MEAS_TIME_MAX 254

struct bh1750_config {
	const struct device *bus;
	uint16_t bus_addr;
};

struct bh1750_data {
	uint16_t adc_count;
	uint8_t oversampling_factor;
};

#endif /* __BH1750_H */
