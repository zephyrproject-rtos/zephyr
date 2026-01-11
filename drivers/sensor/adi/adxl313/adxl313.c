/*
 * Copyright (c) 2025 Lothar Rubusch <l.rubusch@gmail.com>
 *
 * Datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL313.pdf
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl313

#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "adxl313.h"

LOG_MODULE_REGISTER(ADXL313, CONFIG_SENSOR_LOG_LEVEL);

static const uint8_t adxl313_range_init[] = {
	[ADXL313_RANGE_0_5G] = ADXL313_DT_RANGE_0_5G,
	[ADXL313_RANGE_1G] = ADXL313_DT_RANGE_1G,
	[ADXL313_RANGE_2G] = ADXL313_DT_RANGE_2G,
	[ADXL313_RANGE_4G] = ADXL313_DT_RANGE_4G,
};

static const uint8_t adxl313_fifo_ctl_mode_init[] = {
	[ADXL313_FIFO_BYPASSED] = ADXL313_FIFO_CTL_MODE_BYPASSED,
	[ADXL313_FIFO_OLD_SAVED] = ADXL313_FIFO_CTL_MODE_OLD_SAVED,
	[ADXL313_FIFO_STREAMED] = ADXL313_FIFO_CTL_MODE_STREAMED,
	[ADXL313_FIFO_TRIGGERED] = ADXL313_FIFO_CTL_MODE_TRIGGERED,
};

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
static bool adxl313_bus_is_ready_i2c(const union adxl313_bus *bus)
{
	return device_is_ready(bus->i2c.bus);
}

static int adxl313_reg_access_i2c(const struct device *dev, uint8_t cmd, uint8_t reg_addr,
				  uint8_t *data, size_t length)
{
	const struct adxl313_dev_config *cfg = dev->config;

	if (cmd == ADXL313_READ_CMD) {
		return i2c_burst_read_dt(&cfg->bus.i2c, reg_addr, data, length);
	} else {
		return i2c_burst_write_dt(&cfg->bus.i2c, reg_addr, data, length);
	}
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
static bool adxl313_bus_is_ready_spi(const union adxl313_bus *bus)
{
	return spi_is_ready_dt(&bus->spi);
}

int adxl313_reg_access_spi(const struct device *dev, uint8_t cmd, uint8_t reg_addr, uint8_t *data,
			   size_t length)
{
	const struct adxl313_dev_config *cfg = dev->config;
	uint8_t access = reg_addr | cmd | (length == 1 ? 0 : ADXL313_MULTIBYTE_FLAG);
	const struct spi_buf buf[2] = {{.buf = &access, .len = 1}, {.buf = data, .len = length}};
	const struct spi_buf_set rx = {.buffers = buf, .count = ARRAY_SIZE(buf)};
	struct spi_buf_set tx = {
		.buffers = buf,
		.count = 2,
	};
	int ret;

	if (cmd == ADXL313_READ_CMD) {
		tx.count = 1;
		ret = spi_transceive_dt(&cfg->bus.spi, &tx, &rx);
	} else {
		ret = spi_write_dt(&cfg->bus.spi, &tx);
	}

	return ret;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

int adxl313_reg_access(const struct device *dev, uint8_t cmd, uint8_t addr, uint8_t *data,
		       size_t len)
{
	const struct adxl313_dev_config *cfg = dev->config;

	return cfg->reg_access(dev, cmd, addr, data, len);
}

int adxl313_reg_write(const struct device *dev, uint8_t addr, uint8_t *data, uint8_t len)
{
	return adxl313_reg_access(dev, ADXL313_WRITE_CMD, addr, data, len);
}

int adxl313_raw_reg_read(const struct device *dev, uint8_t addr, uint8_t *data, uint8_t len)
{
	return adxl313_reg_access(dev, ADXL313_READ_CMD, addr, data, len);
}

/* registers not to be cached */
static uint8_t adxl313_cache_volatile[] = {
	ADXL313_REG_SOFT_RESET,  /* 0x18 - keep ordered */
	ADXL313_REG_INT_SOURCE,  /* 0x30 */
	ADXL313_REG_DATA_X0_REG, /* 0x32 */
	ADXL313_REG_DATA_X1_REG, /* 0x33 */
	ADXL313_REG_DATA_Y0_REG, /* 0x34 */
	ADXL313_REG_DATA_Y1_REG, /* 0x35 */
	ADXL313_REG_DATA_Z0_REG, /* 0x36 */
	ADXL313_REG_DATA_Z1_REG, /* 0x37 */
	ADXL313_REG_FIFO_STATUS, /* 0x39 */
};
#define cache_idx(reg) ((reg) - ADXL313_CACHE_START)

