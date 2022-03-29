/*
 * Copyright (c) 2020 Michael Pollind
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/sensor.h>
#include <sys/byteorder.h>
#include <kernel.h>
#include <sys/__assert.h>

#include <logging/log.h>

#include "icm20948.h"

LOG_MODULE_REGISTER(ICM20948, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "ICM20948 driver enabled without any devices"
#endif

struct icm20948_config {
	struct icm20948_bus bus;
	const struct icm20948_bus_io *bus_io;
};

struct icm20948_data {
	int16_t acc[3];
	int16_t gyro[3];

	int16_t temp;

	enum icm20948_gyro_fs gyro_fs; // gyro range
	enum icm20948_accel_fs accel_fs; // accel range
};

static inline int icm20948_read_data(const struct device *dev, uint16_t reg_bank_addr,
				     uint8_t *value, uint8_t len)
{
	const struct icm20948_config *cfg = dev->config;

	return cfg->bus_io->read_data(&cfg->bus, reg_bank_addr, value, len);
}

static inline int icm20948_write_data(const struct device *dev, uint16_t reg_bank_addr,
				      uint8_t *value, uint8_t len)
{
	const struct icm20948_config *cfg = dev->config;
	return cfg->bus_io->write_data(&cfg->bus, reg_bank_addr, value, len);
}

static inline int icm20948_read_reg(const struct device *dev, uint16_t reg_bank_addr,
				    uint8_t *value)
{
	const struct icm20948_config *cfg = dev->config;
	return cfg->bus_io->read_reg(&cfg->bus, reg_bank_addr, value);
}

static inline int icm20948_write_reg(const struct device *dev, uint16_t reg_bank_addr,
				     uint8_t value)
{
	const struct icm20948_config *cfg = dev->config;
	return cfg->bus_io->write_reg(&cfg->bus, reg_bank_addr, value);
}

static inline int icm20948_update_reg(const struct device *dev, uint16_t reg_bank_addr,
				      uint8_t mask, uint8_t value)
{
	const struct icm20948_config *cfg = dev->config;
	return cfg->bus_io->update_reg(&cfg->bus, reg_bank_addr, mask, value);
}

static inline int icm20948_bus_check(const struct device *dev)
{
	const struct icm20948_config *cfg = dev->config;
	return cfg->bus_io->check(&cfg->bus);
}

static inline int icm20948_set_accel_fs(const struct device *dev, enum icm20948_accel_fs accel_fs)
{
	struct icm20948_data *data = (struct icm20948_data *)dev->data;

	/* set default fullscale range for gyro */
	if (icm20948_update_reg(dev, ICM20948_REG_GYRO_CONFIG_1, ICM20948_ACCEL_MASK, accel_fs)) {
		LOG_DBG("failed to set acceleromter full-scale");
		return -EIO;
	}
	data->accel_fs = accel_fs;
	return 0;
}

static inline int icm20948_set_gyro_fs(const struct device *dev, enum icm20948_gyro_fs gyro_fs)
{
	struct icm20948_data *data = (struct icm20948_data *)dev->data;

	/* set default fullscale range for acc */
	if (icm20948_update_reg(dev, ICM20948_REG_ACCEL_CONFIG, ICM20948_ACCEL_MASK, gyro_fs)) {
		return -EIO;
	}
	data->gyro_fs = gyro_fs;
	return 0;
}

#ifdef CONFIG_ICM20948_ACCEL_RANGE_RUNTIME
static const uint16_t icm20948_accel_fs_map[] = { 2, 4, 8, 16 };
static int icm20948_accel_range_set(const struct device *dev, int32_t range)
{
	for (size_t i = 0; i < ARRAY_SIZE(icm20948_accel_fs_map); i++) {
		if (range == icm20948_accel_fs_map[i]) {
			return icm20948_set_accel_fs(dev, i);
		}
	}
	return -EINVAL;
};
#endif

