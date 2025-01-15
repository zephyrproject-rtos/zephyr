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

int adxl345_reg_access_spi(const struct device *dev, uint8_t cmd, uint8_t reg_addr,
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
	int ret;

	if (cmd == ADXL345_READ_CMD) {
		tx.count = 1;
		ret = spi_transceive_dt(&cfg->bus.spi, &tx, &rx);
		return ret;
	} else {
		ret = spi_write_dt(&cfg->bus.spi, &tx);
		return ret;
	}
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

int adxl345_reg_access(const struct device *dev, uint8_t cmd, uint8_t addr,
				     uint8_t *data, size_t len)
{
	const struct adxl345_dev_config *cfg = dev->config;

	return cfg->reg_access(dev, cmd, addr, data, len);
}

int adxl345_reg_write(const struct device *dev, uint8_t addr, uint8_t *data,
				    uint8_t len)
{

	return adxl345_reg_access(dev, ADXL345_WRITE_CMD, addr, data, len);
}

int adxl345_reg_read(const struct device *dev, uint8_t addr, uint8_t *data,
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

int adxl345_reg_write_mask(const struct device *dev,
			       uint8_t reg_addr,
			       uint8_t mask,
			       uint8_t data)
{
	int ret;
	uint8_t tmp;

	ret = adxl345_reg_read_byte(dev, reg_addr, &tmp);
	if (ret) {
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

int adxl345_get_status(const struct device *dev,
			   uint8_t *status1,
			   uint16_t *fifo_entries)
{
	uint8_t buf[2], length = 1U;
	int ret;

	ret = adxl345_reg_read(dev, ADXL345_INT_SOURCE, buf, length);

	*status1 = buf[0];
	ret = adxl345_reg_read(dev, ADXL345_FIFO_STATUS_REG, buf+1, length);
	if (fifo_entries) {
		*fifo_entries = buf[1] & 0x3F;
	}

	return ret;
}

/**
 * Configure the operating parameters for the FIFO.
 * @param dev - The device structure.
 * @param mode - FIFO Mode. Specifies FIFO operating mode.
 *		Accepted values: ADXL345_FIFO_BYPASSED
 *				 ADXL345_FIFO_STREAMED
 *				 ADXL345_FIFO_TRIGGERED
 *				 ADXL345_FIFO_OLD_SAVED
 * @param trigger - FIFO trigger. Links trigger event to appropriate INT.
 *		Accepted values: ADXL345_INT1
 *				 ADXL345_INT2
 * @param fifo_samples - FIFO Samples. Watermark number of FIFO samples that
 *			triggers a FIFO_FULL condition when reached.
 *			Values range from 0 to 32.

 * @return 0 in case of success, negative error code otherwise.
 */
int adxl345_configure_fifo(const struct device *dev,
				  enum adxl345_fifo_mode mode,
				  enum adxl345_fifo_trigger trigger,
				  uint16_t fifo_samples)
{
	struct adxl345_dev_data *data = dev->data;
	uint8_t fifo_config;
	int ret;

	if (fifo_samples > 32) {
		return -EINVAL;
	}

	fifo_config = (ADXL345_FIFO_CTL_TRIGGER_MODE(trigger) |
		       ADXL345_FIFO_CTL_MODE_MODE(mode) |
		       ADXL345_FIFO_CTL_SAMPLES_MODE(fifo_samples));

	ret = adxl345_reg_write_byte(dev, ADXL345_FIFO_CTL_REG, fifo_config);
	if (ret) {
		return ret;
	}

	data->fifo_config.fifo_trigger = trigger;
	data->fifo_config.fifo_mode = mode;
	data->fifo_config.fifo_samples = fifo_samples;

	return 0;
}

/**
 * Set the mode of operation.
 * @param dev - The device structure.
 * @param op_mode - Mode of operation.
 *		Accepted values: ADXL345_STANDBY
 *				 ADXL345_MEASURE
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl345_set_op_mode(const struct device *dev, enum adxl345_op_mode op_mode)
{
	return adxl345_reg_write_mask(dev, ADXL345_POWER_CTL_REG,
					   ADXL345_POWER_CTL_MEASURE_MSK,
					   ADXL345_POWER_CTL_MEASURE_MODE(op_mode));
}

/**
 * Set Output data rate.
 * @param dev - The device structure.
 * @param odr - Output data rate.
 *		Accepted values: ADXL345_ODR_12HZ
 *				 ADXL345_ODR_25HZ
 *				 ADXL345_ODR_50HZ
 *				 ADXL345_ODR_100HZ
 *				 ADXL345_ODR_200HZ
 *				 ADXL345_ODR_400HZ
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl345_set_odr(const struct device *dev, enum adxl345_odr odr)
{
	return adxl345_reg_write_mask(dev, ADXL345_RATE_REG,
					   ADXL345_ODR_MSK,
					   ADXL345_ODR_MODE(odr));
}

static int adxl345_attr_set_odr(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	enum adxl345_odr odr;
	struct adxl345_dev_data *data = dev->data;

	switch (val->val1) {
	case 12:
		odr = ADXL345_ODR_12HZ;
		break;
	case 25:
		odr = ADXL345_ODR_25HZ;
		break;
	case 50:
		odr = ADXL345_ODR_50HZ;
		break;
	case 100:
		odr = ADXL345_ODR_100HZ;
		break;
	case 200:
		odr = ADXL345_ODR_200HZ;
		break;
	case 400:
		odr = ADXL345_ODR_400HZ;
		break;
	default:
		return -EINVAL;
	}

	int ret = adxl345_set_odr(dev, odr);

	if (ret == 0) {
		data->odr = odr;
	}

	return ret;
}

static int adxl345_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return adxl345_attr_set_odr(dev, chan, attr, val);
	default:
		return -ENOTSUP;
	}
}

int adxl345_read_sample(const struct device *dev,
			       struct adxl345_sample *sample)
{
	int16_t raw_x, raw_y, raw_z;
	uint8_t axis_data[6], status1;
	struct adxl345_dev_data *data = dev->data;

	if (!IS_ENABLED(CONFIG_ADXL345_TRIGGER)) {
		do {
			adxl345_get_status(dev, &status1, NULL);
		} while (!(ADXL345_STATUS_DATA_RDY(status1)));
	}

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

	sample->selected_range = data->selected_range;
	sample->is_full_res = data->is_full_res;

	return 0;
}

void adxl345_accel_convert(struct sensor_value *val, int16_t sample)
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

static DEVICE_API(sensor, adxl345_api_funcs) = {
	.attr_set = adxl345_attr_set,
	.sample_fetch = adxl345_sample_fetch,
	.channel_get = adxl345_channel_get,
#ifdef CONFIG_ADXL345_TRIGGER
	.trigger_set = adxl345_trigger_set,
#endif
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = adxl345_submit,
	.get_decoder = adxl345_get_decoder,
#endif
};

#ifdef CONFIG_ADXL345_TRIGGER
/**
 * Configure the INT1 and INT2 interrupt pins.
 * @param dev - The device structure.
 * @param int1 -  INT1 interrupt pins.
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl345_interrupt_config(const struct device *dev,
				    uint8_t int1)
{
	int ret;
	const struct adxl345_dev_config *cfg = dev->config;

	ret = adxl345_reg_write_byte(dev, ADXL345_INT_MAP, int1);
	if (ret) {
		return ret;
	}

	ret = adxl345_reg_write_byte(dev, ADXL345_INT_ENABLE, int1);
	if (ret) {
		return ret;
	}

	uint8_t samples;

	ret = adxl345_reg_read_byte(dev, ADXL345_INT_MAP, &samples);
	ret = adxl345_reg_read_byte(dev, ADXL345_INT_ENABLE, &samples);
#ifdef CONFIG_ADXL345_TRIGGER
	gpio_pin_interrupt_configure_dt(&cfg->interrupt,
					      GPIO_INT_EDGE_TO_ACTIVE);
#endif
	return 0;
}
#endif

static int adxl345_init(const struct device *dev)
{
	int rc;
	struct adxl345_dev_data *data = dev->data;
#ifdef CONFIG_ADXL345_TRIGGER
	const struct adxl345_dev_config *cfg = dev->config;
#endif
	uint8_t dev_id, full_res;

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

#if CONFIG_ADXL345_STREAM
	rc = adxl345_reg_write_byte(dev, ADXL345_FIFO_CTL_REG, ADXL345_FIFO_STREAM_MODE);
	if (rc < 0) {
		LOG_ERR("FIFO enable failed\n");
		return -EIO;
	}
#endif

	rc = adxl345_reg_write_byte(dev, ADXL345_DATA_FORMAT_REG, ADXL345_RANGE_8G);
	if (rc < 0) {
		LOG_ERR("Data format set failed\n");
		return -EIO;
	}

	data->selected_range = ADXL345_RANGE_8G;

	rc = adxl345_reg_write_byte(dev, ADXL345_RATE_REG, ADXL345_RATE_25HZ);
	if (rc < 0) {
		LOG_ERR("Rate setting failed\n");
		return -EIO;
	}

#ifdef CONFIG_ADXL345_TRIGGER
	rc = adxl345_configure_fifo(dev, ADXL345_FIFO_STREAMED,
				     ADXL345_INT2,
				     SAMPLE_NUM);
	if (rc) {
		return rc;
	}
#endif

	rc = adxl345_reg_write_byte(dev, ADXL345_POWER_CTL_REG, ADXL345_ENABLE_MEASURE_BIT);
	if (rc < 0) {
		LOG_ERR("Enable measure bit failed\n");
		return -EIO;
	}

#ifdef CONFIG_ADXL345_TRIGGER
	rc = adxl345_init_interrupt(dev);
	if (rc < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return -EIO;
	}

	rc = adxl345_set_odr(dev, cfg->odr);
	if (rc) {
		return rc;
	}
	rc = adxl345_interrupt_config(dev, ADXL345_INT_MAP_WATERMARK_MSK);
	if (rc) {
		return rc;
	}
#endif

	rc = adxl345_reg_read_byte(dev, ADXL345_DATA_FORMAT_REG, &full_res);
	uint8_t is_full_res_set = (full_res & ADXL345_DATA_FORMAT_FULL_RES) != 0;

	data->is_full_res = is_full_res_set;
	return 0;
}

#ifdef CONFIG_ADXL345_TRIGGER
#define ADXL345_CFG_IRQ(inst) \
		.interrupt = GPIO_DT_SPEC_INST_GET(inst, int2_gpios),
#else
#define ADXL345_CFG_IRQ(inst)
#endif /* CONFIG_ADXL345_TRIGGER */

#define ADXL345_RTIO_SPI_DEFINE(inst)   \
	COND_CODE_1(CONFIG_SPI_RTIO,    \
			(SPI_DT_IODEV_DEFINE(adxl345_iodev_##inst, DT_DRV_INST(inst), \
			SPI_WORD_SET(8) | SPI_TRANSFER_MSB |            \
			SPI_MODE_CPOL | SPI_MODE_CPHA, 0U);),    \
			())

#define ADXL345_RTIO_I2C_DEFINE(inst)    \
	COND_CODE_1(CONFIG_I2C_RTIO, \
			(I2C_DT_IODEV_DEFINE(adxl345_iodev_##inst, DT_DRV_INST(inst));),  \
			())

	/* Conditionally set the RTIO size based on the presence of SPI/I2C
	 * lines 541 - 542.
	 * The sizes of sqe and cqe pools are increased due to the amount of
	 * multibyte reads needed for watermark using 31 samples
	 * (adx345_stram - line 203), using smaller amounts of samples
	 * to trigger an interrupt can decrease the pool sizes.
	 */
#define ADXL345_RTIO_DEFINE(inst)                                      \
	/* Conditionally include SPI and/or I2C parts based on their presence */ \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),  \
				(ADXL345_RTIO_SPI_DEFINE(inst)), \
				())       \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c),     \
				(ADXL345_RTIO_I2C_DEFINE(inst)),        \
				())                                  \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, spi_dt_spec) &&           \
				DT_INST_NODE_HAS_PROP(inst, i2c_dt_spec),              \
		(RTIO_DEFINE(adxl345_rtio_ctx_##inst, 128, 128);),              \
		(RTIO_DEFINE(adxl345_rtio_ctx_##inst, 64, 64);))               \

#define ADXL345_CONFIG(inst)								\
		.odr = DT_INST_PROP(inst, odr),						\
		.fifo_config.fifo_mode = ADXL345_FIFO_STREAMED,				\
		.fifo_config.fifo_trigger = ADXL345_INT2,			\
		.fifo_config.fifo_samples = SAMPLE_NUM,					\
		.odr = ADXL345_RATE_25HZ,						\

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
		.bus_type = ADXL345_BUS_SPI,       \
		ADXL345_CONFIG(inst)					\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int2_gpios),	\
		(ADXL345_CFG_IRQ(inst)), ())				\
	}

#define ADXL345_CONFIG_I2C(inst)			    \
	{						    \
		.bus = {.i2c = I2C_DT_SPEC_INST_GET(inst)}, \
		.bus_is_ready = adxl345_bus_is_ready_i2c,   \
		.reg_access = adxl345_reg_access_i2c,	    \
		.bus_type = ADXL345_BUS_I2C,                \
		ADXL345_CONFIG(inst)					\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int2_gpios),	\
		(ADXL345_CFG_IRQ(inst)), ())		\
	}

#define ADXL345_DEFINE(inst)								\
	IF_ENABLED(CONFIG_ADXL345_STREAM, (ADXL345_RTIO_DEFINE(inst)));                 \
	static struct adxl345_dev_data adxl345_data_##inst = {                  \
	COND_CODE_1(adxl345_iodev_##inst, (.rtio_ctx = &adxl345_rtio_ctx_##inst,        \
				.iodev = &adxl345_iodev_##inst,), ()) \
	};     \
	static const struct adxl345_dev_config adxl345_config_##inst =                  \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (ADXL345_CONFIG_SPI(inst)),      \
			    (ADXL345_CONFIG_I2C(inst)));                                \
                                                                                        \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adxl345_init, NULL,				\
			      &adxl345_data_##inst, &adxl345_config_##inst, POST_KERNEL,\
			      CONFIG_SENSOR_INIT_PRIORITY, &adxl345_api_funcs);		\

DT_INST_FOREACH_STATUS_OKAY(ADXL345_DEFINE)