enum adxl313_cache_res {
	ADXL313_CACHE_HIT = 0,
	ADXL313_NOT_CACHED,
	ADXL313_CACHE_INVAL,
};

static enum adxl313_cache_res adxl313_cache_get(struct adxl313_dev_data *data, uint8_t reg,
						uint8_t *val)
{
	const uint8_t reg_idx = cache_idx(reg);
	int i;

	if (reg < ADXL313_CACHE_START || reg > ADXL313_CACHE_END) {
		return ADXL313_CACHE_INVAL;
	}

	for (i = 0; i < ARRAY_SIZE(adxl313_cache_volatile); i++) {
		if (reg == adxl313_cache_volatile[i]) {
			return ADXL313_NOT_CACHED;
		}
	}

	*val = data->reg_cache[reg_idx];

	return ADXL313_CACHE_HIT;
}

static enum adxl313_cache_res adxl313_cache_put(struct adxl313_dev_data *data, uint8_t reg,
						uint8_t val)
{
	const uint8_t reg_idx = cache_idx(reg);
	int i;

	if (reg < ADXL313_CACHE_START || reg > ADXL313_CACHE_END) {
		return ADXL313_CACHE_INVAL;
	}

	for (i = 0; i < ARRAY_SIZE(adxl313_cache_volatile); i++) {
		if (reg == adxl313_cache_volatile[i]) {
			return ADXL313_NOT_CACHED;
		}
	}

	data->reg_cache[reg_idx] = val;

	return ADXL313_CACHE_HIT;
}

int adxl313_reg_read_byte(const struct device *dev, uint8_t addr, uint8_t *buf)
{
	if (adxl313_cache_get(dev->data, addr, buf) != ADXL313_CACHE_HIT) {
		return adxl313_raw_reg_read(dev, addr, buf, 1);
	}

	return 0;
}

int adxl313_reg_write_byte(const struct device *dev, uint8_t addr, uint8_t val)
{
	adxl313_cache_put(dev->data, addr, val);

	return adxl313_reg_write(dev, addr, &val, 1);
}

int adxl313_reg_write_mask(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t data)
{
	uint8_t regval, tmp;
	int ret;

	ret = adxl313_reg_read_byte(dev, reg, &regval);
	if (ret) {
		return ret;
	}

	tmp = regval & ~mask;
	tmp |= data & mask;

	return adxl313_reg_write_byte(dev, reg, tmp);
}

int adxl313_reg_assign_bits(const struct device *dev, uint8_t reg, uint8_t mask, bool en)
{
	return adxl313_reg_write_mask(dev, reg, mask, en ? mask : 0x00);
}

static inline bool adxl313_bus_is_ready(const struct device *dev)
{
	const struct adxl313_dev_config *cfg = dev->config;

	return cfg->bus_is_ready(&cfg->bus);
}

bool adxl313_is_measure_en(const struct device *dev)
{
	uint8_t regval;
	int ret;

	ret = adxl313_reg_read_byte(dev, ADXL313_REG_POWER_CTL, &regval);
	if (ret) {
		return false;
	}

	return !!(FIELD_GET(ADXL313_POWER_CTL_MEASURE, regval));
}

int adxl313_set_measure_en(const struct device *dev, bool en)
{
	return adxl313_reg_assign_bits(dev, ADXL313_REG_POWER_CTL, ADXL313_POWER_CTL_MEASURE, en);
}

int adxl313_get_fifo_entries(const struct device *dev)
{
	uint8_t regval;
	int ret;

	ret = adxl313_reg_read_byte(dev, ADXL313_REG_FIFO_STATUS, &regval);
	if (ret) {
		return ret;
	}

	return FIELD_GET(ADXL313_FIFO_STATUS_ENTRIES_MSK, regval);
}