#ifdef CONFIG_ICM20948_GYRO_RANGE_RUNTIME
static const uint16_t icm20948_gyro_fs_map[] = { 250, 500, 1000, 2000 };
static int icm20948_gyro_range_set(const struct device *dev, int32_t range)
{
	for (size_t i = 0; i < ARRAY_SIZE(icm20948_gyro_fs_map); i++) {
		if (range == icm20948_gyro_fs_map[i]) {
			return icm20948_set_gyro_fs(dev, i);
		}
	}
	return -EINVAL;
}
#endif

static int icm20948_accel_config(const struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
#ifdef CONFIG_ICM20948_ACCEL_RANGE_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return icm20948_accel_range_set(dev, sensor_ms2_to_g(val));
#endif
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}
}

static int icm20948_gyro_config(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
#ifdef CONFIG_ICM20948_GYRO_RANGE_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return icm20948_gyro_range_set(dev, sensor_rad_to_degrees(val));
#endif
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}
}

static int icm20948_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		return icm20948_accel_config(dev, chan, attr, val);
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return icm20948_gyro_config(dev, chan, attr, val);
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}
	return 0;
}

static inline void icm20948_accel_convert(struct sensor_value *val, int raw_val,
					  enum icm20948_accel_fs sensitivity)
{
	/* Sensitivity is exposed in LSB/g */
	double dval = raw_val / ((double)(16384 >> sensitivity));

	val->val1 = (int32_t)dval;
	val->val2 = (((int32_t)(dval * 1000)) % 1000) * 1000;
}

static inline void icm20948_gyro_convert(struct sensor_value *val, int raw_val,
					 enum icm20948_gyro_fs sensitivity)
{
	/* Sensitivity is exposed in LSB/dps */
	double dval = raw_val / ((double)(131.0 / (sensitivity + 1)));

	val->val1 = (int32_t)dval;
	val->val2 = (((int32_t)(dval * 1000)) % 1000) * 1000;
}

static int icm20948_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct icm20948_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		icm20948_accel_convert(val, data->acc[0], data->accel_fs);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm20948_accel_convert(val, data->acc[1], data->accel_fs);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm20948_accel_convert(val, data->acc[2], data->accel_fs);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		icm20948_accel_convert(val, data->acc[0], data->accel_fs);
		icm20948_accel_convert(val + 1, data->acc[1], data->accel_fs);
		icm20948_accel_convert(val + 2, data->acc[2], data->accel_fs);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm20948_gyro_convert(val, data->gyro[0], data->gyro_fs);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm20948_gyro_convert(val, data->gyro[1], data->gyro_fs);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm20948_gyro_convert(val, data->gyro[2], data->gyro_fs);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm20948_gyro_convert(val, data->gyro[0], data->gyro_fs);
		icm20948_gyro_convert(val, data->gyro[1], data->gyro_fs);
		icm20948_gyro_convert(val, data->gyro[2], data->gyro_fs);
		break;
	case SENSOR_CHAN_DIE_TEMP:
	case SENSOR_CHAN_AMBIENT_TEMP:
		break;
	default:
		break;
	}
	return 0;
}

static int icm20948_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct icm20948_data *data = dev->data;

	union {
		uint8_t raw[6];
		struct {
			int16_t axis[3];
		};
	} buf __aligned(2);

	union {
		uint8_t raw[2];
		struct {
			int16_t temp;
		};
	} buf2 __aligned(2);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ: {
		if (icm20948_read_data(dev, ICM20948_REG_ACCEL_XOUT_H_SH, buf.raw, sizeof(buf))) {
			LOG_DBG("Failed to fetch raw data samples from accel");
			return -EIO;
		}
		data->acc[0] = sys_be16_to_cpu(buf.axis[0]);
		data->acc[1] = sys_be16_to_cpu(buf.axis[1]);
		data->acc[2] = sys_be16_to_cpu(buf.axis[2]);
	} break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ: {
		if (icm20948_read_data(dev, ICM20948_REG_GYRO_XOUT_H_SH, buf.raw, sizeof(buf))) {
			LOG_DBG("Failed to fetch raw data samples from gyro");
			return -EIO;
		}
		data->gyro[0] = sys_be16_to_cpu(buf.axis[0]);
		data->gyro[1] = sys_be16_to_cpu(buf.axis[1]);
		data->gyro[2] = sys_be16_to_cpu(buf.axis[2]);
		return 0;
	} break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		data->temp = sys_be16_to_cpu(buf2.temp);
		break;
	case SENSOR_CHAN_ALL: {
		if (icm20948_sample_fetch(dev, SENSOR_CHAN_ACCEL_XYZ)) {
			return -EIO;
		}
		if (icm20948_sample_fetch(dev, SENSOR_CHAN_GYRO_XYZ)) {
			return -EIO;
		}
		if (icm20948_sample_fetch(dev, SENSOR_CHAN_AMBIENT_TEMP)) {
			return -EIO;
		}
	} break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

