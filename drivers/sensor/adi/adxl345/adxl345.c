/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl345

#include <zephyr/drivers/sensor/adxl345.h>
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

int adxl345_reg_write_byte(const struct device *dev, uint8_t addr, uint8_t val)
{
	return adxl345_reg_write(dev, addr, &val, 1);
}

int adxl345_reg_read_byte(const struct device *dev, uint8_t addr, uint8_t *buf)

{
	return adxl345_reg_read(dev, addr, buf, 1);
}

int adxl345_reg_write_mask(const struct device *dev, uint8_t reg_addr, uint32_t mask, uint8_t data)
{
	int ret;
	uint8_t tmp;

	ret = adxl345_reg_read_byte(dev, reg_addr, &tmp);
	if (ret != 0) {
		return ret;
	}

	tmp &= ~mask;
	tmp |= data;

	return adxl345_reg_write_byte(dev, reg_addr, tmp);
}

static inline bool adxl345_bus_is_ready(const struct device *dev)
{
	const struct adxl345_dev_config *cfg = dev->config;

	return cfg->bus_is_ready(&cfg->bus);
}

static int adxl345_read_sample(const struct device *dev,
			       struct adxl345_sample *sample)
{
	uint8_t axis_data[6];

	int rc = adxl345_reg_read(dev, ADXL345_X_AXIS_DATA_0_REG, axis_data, 6);

	if (rc < 0) {
		LOG_ERR("Samples read failed with rc=%d\n", rc);
		return rc;
	}

	sample->x = axis_data[0] | (((int16_t)axis_data[1]) << 8);
	sample->y = axis_data[2] | (((int16_t)axis_data[3]) << 8);
	sample->z = axis_data[4] | (((int16_t)axis_data[5]) << 8);

	return 0;
}

static inline void adxl345_accel_convert(struct sensor_value *val, int16_t sample, uint16_t scale)
{
	int64_t micro_g = (sample * scale) * 1000;

	sensor_ug_to_ms2(micro_g, val);
}

static int adxl345_sample_fetch(const struct device *dev,
				enum sensor_channel cha)
{
	struct adxl345_dev_data *data = dev->data;
	struct adxl345_sample sample;
	uint8_t samples_count;
	int rc;

	data->sample_number = 0;

	if (data->fifo_mode == ADXL345_FIFO_BYPASS) {
		samples_count = 1;
	} else {
		rc = adxl345_reg_read_byte(dev, ADXL345_FIFO_STATUS_REG, &samples_count);
		if (rc < 0) {
			LOG_ERR("Failed to read FIFO status rc = %d\n", rc);
			return rc;
		}
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

	return samples_count;
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
		adxl345_accel_convert(val, data->bufx[data->sample_number], data->scale_factor);
		data->sample_number++;
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl345_accel_convert(val, data->bufy[data->sample_number], data->scale_factor);
		data->sample_number++;
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl345_accel_convert(val, data->bufz[data->sample_number], data->scale_factor);
		data->sample_number++;
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl345_accel_convert(val++, data->bufx[data->sample_number], data->scale_factor);
		adxl345_accel_convert(val++, data->bufy[data->sample_number], data->scale_factor);
		adxl345_accel_convert(val, data->bufz[data->sample_number], data->scale_factor);
		data->sample_number++;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_ADXL345_TRIGGER
static int adxl345_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_ACTIVE_THRESH: {
		int16_t milli_g = sensor_ms2_to_ug(val) / 1000;
		return adxl345_reg_write_byte(dev, ADXL345_THRESH_ACT_REG, milli_g * 2 / 125);
	}
	case SENSOR_ATTR_INACTIVE_THRESH: {
		int16_t milli_g = sensor_ms2_to_ug(val) / 1000;
		return adxl345_reg_write_byte(dev, ADXL345_THRESH_INACT_REG, milli_g * 2 / 125);
	}
	case SENSOR_ATTR_INACTIVE_TIME: {
		uint16_t milli_seconds = (((int64_t)val->val1) * 1000) + (val->val2 / 1000);
		return adxl345_reg_write_byte(dev, ADXL345_TIME_INACT_REG, milli_seconds / 1000);
	}
	case SENSOR_ATTR_LINK_MOVEMENT: {
		return adxl345_reg_write_mask(dev, ADXL345_POWER_CTL_REG,
					      ADXL345_POWER_CTL_LINK_BIT,
					      ADXL345_POWER_CTL_LINK_BIT);
	}
	default: {
		return -ENOTSUP;
	}
	}
}
#endif

static const struct sensor_driver_api adxl345_api_funcs = {
	.sample_fetch = adxl345_sample_fetch,
	.channel_get = adxl345_channel_get,
#ifdef CONFIG_ADXL345_TRIGGER
	.attr_set = adxl345_attr_set,
	.trigger_set = adxl345_trigger_set,
#endif
};

static int adxl345_set_output_rate(const struct device *dev, const enum adxl345_odr odr)
{
	int rc = adxl345_reg_write_mask(dev, ADXL345_BW_RATE_REG, ADXL345_BW_RATE_RATE_MSK, odr);

	if (rc < 0) {
		return -EIO;
	}

	return rc;
}