int adxl313_get_status(const struct device *dev, uint8_t *status)
{
	return adxl313_reg_read_byte(dev, ADXL313_REG_INT_SOURCE, status);
}

int adxl313_flush_fifo(const struct device *dev)
{
#if defined(CONFIG_ADXL313_TRIGGER)
	/*
	 * Simply turning off (BYPASSED) and then back on (e.g., set to STREAM)
	 * the FIFO does not fully reset its status and control registers, as
	 * mentioned in the datasheet. This behavior can be observed by enabling
	 * CONFIG_ADXL313_TRIGGER and creating an application that registers a
	 * handler for watermark or overrun events. Simply toggling the FIFO
	 * state will never trigger these events. Therefore, it is necessary to
	 * ensure that the FIFO is read completely empty.
	 */
	uint8_t sample_line[ADXL313_FIFO_SAMPLE_SIZE];
	int rc;

	rc = adxl313_set_measure_en(dev, false);
	if (rc) {
		return rc;
	}

	while (true) {
		int8_t fifo_entries = adxl313_get_fifo_entries(dev);

		if (fifo_entries < 0) {
			return fifo_entries;
		}

		if (fifo_entries == 0) {
			break;
		}

		fifo_entries += 1;
		while (fifo_entries > 0) { /* Read FIFO entries + 1 sample lines */
			rc = adxl313_raw_reg_read(dev, ADXL313_REG_DATA_XYZ_REGS, sample_line,
						  ADXL313_FIFO_SAMPLE_SIZE);
			if (rc) {
				return rc;
			}

			fifo_entries--;
		}
	}
#endif /* CONFIG_ADXL313_TRIGGER */

	return adxl313_set_measure_en(dev, true);
}

/**
 * adxl313_configure_fifo - Configure the FIFO operating parameters.
 *
 * This function sets the FIFO mode and watermark level used to control FIFO
 * behavior. Depending on the selected mode, the FIFO may be bypassed or operated
 * in a streamed configuration. When the FIFO is bypassed, the watermark level
 * is reset accordingly.
 *
 * @param dev Pointer to the device structure.
 * @param mode FIFO operating mode. Typically either
 *             ADXL313_FIFO_BYPASSED or ADXL313_FIFO_STREAMED.
 * @param fifo_samples FIFO watermark level, expressed as the number of samples
 *                     required to trigger a FIFO_FULL condition. Valid values
 *                     range from 0 to 32; selecting ADXL313_FIFO_BYPASSED
 *                     implicitly resets this value to 0.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int adxl313_configure_fifo(const struct device *dev, enum adxl313_fifo_mode mode,
			   uint8_t fifo_samples)
{
	struct adxl313_dev_data *data = dev->data;
	uint8_t fifo_config = adxl313_fifo_ctl_mode_init[mode];

	data->fifo_config.fifo_mode = mode;

	/* Although rather cosmetic, clear config in case of BYPASSED. */
	if (mode == ADXL313_FIFO_BYPASSED) {
		fifo_samples = 0;
	}

	data->fifo_config.fifo_samples = MIN(fifo_samples, ADXL313_FIFO_MAX_SIZE);
	fifo_config |= data->fifo_config.fifo_samples;

	return adxl313_reg_write_byte(dev, ADXL313_REG_FIFO_CTL, fifo_config);
}

static int adxl313_attr_set_odr(const struct device *dev, const struct sensor_value *val)
{
	enum adxl313_odr odr;
	struct adxl313_dev_data *data = dev->data;

	switch (val->val1) {
	case 6:
		odr = ADXL313_ODR_6_25HZ;
		break;
	case 12:
		odr = ADXL313_ODR_12_5HZ;
		break;
	case 25:
		odr = ADXL313_ODR_25HZ;
		break;
	case 50:
		odr = ADXL313_ODR_50HZ;
		break;
	case 100:
		odr = ADXL313_ODR_100HZ;
		break;
	case 200:
		odr = ADXL313_ODR_200HZ;
		break;
	case 400:
		odr = ADXL313_ODR_400HZ;
		break;
	case 800:
		odr = ADXL313_ODR_800HZ;
		break;
	case 1600:
		odr = ADXL313_ODR_1600HZ;
		break;
	case 3200:
		odr = ADXL313_ODR_3200HZ;
		break;
	default:
		return -EINVAL;
	}

	data->odr = odr;

	return adxl313_reg_write_mask(dev, ADXL313_REG_RATE, ADXL313_RATE_ODR_MSK, odr);
}

