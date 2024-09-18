/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl345

#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "adxl345.h"

LOG_MODULE_REGISTER(ADXL345, CONFIG_SENSOR_LOG_LEVEL);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
static bool adxl345_bus_is_ready_i2c(const union adxl345_bus *bus)
{
	return device_is_ready(bus->i2c.bus);
}

static int adxl345_reg_access_i2c(const struct device *dev, uint8_t cmd, uint8_t reg_addr,
				  uint8_t *data, size_t length)
{
	const struct adxl345_dev_config *cfg = dev->config;

	if (cmd == ADXL345_READ_CMD) {
		return i2c_burst_read_dt(&cfg->bus.i2c, reg_addr, data, length);
	} else {
		return i2c_burst_write_dt(&cfg->bus.i2c, reg_addr, data, length);
	}
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
static bool adxl345_bus_is_ready_spi(const union adxl345_bus *bus)
{
	return spi_is_ready_dt(&bus->spi);
}

static int adxl345_reg_access_spi(const struct device *dev, uint8_t cmd, uint8_t reg_addr,
				  uint8_t *data, size_t length)
{
	const struct adxl345_dev_config *cfg = dev->config;
	uint8_t access = reg_addr | cmd | (length == 1 ? 0 : ADXL345_MULTIBYTE_FLAG);
	const struct spi_buf buf[2] = {{.buf = &access, .len = 1}, {.buf = data, .len = length}};
	const struct spi_buf_set rx = {.buffers = buf, .count = ARRAY_SIZE(buf)};
	struct spi_buf_set tx = {
		.buffers = buf,
		.count = 2,
	};

	if (cmd == ADXL345_READ_CMD) {
		tx.count = 1;
		return spi_transceive_dt(&cfg->bus.spi, &tx, &rx);
	} else {
		return spi_write_dt(&cfg->bus.spi, &tx);
	}
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

static inline int adxl345_reg_access(const struct device *dev, uint8_t cmd, uint8_t addr,
				     uint8_t *data, size_t len)
{
	const struct adxl345_dev_config *cfg = dev->config;

	return cfg->reg_access(dev, cmd, addr, data, len);
}

static inline int adxl345_reg_write(const struct device *dev, uint8_t addr, uint8_t *data,
				    uint8_t len)
{

	return adxl345_reg_access(dev, ADXL345_WRITE_CMD, addr, data, len);
}

static inline int adxl345_reg_read(const struct device *dev, uint8_t addr, uint8_t *data,
				   uint8_t len)
{

	return adxl345_reg_access(dev, ADXL345_READ_CMD, addr, data, len);
}

static inline int adxl345_reg_write_byte(const struct device *dev, uint8_t addr, uint8_t val)
{
	return adxl345_reg_write(dev, addr, &val, 1);
}

static inline int adxl345_reg_read_byte(const struct device *dev, uint8_t addr, uint8_t *buf)

{
	return adxl345_reg_read(dev, addr, buf, 1);
}

static inline bool adxl345_bus_is_ready(const struct device *dev)
{
	const struct adxl345_dev_config *cfg = dev->config;

	return cfg->bus_is_ready(&cfg->bus);
}

static int adxl345_read_sample(const struct device *dev,
			       struct adxl345_sample *sample)
{
	int16_t raw_x, raw_y, raw_z;
	uint8_t axis_data[6];

	int rc = adxl345_reg_read(dev, ADXL345_X_AXIS_DATA_0_REG, axis_data, 6);

	if (rc < 0) {
		LOG_ERR("Samples read failed with rc=%d\n", rc);
		return rc;
	}

	raw_x = axis_data[0] | (axis_data[1] << 8);
	raw_y = axis_data[2] | (axis_data[3] << 8);
	raw_z = axis_data[4] | (axis_data[5] << 8);

	sample->x = raw_x;
	sample->y = raw_y;
	sample->z = raw_z;

	return 0;
}

static void adxl345_accel_convert(struct sensor_value *val, int16_t sample)
{
	if (sample & BIT(9)) {
		sample |= ADXL345_COMPLEMENT;
	}

	val->val1 = ((sample * SENSOR_G) / 32) / 1000000;
	val->val2 = ((sample * SENSOR_G) / 32) % 1000000;
}

static int adxl345_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct adxl345_dev_data *data = dev->data;
	struct adxl345_sample sample;
	uint8_t samples_count;
	int rc;

	data->sample_number = 0;
	rc = adxl345_reg_read_byte(dev, ADXL345_FIFO_STATUS_REG, &samples_count);
	if (rc < 0) {
		LOG_ERR("Failed to read FIFO status rc = %d\n", rc);
		return rc;
	}

	__ASSERT_NO_MSG(samples_count <= ARRAY_SIZE(data->bufx));

	for (uint8_t s = 0; s < samples_count; s++) {
		rc = adxl345_read_sample(dev, &sample);
		if (rc < 0) {
			LOG_ERR("Failed to fetch sample rc=%d\n", rc);
			return rc;
		}
		data->bufx[s] = sample.x;
		data->bufy[s] = sample.y;
		data->bufz[s] = sample.z;
	}

	return 0;
}

static int adxl345_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adxl345_dev_data *data = dev->data;

	if (data->sample_number >= ARRAY_SIZE(data->bufx)) {
		data->sample_number = 0;
	}

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		adxl345_accel_convert(val, data->bufx[data->sample_number]);
		data->sample_number++;
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl345_accel_convert(val, data->bufy[data->sample_number]);
		data->sample_number++;
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl345_accel_convert(val, data->bufz[data->sample_number]);
		data->sample_number++;
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl345_accel_convert(val++, data->bufx[data->sample_number]);
		adxl345_accel_convert(val++, data->bufy[data->sample_number]);
		adxl345_accel_convert(val,   data->bufz[data->sample_number]);
		data->sample_number++;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api adxl345_api_funcs = {
	.sample_fetch = adxl345_sample_fetch,
	.channel_get = adxl345_channel_get,
};

static int adxl345_init(const struct device *dev)
{
	int rc;
	struct adxl345_dev_data *data = dev->data;
	uint8_t dev_id;

	data->sample_number = 0;

	if (!adxl345_bus_is_ready(dev)) {
		LOG_ERR("bus not ready");
		return -ENODEV;
	}

	rc = adxl345_reg_read_byte(dev, ADXL345_DEVICE_ID_REG, &dev_id);
	if (rc < 0 || dev_id != ADXL345_PART_ID) {
		LOG_ERR("Read PART ID failed: 0x%x\n", rc);
		return -ENODEV;
	}

	rc = adxl345_reg_write_byte(dev, ADXL345_FIFO_CTL_REG, ADXL345_FIFO_STREAM_MODE);
	if (rc < 0) {
		LOG_ERR("FIFO enable failed\n");
		return -EIO;
	}

	rc = adxl345_reg_write_byte(dev, ADXL345_DATA_FORMAT_REG, ADXL345_RANGE_16G);
	if (rc < 0) {
		LOG_ERR("Data format set failed\n");
		return -EIO;
	}

	rc = adxl345_reg_write_byte(dev, ADXL345_RATE_REG, ADXL345_RATE_25HZ);
	if (rc < 0) {
		LOG_ERR("Rate setting failed\n");
		return -EIO;
	}

	rc = adxl345_reg_write_byte(dev, ADXL345_POWER_CTL_REG, ADXL345_ENABLE_MEASURE_BIT);
	if (rc < 0) {
		LOG_ERR("Enable measure bit failed\n");
		return -EIO;
	}

	return 0;
}

#define ADXL345_CONFIG_SPI(inst)                                       \
	{                                                              \
		.bus = {.spi = SPI_DT_SPEC_INST_GET(inst,              \
						    SPI_WORD_SET(8) |  \
						    SPI_TRANSFER_MSB | \
						    SPI_MODE_CPOL |    \
						    SPI_MODE_CPHA,     \
						    0)},               \
		.bus_is_ready = adxl345_bus_is_ready_spi,              \
		.reg_access = adxl345_reg_access_spi,                  \
	}

#define ADXL345_CONFIG_I2C(inst)			    \
	{						    \
		.bus = {.i2c = I2C_DT_SPEC_INST_GET(inst)}, \
		.bus_is_ready = adxl345_bus_is_ready_i2c,   \
		.reg_access = adxl345_reg_access_i2c,	    \
	}

#define ADXL345_DEFINE(inst)								\
	static struct adxl345_dev_data adxl345_data_##inst;				\
											\
	static const struct adxl345_dev_config adxl345_config_##inst =                  \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (ADXL345_CONFIG_SPI(inst)),      \
			    (ADXL345_CONFIG_I2C(inst)));                                \
                                                                                        \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adxl345_init, NULL,				\
			      &adxl345_data_##inst, &adxl345_config_##inst, POST_KERNEL,\
			      CONFIG_SENSOR_INIT_PRIORITY, &adxl345_api_funcs);		\

DT_INST_FOREACH_STATUS_OKAY(ADXL345_DEFINE)