static int adxl345_set_range(const struct device *dev, const enum adxl345_range range)
{
	int rc;
	struct adxl345_dev_data *data = dev->data;

	rc = adxl345_reg_write_mask(dev, ADXL345_DATA_FORMAT_REG, ADXL345_DATA_FORMAT_RANGE_MSK,
				    range);
	if (rc < 0) {
		return -EIO;
	}

	switch (range) {
	case ADXL345_2G_RANGE:
		data->scale_factor = 4;
		break;
	case ADXL345_4G_RANGE:
		data->scale_factor = 8;
		break;
	case ADXL345_8G_RANGE:
		data->scale_factor = 16;
		break;
	case ADXL345_16G_RANGE:
	default:
		data->scale_factor = 32;
		break;
	}

	return rc;
}

static enum adxl345_odr adxl345_map_odr(int frequency)
{
	switch (frequency) {
	case 6:
		return ADXL345_ODR_6P25HZ;
	case 12:
		return ADXL345_ODR_12P5HZ;
	case 25:
		return ADXL345_ODR_25HZ;
	case 50:
		return ADXL345_ODR_50HZ;
	case 100:
		return ADXL345_ODR_100HZ;
	case 200:
		return ADXL345_ODR_200HZ;
	case 400:
		return ADXL345_ODR_400HZ;
	default:
		LOG_WRN("Invalid ADXL345 frequency received, set to 100Hz");
		return ADXL345_ODR_100HZ;
	}
}

static enum adxl345_range adxl345_map_range(int range)
{
	switch (range) {
	case 2:
		return ADXL345_2G_RANGE;
	case 4:
		return ADXL345_4G_RANGE;
	case 8:
		return ADXL345_8G_RANGE;
	case 16:
		return ADXL345_16G_RANGE;
	default:
		LOG_WRN("Invalid ADXL345 range received, set to 16G");
		return ADXL345_16G_RANGE;
	}
}

static enum adxl345_fifo adxl345_map_fifo(int fifo_mode)
{
	switch (fifo_mode) {
	case 0:
		return ADXL345_FIFO_BYPASS;
	case 1:
		return ADXL345_FIFO_FIFO;
	case 2:
		return ADXL345_FIFO_STREAM;
	case 3:
		return ADXL345_FIFO_TRIGGER;
	default:
		LOG_WRN("Invalid ADXL345 range received, set to bypass");
		return ADXL345_FIFO_BYPASS;
	}
}

static int adxl345_init(const struct device *dev)
{
	int rc;
	struct adxl345_dev_data *data = dev->data;
	const struct adxl345_dev_config *cfg = dev->config;
	uint8_t dev_id;

	data->sample_number = 0;

	if (!adxl345_bus_is_ready(dev)) {
		LOG_ERR("bus not ready");
		return -ENODEV;
	}

	rc = adxl345_reg_read_byte(dev, ADXL345_DEVICE_ID_REG, &dev_id);
	if (rc < 0 || dev_id != ADXL345_DEVICE_ID_VALUE) {
		LOG_ERR("Read PART ID failed: 0x%x\n", rc);
		return -ENODEV;
	}

	data->fifo_mode = adxl345_map_fifo(cfg->fifo_mode);
	rc = adxl345_reg_write_byte(dev, ADXL345_FIFO_CTL_REG, data->fifo_mode);
	if (rc < 0) {
		LOG_ERR("FIFO enable failed\n");
		return -EIO;
	}

	data->range = adxl345_map_range(cfg->range);
	rc = adxl345_set_range(dev, data->range);
	if (rc < 0) {
		LOG_ERR("Data format set failed\n");
		return -EIO;
	}

	data->odr = adxl345_map_odr(cfg->frequency);
	rc = adxl345_set_output_rate(dev, data->odr);
	if (rc < 0) {
		LOG_ERR("Rate setting failed\n");
		return rc;
	}

	rc = adxl345_reg_write_byte(dev, ADXL345_POWER_CTL_REG, ADXL345_POWER_CTL_REG_MEASURE_BIT);
	if (rc < 0) {
		LOG_ERR("Enable measure bit failed\n");
		return -EIO;
	}

#ifdef CONFIG_ADXL345_TRIGGER
	rc = adxl345_init_interrupt(dev);
	if (rc != 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return -EIO;
	}
#endif

	return 0;
}

#ifdef CONFIG_ADXL345_TRIGGER
#define ADXL345_CFG_IRQ(inst) .interrupt = GPIO_DT_SPEC_INST_GET(inst, int1_gpios),
#else
#define ADXL345_CFG_IRQ(inst)
#endif /* CONFIG_ADXL345_TRIGGER */

#define ADXL345_CONFIG(inst)                                                                       \
	.frequency = DT_INST_PROP(inst, frequency), .range = DT_INST_PROP(inst, range),            \
	.fifo_mode = DT_INST_PROP(inst, fifo), ADXL345_CFG_IRQ(inst)

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
		ADXL345_CONFIG(inst)                                   \
	}

#define ADXL345_CONFIG_I2C(inst)			    \
	{						    \
		.bus = {.i2c = I2C_DT_SPEC_INST_GET(inst)}, \
		.bus_is_ready = adxl345_bus_is_ready_i2c,   \
		.reg_access = adxl345_reg_access_i2c,	    \
		ADXL345_CONFIG(inst)                        \
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