/**
 * adxl313_attr_set - Set sensor attributes from an application.
 *
 * This function allows the application to configure key features of the ADXL313
 * sensor by setting the appropriate attributes. The available attributes and
 * their effects are:
 *
 * - SENSOR_ATTR_SAMPLING_FREQUENCY: Configures the output data rate (ODR) of
 *	the sensor.
 *
 * @param dev Pointer to the device structure.
 * @param chan The sensor channel (currently unused, reserved for future expansion).
 * @param attr The attribute to configure (one of the SENSOR_ATTR_* options above).
 * @param val Pointer to a sensor_value containing the desired value for the attribute.
 *
 * @return 0 on success, or a negative error code if the attribute is unsupported
 *         or the configuration fails.
 */
static int adxl313_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY: /* ODR */
		return adxl313_attr_set_odr(dev, val);
	default:
		return -ENOTSUP;
	}
}

int adxl313_read_sample(const struct device *dev, struct adxl313_xyz_accel_data *sample)
{
	uint8_t axis_data[ADXL313_FIFO_SAMPLE_SIZE];
	int rc;

	rc = adxl313_raw_reg_read(dev, ADXL313_REG_DATA_XYZ_REGS, axis_data,
				  ADXL313_FIFO_SAMPLE_SIZE);
	if (rc < 0) {
		LOG_ERR("Samples read failed with rc=%d", rc);
		return rc;
	}

	sample->x = axis_data[0] | axis_data[1] << 8;
	sample->y = axis_data[2] | axis_data[3] << 8;
	sample->z = axis_data[4] | axis_data[5] << 8;

#ifdef CONFIG_ADXL313_TRIGGER
	struct adxl313_dev_data *data = dev->data;

	sample->is_full_res = data->is_full_res; /* needed for decoder */
	sample->selected_range = data->selected_range;
#endif

	return rc;
}

/**
 * adxl313_accel_convert - Convert a raw acceleration sample to a sensor_value.
 *
 * This function provides a fallback conversion path for raw acceleration
 * samples when no decoder is available, i.e. when neither TRIGGER nor STREAM
 * modes are enabled.
 *
 * The conversion assumes the device is operating in full-resolution mode
 * unless specified otherwise. The resulting acceleration value is written
 * to the provided @ref sensor_value structure, including the fractional
 * component.
 *
 * @param[out] out Pointer to the sensor_value structure to be populated.
 * @param sample  Raw acceleration sample read from the sensor.
 * @param range   Configured acceleration range of the device.
 * @param is_full_res Indicates whether full-resolution mode is enabled.
 */
static void adxl313_accel_convert(struct sensor_value *out, int16_t sample,
				  enum adxl313_range range, bool is_full_res)
{
	q31_t tmp;
	int64_t accel;
	uint8_t shift;

	adxl313_accel_convert_q31(&tmp, sample, range, is_full_res);

	/* Resolve effective resolution shift */
	if (is_full_res) {
		shift = range_to_shift[range];
	} else {
		/* No full-res: always 10-bit, 256 LSB/g */
		shift = range_to_shift[ADXL313_RANGE_0_5G];
	}

	accel = (int64_t)tmp * SENSOR_G / 10;
	accel >>= (31 - shift);

	out->val1 = accel / 1000000;
	out->val2 = accel % 1000000;
}

/**
 * adxl313_sample_fetch - Fetch raw sensor samples.
 *
 * Fetches a set of raw (unscaled) sensor samples. Each set consists of three
 * axis values read in a burst. If FIFO buffering is enabled, the function
 * loops to read up to the number of available FIFO entries. Samples are stored
 * internally within the driver's data object.
 *
 * @param dev The device structure.
 * @param chan The sensor channel.
 *
 * @returns 0 for success, else a negative error code.
 */
