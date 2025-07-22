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

static const uint8_t adxl345_fifo_ctl_trigger_init[] = {
	[ADXL345_FIFO_CTL_TRIGGER_INT1] = ADXL345_INT1,
	[ADXL345_FIFO_CTL_TRIGGER_INT2] = ADXL345_INT2,
	[ADXL345_FIFO_CTL_TRIGGER_UNSET] = ADXL345_INT_UNSET,
};

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
	uint8_t regval, tmp;
	int ret;

	ret = adxl345_reg_read_byte(dev, reg_addr, &regval);
	if (ret) {
		return ret;
	}

	tmp = regval & ~mask;
	tmp |= data & mask;

	return adxl345_reg_write_byte(dev, reg_addr, tmp);
}

int adxl345_reg_assign_bits(const struct device *dev, uint8_t reg, uint8_t mask,
			    bool en)
{
	return adxl345_reg_write_mask(dev, reg, mask, en ? mask : 0x00);
}

static inline bool adxl345_bus_is_ready(const struct device *dev)
{
	const struct adxl345_dev_config *cfg = dev->config;

	return cfg->bus_is_ready(&cfg->bus);
}

int adxl345_set_measure_en(const struct device *dev, bool en)
{
	return adxl345_reg_assign_bits(dev, ADXL345_POWER_CTL_REG,
				       ADXL345_POWER_CTL_MODE_MSK, en);
}

int adxl345_get_fifo_entries(const struct device *dev)
{
	uint8_t regval;
	int rc;

	rc = adxl345_reg_read_byte(dev, ADXL345_FIFO_STATUS_REG, &regval);
	if (rc) {
		return rc;
	}

	return FIELD_GET(ADXL345_FIFO_ENTRIES_MSK, regval);
}

int adxl345_get_status(const struct device *dev, uint8_t *status)
{
	return adxl345_reg_read_byte(dev, ADXL345_INT_SOURCE_REG, status);
}

int adxl345_flush_fifo(const struct device *dev)
{
#ifdef CONFIG_ADXL345_TRIGGER
	int8_t sample_number;
	uint8_t regval;
	int rc;

	rc = adxl345_set_measure_en(dev, false);
	if (rc) {
		return rc;
	}

	sample_number = adxl345_get_fifo_entries(dev);
	while (sample_number >= 0) { /* Read FIFO entries + 1 sample lines */
		rc = adxl345_reg_read(dev, ADXL345_REG_DATA_XYZ_REGS,
				      &regval, ADXL345_FIFO_SAMPLE_SIZE);
		if (rc) {
			return rc;
		}

		sample_number--;
	}
#endif /* CONFIG_ADXL345_TRIGGER */

	return adxl345_set_measure_en(dev, true);
}