int icm20948_chip_init(struct device *dev)
{

	int err = icm20948_bus_check(dev);
	if (err < 0) {
		LOG_DBG("bus check failed: %d", err);
		return err;
	}


	uint8_t tmp = 0;
	/* verify chip ID */
	if (icm20948_read_reg(dev, ICM20948_REG_WHO_AM_I, &tmp)) {
		LOG_ERR("Failed to read chip ID");
		return -EIO;
	}

	if (tmp != ICM20948_WHO_AM_I) {
		LOG_ERR("Invalid Chip ID Expects 0x%x -- 0x%x", ICM20948_WHO_AM_I, tmp);
		return -ENOTSUP;
	}

	/* set gyro and accel config */
	icm20948_set_gyro_fs(dev, ICM20948_GYRO_FS_DEFAULT);
	icm20948_set_accel_fs(dev, ICM20948_ACCEL_FS_DEFAULT);

	/* set default fullscale range for gyro */
	if (icm20948_update_reg(dev, ICM20948_REG_GYRO_CONFIG_1, ICM20948_GYRO_MASK,
				ICM20948_GYRO_FS_DEFAULT)) {
		return -EIO;
	}

	/* wake up chip */
	if (icm20948_update_reg(dev, ICM20948_REG_PWR_MGMT_1, BIT(6), 0)) {
		LOG_ERR("Failed to wake up chip.");
		return -EIO;
	}

	return 0;
}

static const struct sensor_driver_api icm20948_api_funcs = { 
	.attr_set = icm20948_attr_set,
	.sample_fetch = icm20948_sample_fetch,
	.channel_get = icm20948_channel_get 
};


/* Initializes a struct icm20948_config for an instance on a SPI bus. */
#define ICM20948_CONFIG_SPI(inst)                                                                  \
	{    \
		.bus = { \
			.spi = SPI_DT_SPEC_INST_GET(inst,			\
					    SPI_OP_MODE_MASTER |	\
					    SPI_MODE_CPOL |		\
					    SPI_MODE_CPHA |		\
					    SPI_WORD_SET(8) |		\
					    SPI_TRANSFER_MSB,		\
					    0U),			\
			.active_bank = 255 \
		}, \
		.bus_io = &icm20948_bus_io_spi,                            \
	}

/* Initializes a struct icm20948_config for an instance on an I2C bus. */
#define ICM20948_CONFIG_I2C(inst)                                                                  \
	{                                                                                          \
		.bus = { \
			.i2c = I2C_DT_SPEC_INST_GET(inst),		\
			.active_bank = 255,						\
		}, \
		.bus_io = &icm20948_bus_io_i2c,            \
	}


#define ICM42605_DEFINE(inst)                                                                       \
	static struct icm20948_data icm20948_data_##inst;                                          \
	static const struct icm20948_config icm20948_config_##inst =                               \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), \
				(ICM20948_CONFIG_SPI(inst)),                \
			    (ICM20948_CONFIG_I2C(inst)));                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, icm20948_chip_init, PM_DEVICE_DT_INST_GET(inst),               \
			      &icm20948_data_##inst, &icm20948_config_##inst, POST_KERNEL,         \
			      CONFIG_SENSOR_INIT_PRIORITY, &icm20948_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(ICM42605_DEFINE)