static int adxl313_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct adxl313_dev_data *data = dev->data;
	uint8_t count;
	int rc;

	count = 1;

	/* FIFO BYPASSED is the only mode not using a FIFO buffer */
	if (data->fifo_config.fifo_mode != ADXL313_FIFO_BYPASSED) {
		count = adxl313_get_fifo_entries(dev);
		if (count < 0) {
			return -EIO;
		}
	}

	__ASSERT_NO_MSG(count <= ARRAY_SIZE(data->sample));

	for (uint8_t s = 0; s < count; s++) {
		rc = adxl313_read_sample(dev, &data->sample[s]);
		if (rc < 0) {
			LOG_ERR("Failed to fetch sample rc=%d", rc);
			return rc;
		}
	}

	/* new sample available, reset book-keeping */
	data->sample_idx = 0;
	data->fifo_entries = count;

	return 0;
}

/**
 * adxl313_channel_get - Read a single element of one or three axis.
 *
 * @param dev The sensor device.
 * @param chan The axis channel, can be x, y, z or xyz.
 * @param val The resulting value after conversion of the raw axis data. Val can
 *	be a single value or an array of three values, where the index
 *	correspondes to x, y, z axis entries.
 *
 * @returns 0 for success, else a negative error value.
 */
static int adxl313_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adxl313_dev_data *data = dev->data;
	int idx;

	if (data->fifo_entries < 0) {
		return -ENOTSUP;
	}

	if (data->fifo_entries == 0) { /* empty */
		val->val1 = 0;
		val->val2 = 0;
		if (chan == SENSOR_CHAN_ACCEL_XYZ) {
			val[1].val1 = 0;
			val[1].val2 = 0;
			val[2].val1 = 0;
			val[2].val2 = 0;
		}

		return -ENODATA;
	}

	data->sample_idx = data->sample_idx % data->fifo_entries;
	idx = data->sample_idx;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		adxl313_accel_convert(val, data->sample[idx].x, data->selected_range,
				      data->is_full_res);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl313_accel_convert(val, data->sample[idx].y, data->selected_range,
				      data->is_full_res);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl313_accel_convert(val, data->sample[idx].z, data->selected_range,
				      data->is_full_res);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl313_accel_convert(val++, data->sample[idx].x, data->selected_range,
				      data->is_full_res);
		adxl313_accel_convert(val++, data->sample[idx].y, data->selected_range,
				      data->is_full_res);
		adxl313_accel_convert(val, data->sample[idx].z, data->selected_range,
				      data->is_full_res);
		break;
	default:
		return -ENOTSUP;
	}

	data->sample_idx++;

	return 0;
}

static DEVICE_API(sensor, adxl313_api_funcs) = {
	.attr_set = adxl313_attr_set,
	.sample_fetch = adxl313_sample_fetch,
	.channel_get = adxl313_channel_get,
#ifdef CONFIG_ADXL313_TRIGGER
	.trigger_set = adxl313_trigger_set,
#endif
#ifdef CONFIG_SENSOR_ASYNC_API
	.get_decoder = adxl313_get_decoder,
#endif
};

