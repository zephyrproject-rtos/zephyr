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

static const uint8_t adxl345_fifo_ctl_trigger_init[] = {
	[ADXL345_FIFO_CTL_TRIGGER_INT1] = ADXL345_INT1,
	[ADXL345_FIFO_CTL_TRIGGER_INT2] = ADXL345_INT2,
	[ADXL345_FIFO_CTL_TRIGGER_UNSET] = ADXL345_INT_UNSET,
};

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
	uint8_t access = reg_addr;

	access |= cmd;
	access |= (length == 1) ? 0 : ADXL345_MULTIBYTE_FLAG;

	const struct spi_buf buf[] = {
		{.buf = &access, .len = 1},
		{.buf = data, .len = length},
	};

	const struct spi_buf_set rx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	struct spi_buf_set tx = {
		.buffers = buf,
		.count = (cmd == ADXL345_READ_CMD) ? 1 : 2,
	};

	if (cmd == ADXL345_READ_CMD) {
		return spi_transceive_dt(&cfg->bus.spi, &tx, &rx);
	}
	return spi_write_dt(&cfg->bus.spi, &tx);
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

int adxl345_reg_update_bits(const struct device *dev, uint8_t reg,
			    uint8_t mask, uint8_t val)
{
	uint8_t regval, tmp;
	int rc;

	rc = adxl345_reg_read_byte(dev, reg, &regval);
	if (rc) {
		return rc;
	}

	tmp = regval & ~mask;
	tmp |= val & mask;

	return adxl345_reg_write_byte(dev, reg, tmp);
}

static inline bool adxl345_bus_is_ready(const struct device *dev)
{
	const struct adxl345_dev_config *cfg = dev->config;

	return cfg->bus_is_ready(&cfg->bus);
}

static const uint8_t adxl345_op_mode_init[] = {
	[ADXL345_OP_STANDBY] = ADXL345_POWER_CTL_STANDBY_MODE,
	[ADXL345_OP_MEASURE] = ADXL345_POWER_CTL_MEASURE_MODE,
};

int adxl345_set_measure_en(const struct device *dev, bool en)
{
	uint8_t val = adxl345_op_mode_init[en ? ADXL345_OP_MEASURE
		 : ADXL345_OP_STANDBY];

	return adxl345_reg_update_bits(dev, ADXL345_REG_POWER_CTL,
				      ADXL345_POWER_CTL_MODE_MSK, val);
}

int adxl345_get_fifo_entries(const struct device *dev, uint8_t *fifo_entries)
{
	uint8_t regval;
	int rc;

	rc = adxl345_reg_read_byte(dev, ADXL345_REG_FIFO_STATUS, &regval);
	if (rc) {
		return rc;
	}

	*fifo_entries = FIELD_GET(ADLX345_FIFO_STATUS_ENTRIES_MSK, regval);

	return 0;
}

int adxl345_get_status(const struct device *dev, uint8_t *status)
{
	return adxl345_reg_read_byte(dev, ADXL345_REG_INT_SOURCE, status);
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

	data->odr = odr;

	return adxl345_reg_update_bits(dev, ADXL345_REG_RATE, ADXL345_ODR_MSK,
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
	default:
		return -ENOTSUP;
	}
}

int adxl345_get_accel_data(const struct device *dev,
			   struct adxl345_xyz_accel_data *sample)
{
	uint8_t axis_data[ADXL345_FIFO_SAMPLE_SIZE];
	int rc;

	rc = adxl345_reg_read(dev, ADXL345_REG_DATA_XYZ_REGS,
			      axis_data, ADXL345_FIFO_SAMPLE_SIZE);
	if (rc < 0) {
		LOG_ERR("Samples read failed with rc=%d\n", rc);
		return rc;
	}

	sample->x = axis_data[0] | axis_data[1] << 8;
	sample->y = axis_data[2] | axis_data[3] << 8;
	sample->z = axis_data[4] | axis_data[5] << 8;

#ifdef CONFIG_ADXL345_TRIGGER
	struct adxl345_dev_data *data = dev->data;

	sample->is_full_res = data->is_full_res; /* needed for decoder */
	sample->selected_range = data->selected_range;
#endif

	return rc;
}

/**
 * adxl345_accel_convert - The fallback conversion of raw measurements.
 * @out: Converted value for output, containing the initialized fractional.
 * @sample: Input raw measurement.
 * When working without decoder, neither TRIGGER, nor STREAM is enabled,
 * this small converter is used. It assumes full scale resolution and 8g.
 */
void adxl345_accel_convert(struct sensor_value *out, int16_t sample)
{
	/* full resolution enabled w/ 8g */
	if (sample & BIT(11)) {
		sample |= ADXL345_COMPLEMENT_MASK(12);
	}
	out->val1 = ((sample * SENSOR_G) / 32) / 1000000;
	out->val2 = ((sample * SENSOR_G) / 32) % 1000000;
}