/**
 * Configure the operating parameters for the FIFO.
 * @param dev - The device structure.
 * @param mode - FIFO Mode. Specifies FIFO operating mode, currently either
 *		ADXL345_FIFO_BYPASSED or ADXL345_FIFO_STREAMED.
 * @param trigger - Currently ignored, thus set to ADXL345_INT_UNSET.
 * @param fifo_samples - FIFO Samples. Watermark number of FIFO samples that
 *			triggers a FIFO_FULL condition when reached.
 *			Values range from 0 to 32.
 *
 * Note: The terms "trigger", "ADXL345_FIFO_TRIGGERED", "ADXL345_FIFO_STREAMED"
 * and "ADXL345_FIFO_BYPASSED" in this context are specific to the ADXL345
 * sensor and are defined in its datasheet. These terms are unrelated to
 * Zephyr's TRIGGER or STREAM APIs.
 *
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl345_configure_fifo(const struct device *dev,
				  enum adxl345_fifo_mode mode,
				  enum adxl345_fifo_trigger trigger,
				  uint8_t fifo_samples)
{
	struct adxl345_dev_data *data = dev->data;
	uint8_t fifo_config;

	fifo_config = adxl345_fifo_ctl_mode_init[mode];
	data->fifo_config.fifo_mode = mode;

	fifo_config |= adxl345_fifo_ctl_trigger_init[trigger];
	data->fifo_config.fifo_trigger = trigger;

	fifo_samples = MIN(fifo_samples, ADXL345_FIFO_CTL_SAMPLES_MSK);
	fifo_config |= fifo_samples;
	data->fifo_config.fifo_samples = fifo_samples;

	return adxl345_reg_write_byte(dev, ADXL345_FIFO_CTL_REG, fifo_config);
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
		odr = ADXL345_ODR_12_5HZ;
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

	data->odr = odr;

	return adxl345_reg_write_mask(dev, ADXL345_RATE_REG, ADXL345_ODR_MSK,
				      ADXL345_ODR_MODE(odr));
}

static int adxl345_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return adxl345_attr_set_odr(dev, chan, attr, val);
	case SENSOR_ATTR_UPPER_THRESH:
		return adxl345_reg_write_byte(dev, ADXL345_THRESH_ACT_REG, val->val1);
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
			adxl345_get_status(dev, &status1);
		} while (!(ADXL345_STATUS_DATA_RDY(status1)));
	}

	int rc = adxl345_reg_read(dev, ADXL345_REG_DATA_XYZ_REGS,
				  axis_data, ADXL345_FIFO_SAMPLE_SIZE);
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
	int rc;

	rc = adxl345_read_sample(dev, &sample);
	if (rc < 0) {
		LOG_ERR("Failed to fetch sample rc=%d\n", rc);
		return rc;
	}
	data->samples.x = sample.x;
	data->samples.y = sample.y;
	data->samples.z = sample.z;

	return 0;
}

static int adxl345_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adxl345_dev_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		adxl345_accel_convert(val, data->samples.x);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl345_accel_convert(val, data->samples.y);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl345_accel_convert(val, data->samples.z);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl345_accel_convert(val++, data->samples.x);
		adxl345_accel_convert(val++, data->samples.y);
		adxl345_accel_convert(val,   data->samples.z);
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

static int adxl345_init(const struct device *dev)
{
	struct adxl345_dev_data *data = dev->data;
	const struct adxl345_dev_config *cfg = dev->config;
	enum adxl345_fifo_mode fifo_mode;
	uint8_t dev_id;
	uint8_t int_en;
	uint8_t regval;
	int rc;

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
	data->selected_range = ADXL345_RANGE_8G;
	data->is_full_res = true;

	/*
	 * Reset the following sensor fields (in case of warm starts)
	 * - turn off measurements as MSB values, use left justified vals
	 * - configure full resolution accordingly
	 * - turn off interrupt inversion
	 * - turn off 3-wire SPI
	 * - turn off self test mode
	 */
	regval = (data->is_full_res ? ADXL345_DATA_FORMAT_FULL_RES : 0x00);
	regval |= adxl345_range_init[data->selected_range];
	rc = adxl345_reg_write_byte(dev, ADXL345_DATA_FORMAT_REG, regval);
	if (rc < 0) {
		LOG_ERR("Data format set failed\n");
		return -EIO;
	}

	rc = adxl345_reg_write_mask(dev, ADXL345_RATE_REG, ADXL345_ODR_MSK,
				    ADXL345_ODR_MODE(cfg->odr));
	if (rc) {
		LOG_ERR("Rate setting failed\n");
		return rc;
	}

	fifo_mode = ADXL345_FIFO_BYPASSED;
	int_en = 0x00;
#if defined(CONFIG_ADXL345_TRIGGER) || defined(CONFIG_ADXL345_STREAM)
	if (adxl345_init_interrupt(dev)) {
		LOG_INF("No IRQ lines specified, fallback to FIFO BYPASSED");
		fifo_mode = ADXL345_FIFO_BYPASSED;
	} else {
		LOG_INF("Set FIFO STREAMED mode");
		fifo_mode = ADXL345_FIFO_STREAMED;

		/*
		 * Currently, map all interrupts to the (same) gpio line
		 * configured in the device tree. This is usually sufficient,
		 * also since not every hardware will have both gpio lines
		 * soldered. Anyway, for individual interrupt mapping, set up
		 * DTB bindings.
		 */
		uint8_t int_mask = UCHAR_MAX;

		rc = adxl345_reg_assign_bits(dev, ADXL345_INT_MAP_REG, int_mask,
					     (cfg->drdy_pad == 2));
		if (rc) {
			return rc;
		}
	}
#endif
	rc = adxl345_configure_fifo(dev, fifo_mode, ADXL345_INT_UNSET,
				    ADXL345_FIFO_CTL_SAMPLES_MSK);
	if (rc) {
		return rc;
	}

	if (fifo_mode == ADXL345_FIFO_BYPASSED) {
		return adxl345_set_measure_en(dev, true);
	}

	return 0;
}

#ifdef CONFIG_ADXL345_TRIGGER
#define ADXL345_CFG_IRQ(inst) \
	.gpio_int1 = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0}),	\
	.gpio_int2 = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, {0}),	\
	.drdy_pad = DT_INST_PROP_OR(inst, drdy_pin, -1),
#else
#define ADXL345_CFG_IRQ(inst)
#endif /* CONFIG_ADXL345_TRIGGER */

#define ADXL345_CONFIG_COMMON(inst)	\
	IF_ENABLED(UTIL_OR(DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
			   DT_INST_NODE_HAS_PROP(inst, int2_gpios)),	\
			   (ADXL345_CFG_IRQ(inst)))

#define ADXL345_RTIO_SPI_DEFINE(inst)								   \
	COND_CODE_1(CONFIG_SPI_RTIO,								   \
			(SPI_DT_IODEV_DEFINE(adxl345_iodev_##inst, DT_DRV_INST(inst),		   \
			SPI_WORD_SET(8) | SPI_TRANSFER_MSB |					   \
			SPI_MODE_CPOL | SPI_MODE_CPHA);),					   \
			())