static int adxl313_init(const struct device *dev)
{
	struct adxl313_dev_data *data = dev->data;
	const struct adxl313_dev_config *cfg = dev->config;
	enum adxl313_fifo_mode fifo_mode;
	uint8_t dev_id;
	uint8_t int_en;
	uint8_t fifo_samples;
	uint8_t power_ctl;
	uint8_t regval;
	int rc;

	if (!adxl313_bus_is_ready(dev)) {
		LOG_ERR("bus not ready");
		return -ENODEV;
	}

	rc = adxl313_raw_reg_read(dev, ADXL313_REG_DEVID0, &dev_id, 1);
	if (rc < 0 || dev_id != ADXL313_EXPECTED_DEVID0) {
		LOG_ERR("Read PART ID failed: 0x%x", rc);
		return -ENODEV;
	}

	rc = adxl313_raw_reg_read(dev, ADXL313_REG_DEVID1, &dev_id, 1);
	if (rc < 0 || dev_id != ADXL313_EXPECTED_DEVID1) {
		LOG_ERR("Read PART ID failed: 0x%x", rc);
		return -ENODEV;
	}

	/* Preset used config registers. */

	power_ctl = 0x00;
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	power_ctl |= ADXL313_POWER_CTL_I2C_DISABLE;
#endif
	rc = adxl313_reg_write_byte(dev, ADXL313_REG_POWER_CTL, power_ctl);
	if (rc) {
		return rc;
	}

	rc = adxl313_reg_write_byte(dev, ADXL313_REG_INT_ENABLE, 0x00);
	if (rc) {
		return rc;
	}

	rc = adxl313_reg_write_byte(dev, ADXL313_REG_INT_MAP, 0x00);
	if (rc) {
		return rc;
	}

	rc = adxl313_reg_write_byte(dev, ADXL313_REG_RATE, 0x00);
	if (rc) {
		return rc;
	}

	rc = adxl313_reg_write_byte(dev, ADXL313_REG_FIFO_CTL, 0x00);
	if (rc) {
		return rc;
	}

	/* initial setting */
	data->selected_range = cfg->selected_range;
	data->odr = cfg->odr;
	data->is_full_res = true;

	/*
	 * Reset the following sensor fields (in case of warm starts)
	 * - turn off measurements
	 * - configure full resolution accordingly
	 * - turn off interrupt inversion
	 * - turn off 3-wire SPI
	 * - turn off self test mode
	 */
	regval = (data->is_full_res ? ADXL313_DATA_FORMAT_FULL_RES : 0x00);
	regval |= adxl313_range_init[data->selected_range];
	rc = adxl313_reg_write_byte(dev, ADXL313_REG_DATA_FORMAT, regval);
	if (rc < 0) {
		LOG_ERR("Data format set failed");
		return -EIO;
	}

	rc = adxl313_reg_write_mask(dev, ADXL313_REG_RATE, ADXL313_RATE_ODR_MSK, data->odr);
	if (rc) {
		LOG_ERR("Rate setting failed");
		return rc;
	}

	fifo_mode = ADXL313_FIFO_BYPASSED;
	fifo_samples = 0;
	int_en = 0x00;
#if defined(CONFIG_ADXL313_TRIGGER)
	if (adxl313_init_interrupt(dev)) {
		LOG_DBG("No IRQ lines specified, fallback to FIFO BYPASSED");
		fifo_mode = ADXL313_FIFO_BYPASSED;
	} else {
		/*
		 * Note, zephyr STREAMING or TRIGGER enabled, at least one
		 * configured interrupt line is required for all sensor events.
		 * FIFO buffering is optional (wm == 0), other events do not
		 * depend on FIFO buffering. In such case FIFO is running on
		 * FIFO_BYPASSED.
		 */
		fifo_samples = cfg->fifo_samples;
		if (fifo_samples) {
			fifo_mode = ADXL313_FIFO_STREAMED;

			/*
			 * Currently, map all interrupts to the (same) gpio line
			 * configured in the device tree. This is usually sufficient,
			 * also since not every hardware will have both gpio lines
			 * soldered. Anyway, for individual interrupt mapping, set up
			 * DTB bindings.
			 */
			rc = adxl313_reg_assign_bits(dev, ADXL313_REG_INT_MAP, UCHAR_MAX,
						     (cfg->drdy_pad == 2));
			if (rc) {
				return rc;
			}
		}
	}
#endif
	rc = adxl313_configure_fifo(dev, fifo_mode, fifo_samples);
	if (rc) {
		return rc;
	}

	if (fifo_mode == ADXL313_FIFO_BYPASSED) {
		return adxl313_set_measure_en(dev, true);
	}

	return 0;
}

#ifdef CONFIG_ADXL313_TRIGGER
#define ADXL313_CFG_IRQ(inst)                                                                      \
	.gpio_int1 = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0}),                              \
	.gpio_int2 = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, {0}),                              \
	.drdy_pad = DT_INST_PROP_OR(inst, drdy_pin, -1),                                           \
	.fifo_samples = DT_INST_PROP_OR(inst, fifo_watermark, 0),
#else
#define ADXL313_CFG_IRQ(inst)
#endif /* CONFIG_ADXL313_TRIGGER */

#define ADXL313_CONFIG_COMMON(inst)                                                                \
	IF_ENABLED(UTIL_OR(DT_INST_NODE_HAS_PROP(inst, int1_gpios),     \
			   DT_INST_NODE_HAS_PROP(inst, int2_gpios)),    \
			   (ADXL313_CFG_IRQ(inst)))

