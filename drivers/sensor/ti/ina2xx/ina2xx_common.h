/*
 * Copyright 2021 Grinn
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA2XX_COMMON_H_
#define ZEPHYR_DRIVERS_SENSOR_INA2XX_COMMON_H_

#include <stdint.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor/ina2xx.h>
#include <zephyr/sys/util_macro.h>

#define INA2XX_REG_DEFINE(_name, _addr, _size, _shift) \
	static const struct ina2xx_reg _name = {         \
		.addr = (_addr),                             \
		.size = (_size),                             \
		.shift = (_shift),                           \
	}

#define INA2XX_CHANNEL_DEFINE(_name, _addr, _size, _shift, _mult, _div) \
	INA2XX_REG_DEFINE(_name##_reg, _addr, _size, _shift);               \
	static const struct ina2xx_channel _name = {                      \
		.reg = (&_name##_reg),                                        \
		.mult = (_mult),                                              \
		.div = (_div),                                                \
	}

/**
 * @brief INA2XX register mapping.
 *
 * Different INA2XX variants have different register sizes and addresses for
 * the same data. This structure allows the driver to specify the register
 * mapping without writing variant specific read functions.
 *
 * @param addr Register address
 * @param size Register size in bits (16, 24 or 40)
 * @param shift Register shift
 */
struct ina2xx_reg {
	uint8_t addr;
	uint8_t size;
	uint8_t shift;
};

/**
 * @brief INA2XX channel mapping.
 *
 * Most INA2XX chips use the same formula for encoding data but have different
 * scaling factors depending on, e.g., ADC resolution.
 *
 * @param reg Register mapping
 * @param mult Channel multiplier
 * @param div Channel divisor
 */
struct ina2xx_channel {
	const struct ina2xx_reg *reg;
	uint32_t mult;
	uint32_t div;
};

/**
 * @brief INA2XX data channels.
 *
 * A helper structure for organizing the channel mappings. Not all channels
 * are available on all INA2XX variants.
 */
struct ina2xx_channels {
	const struct ina2xx_channel *bus_voltage;
	const struct ina2xx_channel *shunt_voltage;
	const struct ina2xx_channel *current;
	const struct ina2xx_channel *power;
	const struct ina2xx_channel *die_temp;
	const struct ina2xx_channel *energy;
	const struct ina2xx_channel *charge;
};

/**
 * @brief The common INA2XX configuration structure.
 */
struct ina2xx_config {
	struct i2c_dt_spec bus;
	uint32_t current_lsb;
	uint16_t config;
	uint16_t adc_config;
	uint16_t cal;
	const struct ina2xx_reg *id_reg;
	const struct ina2xx_reg *config_reg;
	const struct ina2xx_reg *adc_config_reg;
	const struct ina2xx_reg *cal_reg;
	const struct ina2xx_channels *channels;
};

struct ina2xx_data {
	int32_t bus_voltage;
	int32_t shunt_voltage;
	int32_t current;
	uint32_t power;
	int32_t die_temp;
	uint64_t energy;
	uint64_t charge;
};

void ina2xx_sensor_value_from_channel_s32(struct sensor_value *val,
	const struct ina2xx_channel *ch, int32_t raw);

void ina2xx_sensor_value_from_channel_u32(struct sensor_value *val,
	const struct ina2xx_channel *ch, uint32_t raw);

void ina2xx_sensor_value_from_channel_u64(struct sensor_value *val,
	const struct ina2xx_channel *ch, uint64_t raw);

int ina2xx_reg_read_40(const struct i2c_dt_spec *bus, uint8_t reg, uint64_t *val);
int ina2xx_reg_read_24(const struct i2c_dt_spec *bus, uint8_t reg, uint32_t *val);
int ina2xx_reg_read_16(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t *val);

int ina2xx_reg_read(const struct i2c_dt_spec *bus, const struct ina2xx_reg *reg, void *val);
int ina2xx_reg_write(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t val);

int ina2xx_sample_fetch(const struct device *dev, enum sensor_channel chan);

int ina2xx_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val);

int ina2xx_attr_set(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, const struct sensor_value *val);

int ina2xx_attr_get(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, struct sensor_value *val);

int ina2xx_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_INA2XX_COMMON_H_ */