static int adxl345_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct adxl345_dev_data *data = dev->data;
	uint8_t count;
	int rc;

	count = 1;

	/* FIFO BYPASSED is the only mode not using a FIFO buffer */
	if (data->fifo_config.fifo_mode != ADXL345_FIFO_BYPASSED) {
		rc = adxl345_get_fifo_entries(dev, &count);
		if (rc) {
			LOG_ERR("Failed to read FIFO status rc=%d\n", rc);
			return rc;
		}
	}

	__ASSERT_NO_MSG(count <= ARRAY_SIZE(data->sample));

	for (uint8_t s = 0; s < count; s++) {
		rc = adxl345_get_accel_data(dev, &data->sample[s]);
		if (rc < 0) {
			LOG_ERR("Failed to fetch sample rc=%d\n", rc);
			return rc;
		}

#ifdef CONFIG_ADXL345_STREAM
		data->sample[s].is_fifo = 0;
#endif
	}

	/* new sample available, reset book-keeping */
	data->sample_idx = 0;
	data->sample_number = count;

	return 0;
}

/**
 * adxl345_channel_get - Read a single element of one or three axis.
 * @dev: The sensor device.
 * @chan: The axis channel, can be x, y, z or xyz.
 * @val: The resulting value after conversion of the raw axis data. Val can be
 * a single value or an array of three values, where the index correspondes to
 * x, y, z axis entries.
 */
static int adxl345_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adxl345_dev_data *data = dev->data;
	int idx;

	if (data->sample_number <= 0) { /* empty */
		val->val1 = 0;
		val->val2 = 0;
		if (chan == SENSOR_CHAN_ACCEL_XYZ) {
			val[1].val1 = 0;
			val[1].val2 = 0;
			val[2].val1 = 0;
			val[2].val2 = 0;
		}
		return -ENOTSUP;
	}

	data->sample_idx = data->sample_idx % data->sample_number;
	idx = data->sample_idx;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		adxl345_accel_convert(val, data->sample[idx].x);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl345_accel_convert(val, data->sample[idx].y);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl345_accel_convert(val, data->sample[idx].z);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl345_accel_convert(val++, data->sample[idx].x);
		adxl345_accel_convert(val++, data->sample[idx].y);
		adxl345_accel_convert(val,   data->sample[idx].z);

		break;
	default:
		return -ENOTSUP;
	}

	data->sample_idx++;

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
	uint8_t dev_id, pwr_ctl, int_en, regval;
	int rc;

	if (!adxl345_bus_is_ready(dev)) {
		LOG_ERR("bus not ready");
		return -ENODEV;
	}

	rc = adxl345_reg_read_byte(dev, ADXL345_REG_DEVICE_ID, &dev_id);
	if (rc < 0 || dev_id != ADXL345_PART_ID) {
		LOG_ERR("Read PART ID failed: 0x%x\n", rc);
		return -ENODEV;
	}

	/* reset the sensor */
	data->selected_range = ADXL345_RANGE_8G;
	data->is_full_res = true;
	data->sample_number = 0;
	data->sample_idx = 0;

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
	rc = adxl345_reg_write_byte(dev, ADXL345_REG_DATA_FORMAT, regval);
	if (rc < 0) {
		LOG_ERR("Data format set failed\n");
		return -EIO;
	}

	rc = adxl345_reg_update_bits(dev, ADXL345_REG_RATE, ADXL345_ODR_MSK,
				     ADXL345_ODR_MODE(cfg->odr));
	if (rc) {
		return rc;
	}

	pwr_ctl = 0x00;
	int_en = 0x00;
	fifo_mode = ADXL345_FIFO_BYPASSED;

	rc = adxl345_reg_write_byte(dev, ADXL345_REG_INT_ENABLE, int_en);
	if (rc) {
		return rc;
	}

	rc = adxl345_reg_write_byte(dev, ADXL345_REG_POWER_CTL, pwr_ctl);
	if (rc) {
		return rc;
	}

#ifdef CONFIG_ADXL345_TRIGGER
	if (adxl345_init_interrupt(dev)) {
		/* no interrupt lines configured in DT */
		LOG_INF("No IRQ lines specified, fallback to FIFO BYPASSED");
		fifo_mode = ADXL345_FIFO_BYPASSED;
	} else {
		LOG_INF("Set FIFO STREAMED mode");
		fifo_mode = ADXL345_FIFO_STREAMED;

		/*
		 * Currently, map all interrupts to the (same) gpio line
		 * configured in the device tree.
		 */
		uint8_t int_mask = 0xff;

		int_en = (cfg->drdy_pad == 2) ? int_mask : ~int_mask;
		rc = adxl345_reg_update_bits(dev, ADXL345_REG_INT_MAP,
					     int_mask, int_en);
		if (rc) {
			return rc;
		}
	}