#define ADXL313_RTIO_SPI_DEFINE(inst)                                                              \
	COND_CODE_1(CONFIG_SPI_RTIO,                                                  \
			(SPI_DT_IODEV_DEFINE(adxl313_iodev_##inst, DT_DRV_INST(inst), \
			SPI_WORD_SET(8) | SPI_TRANSFER_MSB |            \
			SPI_MODE_CPOL | SPI_MODE_CPHA);),               \
			())

#define ADXL313_RTIO_I2C_DEFINE(inst)                                                              \
	COND_CODE_1(CONFIG_I2C_RTIO,                                                               \
			(I2C_DT_IODEV_DEFINE(adxl313_iodev_##inst, DT_DRV_INST(inst));), \
			())

/*
 * RTIO SQE/CQE pool size depends on the fifo-watermark because we
 * can't just burst-read all the fifo data at once. Datasheet specifies
 * we need to get one frame at a time (through the Data registers),
 * therefore, we set all the sequence at once to properly pull each
 * frame, and then end up calling the completion event so the
 * application receives it).
 */
#define ADXL313_RTIO_DEFINE(inst)                                                                  \
	/* Conditionally include SPI and/or I2C parts based on their presence */                   \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),                          \
				(ADXL313_RTIO_SPI_DEFINE(inst)),        \
				())                          \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c),                          \
				(ADXL313_RTIO_I2C_DEFINE(inst)),        \
				())                          \
	RTIO_DEFINE(adxl313_rtio_ctx_##inst, 4 * ADXL313_FIFO_MAX_SIZE, 4 * ADXL313_FIFO_MAX_SIZE);

#define ADXL313_CONFIG(inst)                                                                       \
	.odr = DT_INST_PROP(inst, odr), .selected_range = DT_INST_PROP(inst, range),

#define ADXL313_CONFIG_SPI(inst)                                                                   \
	{.bus = {.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) | SPI_TRANSFER_MSB |            \
							   SPI_MODE_CPOL | SPI_MODE_CPHA)},        \
	 .bus_is_ready = adxl313_bus_is_ready_spi,                                                 \
	 .reg_access = adxl313_reg_access_spi,                                                     \
	 .bus_type = ADXL313_BUS_SPI,                                                              \
	 ADXL313_CONFIG(inst) COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int1_gpios),                 \
				(ADXL313_CONFIG_COMMON(inst)), ()) \
			 COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int2_gpios),                      \
				(ADXL313_CONFIG_COMMON(inst)), ()) }

#define ADXL313_CONFIG_I2C(inst)                                                                   \
	{.bus = {.i2c = I2C_DT_SPEC_INST_GET(inst)},                                               \
	 .bus_is_ready = adxl313_bus_is_ready_i2c,                                                 \
	 .reg_access = adxl313_reg_access_i2c,                                                     \
	 .bus_type = ADXL313_BUS_I2C,                                                              \
	 ADXL313_CONFIG(inst) COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int1_gpios),                 \
				(ADXL313_CONFIG_COMMON(inst)), ()) \
			 COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int2_gpios),                      \
				(ADXL313_CONFIG_COMMON(inst)), ()) }

#define ADXL313_DEFINE(inst)                                                                       \
	BUILD_ASSERT(COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, fifo_watermark),                      \
				 ((DT_INST_PROP(inst, fifo_watermark) < 32)),                      \
				 (true)),                                                          \
		 "Invalid fifo-watermark setting, consult dts/bindings "                           \
		 "for valid ranges.");                                                             \
                                                                                                   \
	static struct adxl313_dev_data adxl313_data_##inst = {};                                   \
                                                                                                   \
	static const struct adxl313_dev_config adxl313_config_##inst =                             \
			    COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                 \
			    (ADXL313_CONFIG_SPI(inst)),                                            \
			    (ADXL313_CONFIG_I2C(inst)));                                           \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adxl313_init, NULL, &adxl313_data_##inst,               \
				     &adxl313_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &adxl313_api_funcs);             \
                                                                                                   \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, drdy_pin),                 \
		(BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst,                 \
			CONCAT(int, DT_INST_PROP(inst, drdy_pin), _gpios)),                        \
			"No GPIO pin defined for ADXL313 DRDY interrupt");))

DT_INST_FOREACH_STATUS_OKAY(ADXL313_DEFINE)