#define ADXL345_RTIO_I2C_DEFINE(inst)								   \
	COND_CODE_1(CONFIG_I2C_RTIO,								   \
			(I2C_DT_IODEV_DEFINE(adxl345_iodev_##inst, DT_DRV_INST(inst));),	   \
			())

	/** RTIO SQE/CQE pool size depends on the fifo-watermark because we
	 * can't just burst-read all the fifo data at once. Datasheet specifies
	 * we need to get one frame at a time (through the Data registers),
	 * therefore, we set all the sequence at once to properly pull each
	 * frame, and then end up calling the completion event so the
	 * application receives it).
	 */
#define ADXL345_RTIO_DEFINE(inst)								   \
	/* Conditionally include SPI and/or I2C parts based on their presence */		   \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),							   \
				(ADXL345_RTIO_SPI_DEFINE(inst)),				   \
				())								   \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c),							   \
				(ADXL345_RTIO_I2C_DEFINE(inst)),				   \
				())								   \
	RTIO_DEFINE(adxl345_rtio_ctx_##inst,							   \
		    2 * DT_INST_PROP(inst, fifo_watermark) + 2,					   \
		    2 * DT_INST_PROP(inst, fifo_watermark) + 2);

#define ADXL345_CONFIG(inst)									   \
		.odr = DT_INST_PROP_OR(inst, odr, ADXL345_RATE_25HZ),

#define ADXL345_CONFIG_SPI(inst)								   \
	{											   \
		.bus = {.spi = SPI_DT_SPEC_INST_GET(inst,					   \
						    SPI_WORD_SET(8) |				   \
						    SPI_TRANSFER_MSB |				   \
						    SPI_MODE_CPOL |				   \
						    SPI_MODE_CPHA)},				   \
		.bus_is_ready = adxl345_bus_is_ready_spi,					   \
		.reg_access = adxl345_reg_access_spi,						   \
		.bus_type = ADXL345_BUS_SPI,							   \
		ADXL345_CONFIG(inst)								   \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int1_gpios),				   \
				(ADXL345_CONFIG_COMMON(inst)), ())				   \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int2_gpios),				   \
				(ADXL345_CONFIG_COMMON(inst)), ())				   \
	}

#define ADXL345_CONFIG_I2C(inst)								   \
	{											   \
		.bus = {.i2c = I2C_DT_SPEC_INST_GET(inst)},					   \
		.bus_is_ready = adxl345_bus_is_ready_i2c,					   \
		.reg_access = adxl345_reg_access_i2c,						   \
		.bus_type = ADXL345_BUS_I2C,							   \
		ADXL345_CONFIG(inst)								   \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int1_gpios),				   \
				(ADXL345_CONFIG_COMMON(inst)), ())				   \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int2_gpios),				   \
				(ADXL345_CONFIG_COMMON(inst)), ())				   \
	}

#define ADXL345_DEFINE(inst)									   \
												   \
	BUILD_ASSERT(!IS_ENABLED(CONFIG_ADXL345_STREAM) ||					   \
		     DT_INST_NODE_HAS_PROP(inst, fifo_watermark),				   \
		     "Streaming requires fifo-watermark property. Please set it in the"		   \
		     "device-tree node properties");						   \
	BUILD_ASSERT(COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, fifo_watermark),			   \
				 ((DT_INST_PROP(inst, fifo_watermark) > 1) &&			   \
				  (DT_INST_PROP(inst, fifo_watermark) < 32)),			   \
				 (true)),							   \
		     "fifo-watermark must be between 2 and 32. Please set it in "		   \
		     "the device-tree node properties");					   \
												   \
	IF_ENABLED(CONFIG_ADXL345_STREAM, (ADXL345_RTIO_DEFINE(inst)));				   \
	static struct adxl345_dev_data adxl345_data_##inst = {					   \
	IF_ENABLED(CONFIG_ADXL345_STREAM, (.rtio_ctx = &adxl345_rtio_ctx_##inst,		   \
				.iodev = &adxl345_iodev_##inst,))				   \
	};											   \
	static const struct adxl345_dev_config adxl345_config_##inst =				   \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (ADXL345_CONFIG_SPI(inst)),		   \
			    (ADXL345_CONFIG_I2C(inst)));					   \
												   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adxl345_init, NULL,					   \
			      &adxl345_data_##inst, &adxl345_config_##inst, POST_KERNEL,	   \
			      CONFIG_SENSOR_INIT_PRIORITY, &adxl345_api_funcs); \
										\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, drdy_pin),					   \
					 (BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst,		   \
					 CONCAT(int, DT_INST_PROP(inst, drdy_pin), _gpios)),	   \
					 "No GPIO pin defined for ADXL345 DRDY interrupt");))

DT_INST_FOREACH_STATUS_OKAY(ADXL345_DEFINE)