#endif /* CONFIG_ADXL345_TRIGGER */

	data->fifo_config.fifo_mode = fifo_mode;
	uint8_t fifo_config = adxl345_fifo_ctl_mode_init[fifo_mode];

	data->fifo_config.fifo_trigger = ADXL345_INT_UNSET;
	fifo_config |= adxl345_fifo_ctl_trigger_init[ADXL345_INT_UNSET];

	data->fifo_config.fifo_samples = ADXL345_FIFO_CTL_SAMPLES_MSK;
	fifo_config |= ADXL345_FIFO_CTL_SAMPLES_MSK;

	rc = adxl345_reg_write_byte(dev, ADXL345_REG_FIFO_CTL, fifo_config);
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
	.drdy_pad = DT_INST_PROP_OR(inst, drdy_pin, -1)
#else
#define ADXL345_CFG_IRQ(inst)
#endif /* CONFIG_ADXL345_TRIGGER */

#define ADXL345_CONFIG_COMMON(inst)	\
	IF_ENABLED(UTIL_OR(DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
			   DT_INST_NODE_HAS_PROP(inst, int2_gpios)),	\
			   (ADXL345_CFG_IRQ(inst)))

#define ADXL345_RTIO_SPI_DEFINE(inst)   \
	COND_CODE_1(CONFIG_SPI_RTIO,    \
			(SPI_DT_IODEV_DEFINE(adxl345_iodev_##inst, DT_DRV_INST(inst), \
			SPI_WORD_SET(8) | SPI_TRANSFER_MSB |            \
			SPI_MODE_CPOL | SPI_MODE_CPHA, 0U);), ())

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
#define ADXL345_RTIO_DEFINE(inst)					\
	/* Conditionally include SPI or I2C parts based on their presence */ \
	(COND_CODE_1(DT_INST_ON_BUS(inst, spi),				\
				(ADXL345_RTIO_SPI_DEFINE(inst)), ())	\
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c),				\
				(ADXL345_RTIO_I2C_DEFINE(inst)), ())	\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, spi_dt_spec) &&	\
				DT_INST_NODE_HAS_PROP(inst, i2c_dt_spec), \
		(RTIO_DEFINE(adxl345_rtio_ctx_##inst, 128, 128);),	\
		(RTIO_DEFINE(adxl345_rtio_ctx_##inst, 64, 64);)))

#define ADXL345_CONFIG(inst)						\
		.odr = DT_INST_PROP_OR(inst, odr, ADXL345_RATE_25HZ),

#define ADXL345_CONFIG_SPI(inst)					\
	{								\
		.bus = {.spi = SPI_DT_SPEC_INST_GET(inst,		\
						    SPI_WORD_SET(8) |	\
						    SPI_TRANSFER_MSB |	\
						    SPI_MODE_CPOL |	\
						    SPI_MODE_CPHA,	\
						    0)},		\
		.bus_is_ready = adxl345_bus_is_ready_spi,		\
		.reg_access = adxl345_reg_access_spi,			\
		.bus_type = ADXL345_BUS_SPI,				\
		ADXL345_CONFIG(inst)					\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
				(ADXL345_CONFIG_COMMON(inst)), ())	\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int2_gpios),	\
				(ADXL345_CONFIG_COMMON(inst)), ())	\
	}

#define ADXL345_CONFIG_I2C(inst)					\
	{								\
		.bus = {.i2c = I2C_DT_SPEC_INST_GET(inst)},		\
		.bus_is_ready = adxl345_bus_is_ready_i2c,		\
		.reg_access = adxl345_reg_access_i2c,			\
		.bus_type = ADXL345_BUS_I2C,				\
		ADXL345_CONFIG(inst)					\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
				(ADXL345_CONFIG_COMMON(inst)), ())	\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int2_gpios),	\
				(ADXL345_CONFIG_COMMON(inst)), ())	\
	}

#define ADXL345_DEFINE(inst)						\
	IF_ENABLED(CONFIG_ADXL345_STREAM, (ADXL345_RTIO_DEFINE(inst)));	\
	\
	static struct adxl345_dev_data adxl345_data_##inst = {		\
		IF_ENABLED(CONFIG_ADXL345_STREAM,			\
			   (.rtio_ctx = &adxl345_rtio_ctx_##inst,	\
			   .iodev = &adxl345_iodev_##inst,))		\
	};								\
	\
	static const struct adxl345_dev_config adxl345_config_##inst =	\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),			\
			    (ADXL345_CONFIG_SPI(inst)),			\
			    (ADXL345_CONFIG_I2C(inst)));		\
	\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adxl345_init, NULL,		\
				     &adxl345_data_##inst,		\
				     &adxl345_config_##inst,		\
				     POST_KERNEL,			\
				     CONFIG_SENSOR_INIT_PRIORITY,	\
				     &adxl345_api_funcs);		\
	\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, drdy_pin),		\
		   (BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst,		\
			   CONCAT(int, DT_INST_PROP(inst, drdy_pin), _gpios)), \
			"No GPIO pin defined for ADXL345 DRDY interrupt");))

DT_INST_FOREACH_STATUS_OKAY(ADXL345_DEFINE)
