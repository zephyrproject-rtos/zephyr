/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_fxos8700

#include "fxos8700.h"
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(FXOS8700, CONFIG_SENSOR_LOG_LEVEL);

/* Convert the range (8g, 4g, 2g) to the encoded FS register field value */
#define RANGE2FS(x) (__builtin_ctz(x) - 1)

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define DIR_READ(a)  ((a) & 0x7f)
#define DIR_WRITE(a) ((a) | BIT(7))
#define ADDR_7(a) ((a) & BIT(7))

int fxos8700_transceive(const struct device *dev,
				void *data, size_t length)
{
	const struct fxos8700_config *cfg = dev->config;
	const struct spi_buf buf = { .buf = data, .len = length };
	const struct spi_buf_set s = { .buffers = &buf, .count = 1 };

	return spi_transceive_dt(&cfg->bus_cfg.spi, &s, &s);
}

int fxos8700_read_spi(const struct device *dev,
		      uint8_t reg,
		      void *data,
		      size_t length)
{
	const struct fxos8700_config *cfg = dev->config;

	/* Reads must clock out a dummy byte after sending the address. */
	uint8_t reg_buf[3] = { DIR_READ(reg), ADDR_7(reg), 0 };
	const struct spi_buf buf[2] = {
		{ .buf = reg_buf, .len = 3 },
		{ .buf = data, .len = length }
	};
	const struct spi_buf_set tx = { .buffers = buf, .count = 1 };
	const struct spi_buf_set rx = { .buffers = buf, .count = 2 };

	return spi_transceive_dt(&cfg->bus_cfg.spi, &tx, &rx);
}

int fxos8700_byte_read_spi(const struct device *dev,
			   uint8_t reg,
			   uint8_t *byte)
{
	/* Reads must clock out a dummy byte after sending the address. */
	uint8_t data[] = { DIR_READ(reg), ADDR_7(reg), 0};
	int ret;

	ret = fxos8700_transceive(dev, data, sizeof(data));

	*byte = data[2];

	return ret;
}

int fxos8700_byte_write_spi(const struct device *dev,
			    uint8_t reg,
			    uint8_t byte)
{
	uint8_t data[] = { DIR_WRITE(reg), ADDR_7(reg), byte };

	return fxos8700_transceive(dev, data, sizeof(data));
}

int fxos8700_reg_field_update_spi(const struct device *dev,
				  uint8_t reg,
				  uint8_t mask,
				  uint8_t val)
{
	uint8_t old_val;

	if (fxos8700_byte_read_spi(dev, reg, &old_val) < 0) {
		return -EIO;
	}

	return fxos8700_byte_write_spi(dev, reg, (old_val & ~mask) | (val & mask));
}

static const struct fxos8700_io_ops fxos8700_spi_ops = {
	.read = fxos8700_read_spi,
	.byte_read = fxos8700_byte_read_spi,
	.byte_write = fxos8700_byte_write_spi,
	.reg_field_update = fxos8700_reg_field_update_spi,
};
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
int fxos8700_read_i2c(const struct device *dev,
		      uint8_t reg,
		      void *data,
		      size_t length)
{
	const struct fxos8700_config *config = dev->config;

	return i2c_burst_read_dt(&config->bus_cfg.i2c, reg, data, length);
}

int fxos8700_byte_read_i2c(const struct device *dev,
			   uint8_t reg,
			   uint8_t *byte)
{
	const struct fxos8700_config *config = dev->config;

	return i2c_reg_read_byte_dt(&config->bus_cfg.i2c, reg, byte);
}

int fxos8700_byte_write_i2c(const struct device *dev,
			    uint8_t reg,
			    uint8_t byte)
{
	const struct fxos8700_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->bus_cfg.i2c, reg, byte);
}

int fxos8700_reg_field_update_i2c(const struct device *dev,
				  uint8_t reg,
				  uint8_t mask,
				  uint8_t val)
{
	const struct fxos8700_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->bus_cfg.i2c, reg, mask, val);
}
static const struct fxos8700_io_ops fxos8700_i2c_ops = {
	.read = fxos8700_read_i2c,
	.byte_read = fxos8700_byte_read_i2c,
	.byte_write = fxos8700_byte_write_i2c,
	.reg_field_update = fxos8700_reg_field_update_i2c,
};
#endif

static int fxos8700_set_odr(const struct device *dev,
		const struct sensor_value *val)
{
	const struct fxos8700_config *config = dev->config;
	uint8_t dr;
	enum fxos8700_power power;

#ifdef CONFIG_FXOS8700_MODE_HYBRID
	/* ODR is halved in hybrid mode */
	if (val->val1 == 400 && val->val2 == 0) {
		dr = FXOS8700_CTRLREG1_DR_RATE_800;
	} else if (val->val1 == 200 && val->val2 == 0) {
		dr = FXOS8700_CTRLREG1_DR_RATE_400;
	} else if (val->val1 == 100 && val->val2 == 0) {
		dr = FXOS8700_CTRLREG1_DR_RATE_200;
	} else if (val->val1 == 50 && val->val2 == 0) {
		dr = FXOS8700_CTRLREG1_DR_RATE_100;
	} else if (val->val1 == 25 && val->val2 == 0) {
		dr = FXOS8700_CTRLREG1_DR_RATE_50;
	} else if (val->val1 == 6 && val->val2 == 250000) {
		dr = FXOS8700_CTRLREG1_DR_RATE_12_5;
	} else if (val->val1 == 3 && val->val2 == 125000) {
		dr = FXOS8700_CTRLREG1_DR_RATE_6_25;
	} else if (val->val1 == 0 && val->val2 == 781300) {
		dr = FXOS8700_CTRLREG1_DR_RATE_1_56;
	} else {
		return -EINVAL;
	}
#else
	if (val->val1 == 800 && val->val2 == 0) {
		dr = FXOS8700_CTRLREG1_DR_RATE_800;
	} else if (val->val1 == 400 && val->val2 == 0) {
		dr = FXOS8700_CTRLREG1_DR_RATE_400;
	} else if (val->val1 == 200 && val->val2 == 0) {
		dr = FXOS8700_CTRLREG1_DR_RATE_200;
	} else if (val->val1 == 100 && val->val2 == 0) {
		dr = FXOS8700_CTRLREG1_DR_RATE_100;
	} else if (val->val1 == 50 && val->val2 == 0) {
		dr = FXOS8700_CTRLREG1_DR_RATE_50;
	} else if (val->val1 == 12 && val->val2 == 500000) {
		dr = FXOS8700_CTRLREG1_DR_RATE_12_5;
	} else if (val->val1 == 6 && val->val2 == 250000) {
		dr = FXOS8700_CTRLREG1_DR_RATE_6_25;
	} else if (val->val1 == 1 && val->val2 == 562500) {
		dr = FXOS8700_CTRLREG1_DR_RATE_1_56;
	} else {
		return -EINVAL;
	}
#endif

	LOG_DBG("Set ODR to 0x%x", dr);

	/*
	 * Modify FXOS8700_REG_CTRLREG1 can only occur when the device
	 * is in standby mode. Get the current power mode to restore it later.
	 */
	if (fxos8700_get_power(dev, &power)) {
		LOG_ERR("Could not get power mode");
		return -EIO;
	}

	/* Set standby power mode */
	if (fxos8700_set_power(dev, FXOS8700_POWER_STANDBY)) {
		LOG_ERR("Could not set standby");
		return -EIO;
	}

	/* Change the attribute and restore power mode. */
	return config->ops->reg_field_update(dev, FXOS8700_REG_CTRLREG1,
				      FXOS8700_CTRLREG1_DR_MASK | FXOS8700_CTRLREG1_ACTIVE_MASK,
				      dr | power);
}

static int fxos8700_set_mt_ths(const struct device *dev,
			       const struct sensor_value *val)
{
#ifdef CONFIG_FXOS8700_MOTION
	const struct fxos8700_config *config = dev->config;
	uint64_t micro_ms2 = abs(val->val1 * 1000000LL + val->val2);
	uint64_t ths = micro_ms2 / FXOS8700_FF_MT_THS_SCALE;

	if (ths > FXOS8700_FF_MT_THS_MASK) {
		LOG_ERR("Threshold value is out of range");
		return -EINVAL;
	}

	LOG_DBG("Set FF_MT_THS to %d", (uint8_t)ths);

	return config->ops->reg_field_update(dev, FXOS8700_REG_FF_MT_THS,
				      FXOS8700_FF_MT_THS_MASK, (uint8_t)ths);
#else
	return -ENOTSUP;
#endif
}

static int fxos8700_attr_set(const struct device *dev,
			     enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		return fxos8700_set_odr(dev, val);
	} else if (attr == SENSOR_ATTR_SLOPE_TH) {
		return fxos8700_set_mt_ths(dev, val);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int fxos8700_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;
	uint8_t buffer[FXOS8700_MAX_NUM_BYTES];
	uint8_t num_bytes;
	int16_t *raw;
	int ret = 0;
	int i;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	k_sem_take(&data->sem, K_FOREVER);

	/* Read all the channels in one I2C/SPI transaction. The number of bytes to
	 * read and the starting register address depend on the mode
	 * configuration (accel-only, mag-only, or hybrid).
	 */
	num_bytes = config->num_channels * FXOS8700_BYTES_PER_CHANNEL_NORMAL;

	__ASSERT(num_bytes <= sizeof(buffer), "Too many bytes to read");

	if (config->ops->read(dev, config->start_addr, buffer, num_bytes)) {
		LOG_ERR("Could not fetch sample");
		ret = -EIO;
		goto exit;
	}

	/* Parse the buffer into raw channel data (16-bit integers). To save
	 * RAM, store the data in raw format and wait to convert to the
	 * normalized sensor_value type until later.
	 */
	__ASSERT(config->start_channel + config->num_channels
			<= ARRAY_SIZE(data->raw),
			"Too many channels");

	raw = &data->raw[config->start_channel];

	for (i = 0; i < num_bytes; i += 2) {
		*raw++ = (buffer[i] << 8) | (buffer[i+1]);
	}

#ifdef CONFIG_FXOS8700_TEMP
	if (config->ops->byte_read(dev, FXOS8700_REG_TEMP,
				 &data->temp)) {
		LOG_ERR("Could not fetch temperature");
		ret = -EIO;
		goto exit;
	}
#endif

exit:
	k_sem_give(&data->sem);

	return ret;
}

static void fxos8700_accel_convert(struct sensor_value *val, int16_t raw,
				   uint8_t range)
{
	uint8_t frac_bits;
	int64_t micro_ms2;

	/* The range encoding is convenient to compute the number of fractional
	 * bits:
	 * - 2g mode (fs = 0) has 14 fractional bits
	 * - 4g mode (fs = 1) has 13 fractional bits
	 * - 8g mode (fs = 2) has 12 fractional bits
	 */
	frac_bits = 14 - RANGE2FS(range);

	/* Convert units to micro m/s^2. Intermediate results before the shift
	 * are 40 bits wide.
	 */
	micro_ms2 = (raw * SENSOR_G) >> frac_bits;

	/* The maximum possible value is 8g, which in units of micro m/s^2
	 * always fits into 32-bits. Cast down to int32_t so we can use a
	 * faster divide.
	 */
	val->val1 = (int32_t) micro_ms2 / 1000000;
	val->val2 = (int32_t) micro_ms2 % 1000000;
}

static void fxos8700_magn_convert(struct sensor_value *val, int16_t raw)
{
	int32_t micro_g;

	/* Convert units to micro Gauss. Raw magnetic data always has a
	 * resolution of 0.1 uT/LSB, which is equivalent to 0.001 G/LSB.
	 */
	micro_g = raw * 1000;

	val->val1 = micro_g / 1000000;
	val->val2 = micro_g % 1000000;
}

#ifdef CONFIG_FXOS8700_TEMP
static void fxos8700_temp_convert(struct sensor_value *val, int8_t raw)
{
	int32_t micro_c;

	/* Convert units to micro Celsius. Raw temperature data always has a
	 * resolution of 0.96 deg C/LSB.
	 */
	micro_c = raw * 960 * 1000;

	val->val1 = micro_c / 1000000;
	val->val2 = micro_c % 1000000;
}
#endif

static int fxos8700_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;
	int start_channel;
	int num_channels;
	int16_t *raw;
	int ret;
	int i;

	k_sem_take(&data->sem, K_FOREVER);

	/* Start with an error return code by default, then clear it if we find
	 * a supported sensor channel.
	 */
	ret = -ENOTSUP;

	/* If we're in an accelerometer-enabled mode (accel-only or hybrid),
	 * then convert raw accelerometer data to the normalized sensor_value
	 * type.
	 */
	if (config->mode != FXOS8700_MODE_MAGN) {
		switch (chan) {
		case SENSOR_CHAN_ACCEL_X:
			start_channel = FXOS8700_CHANNEL_ACCEL_X;
			num_channels = 1;
			break;
		case SENSOR_CHAN_ACCEL_Y:
			start_channel = FXOS8700_CHANNEL_ACCEL_Y;
			num_channels = 1;
			break;
		case SENSOR_CHAN_ACCEL_Z:
			start_channel = FXOS8700_CHANNEL_ACCEL_Z;
			num_channels = 1;
			break;
		case SENSOR_CHAN_ACCEL_XYZ:
			start_channel = FXOS8700_CHANNEL_ACCEL_X;
			num_channels = 3;
			break;
		default:
			start_channel = 0;
			num_channels = 0;
			break;
		}

		raw = &data->raw[start_channel];
		for (i = 0; i < num_channels; i++) {
			fxos8700_accel_convert(val++, *raw++, config->range);
		}

		if (num_channels > 0) {
			ret = 0;
		}
	}

	/* If we're in an magnetometer-enabled mode (mag-only or hybrid), then
	 * convert raw magnetometer data to the normalized sensor_value type.
	 */
	if (config->mode != FXOS8700_MODE_ACCEL) {
		switch (chan) {
		case SENSOR_CHAN_MAGN_X:
			start_channel = FXOS8700_CHANNEL_MAGN_X;
			num_channels = 1;
			break;
		case SENSOR_CHAN_MAGN_Y:
			start_channel = FXOS8700_CHANNEL_MAGN_Y;
			num_channels = 1;
			break;
		case SENSOR_CHAN_MAGN_Z:
			start_channel = FXOS8700_CHANNEL_MAGN_Z;
			num_channels = 1;
			break;
		case SENSOR_CHAN_MAGN_XYZ:
			start_channel = FXOS8700_CHANNEL_MAGN_X;
			num_channels = 3;
			break;
		default:
			start_channel = 0;
			num_channels = 0;
			break;
		}

		raw = &data->raw[start_channel];
		for (i = 0; i < num_channels; i++) {
			fxos8700_magn_convert(val++, *raw++);
		}

		if (num_channels > 0) {
			ret = 0;
		}
#ifdef CONFIG_FXOS8700_TEMP
		if (chan == SENSOR_CHAN_DIE_TEMP) {
			fxos8700_temp_convert(val, data->temp);
			ret = 0;
		}
#endif
	}

	if (ret != 0) {
		LOG_ERR("Unsupported sensor channel");
	}

	k_sem_give(&data->sem);

	return ret;
}

int fxos8700_get_power(const struct device *dev, enum fxos8700_power *power)
{
	const struct fxos8700_config *config = dev->config;
	uint8_t val;

	if (config->ops->byte_read(dev, FXOS8700_REG_CTRLREG1, &val)) {
		LOG_ERR("Could not get power setting");
		return -EIO;
	}
	val &= FXOS8700_M_CTRLREG1_MODE_MASK;
	*power = val;

	return 0;
}

int fxos8700_set_power(const struct device *dev, enum fxos8700_power power)
{
	const struct fxos8700_config *config = dev->config;

	return config->ops->reg_field_update(dev, FXOS8700_REG_CTRLREG1,
				      FXOS8700_CTRLREG1_ACTIVE_MASK, power);
}

static int fxos8700_init(const struct device *dev)
{
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;
	struct sensor_value odr = {.val1 = 6, .val2 = 250000};

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	if (config->inst_on_bus == FXOS8700_BUS_I2C) {
		if (!device_is_ready(config->bus_cfg.i2c.bus)) {
			LOG_ERR("I2C bus device not ready");
			return -ENODEV;
		}
	}
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	if (config->inst_on_bus == FXOS8700_BUS_SPI) {
		if (!device_is_ready(config->bus_cfg.spi.bus)) {
			LOG_ERR("SPI bus device not ready");
			return -ENODEV;
		}
	}
#endif

	if (config->reset_gpio.port) {
		/* Pulse RST pin high to perform a hardware reset of
		 * the sensor.
		 */

		if (!gpio_is_ready_dt(&config->reset_gpio)) {
			LOG_ERR("GPIO device not ready");
			return -ENODEV;
		}

		gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);

		gpio_pin_set_dt(&config->reset_gpio, 1);
		/* The datasheet does not mention how long to pulse
		 * the RST pin high in order to reset. Stay on the
		 * safe side and pulse for 1 millisecond.
		 */
		k_busy_wait(USEC_PER_MSEC);
		gpio_pin_set_dt(&config->reset_gpio, 0);
	} else {
		/* Software reset the sensor. Upon issuing a software
		 * reset command over the I2C interface, the sensor
		 * immediately resets and does not send any
		 * acknowledgment (ACK) of the written byte to the
		 * master. Therefore, do not check the return code of
		 * the I2C transaction.
		 */
		config->ops->byte_write(dev, FXOS8700_REG_CTRLREG2,
				      FXOS8700_CTRLREG2_RST_MASK);
	}

	/* The sensor requires us to wait 1 ms after a reset before
	 * attempting further communications.
	 */
	k_busy_wait(USEC_PER_MSEC);

	/*
	 * Read the WHOAMI register to make sure we are talking to FXOS8700 or
	 * compatible device and not some other type of device that happens to
	 * have the same I2C address.
	 */
	if (config->ops->byte_read(dev, FXOS8700_REG_WHOAMI,
				 &data->whoami)) {
		LOG_ERR("Could not get WHOAMI value");
		return -EIO;
	}

	switch (data->whoami) {
	case WHOAMI_ID_MMA8451:
	case WHOAMI_ID_MMA8652:
	case WHOAMI_ID_MMA8653:
		if (config->mode != FXOS8700_MODE_ACCEL) {
			LOG_ERR("Device 0x%x supports only "
				    "accelerometer mode",
				    data->whoami);
			return -EIO;
		}
		break;
	case WHOAMI_ID_FXOS8700:
		LOG_DBG("Device ID 0x%x", data->whoami);
		break;
	default:
		LOG_ERR("Unknown Device ID 0x%x", data->whoami);
		return -EIO;
	}

	if (fxos8700_set_odr(dev, &odr)) {
		LOG_ERR("Could not set default data rate");
		return -EIO;
	}

	if (config->ops->reg_field_update(dev, FXOS8700_REG_CTRLREG2,
				   FXOS8700_CTRLREG2_MODS_MASK,
				   config->power_mode)) {
		LOG_ERR("Could not set power scheme");
		return -EIO;
	}

	/* Set the mode (accel-only, mag-only, or hybrid) */
	if (config->ops->reg_field_update(dev, FXOS8700_REG_M_CTRLREG1,
				   FXOS8700_M_CTRLREG1_MODE_MASK,
				   config->mode)) {
		LOG_ERR("Could not set mode");
		return -EIO;
	}

	/* Set hybrid autoincrement so we can read accel and mag channels in
	 * one I2C/SPI transaction.
	 */
	if (config->ops->reg_field_update(dev, FXOS8700_REG_M_CTRLREG2,
				   FXOS8700_M_CTRLREG2_AUTOINC_MASK,
				   FXOS8700_M_CTRLREG2_AUTOINC_MASK)) {
		LOG_ERR("Could not set hybrid autoincrement");
		return -EIO;
	}

	/* Set the full-scale range */
	if (config->ops->reg_field_update(dev, FXOS8700_REG_XYZ_DATA_CFG,
				   FXOS8700_XYZ_DATA_CFG_FS_MASK,
				   RANGE2FS(config->range))) {
		LOG_ERR("Could not set range");
		return -EIO;
	}

	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

#if CONFIG_FXOS8700_TRIGGER
	if (fxos8700_trigger_init(dev)) {
		LOG_ERR("Could not initialize interrupts");
		return -EIO;
	}
#endif

	/* Set active */
	if (fxos8700_set_power(dev, FXOS8700_POWER_ACTIVE)) {
		LOG_ERR("Could not set active");
		return -EIO;
	}
	k_sem_give(&data->sem);

	LOG_DBG("Init complete");

	return 0;
}

static DEVICE_API(sensor, fxos8700_driver_api) = {
	.sample_fetch = fxos8700_sample_fetch,
	.channel_get = fxos8700_channel_get,
	.attr_set = fxos8700_attr_set,
#if CONFIG_FXOS8700_TRIGGER
	.trigger_set = fxos8700_trigger_set,
#endif
};

#define FXOS8700_MODE_PROPS_ACCEL					\
	.mode = FXOS8700_MODE_ACCEL,					\
	.start_addr = FXOS8700_REG_OUTXMSB,				\
	.start_channel = FXOS8700_CHANNEL_ACCEL_X,			\
	.num_channels = FXOS8700_NUM_ACCEL_CHANNELS,

#define FXOS8700_MODE_PROPS_MAGN					\
	.mode = FXOS8700_MODE_MAGN,					\
	.start_addr = FXOS8700_REG_M_OUTXMSB,				\
	.start_channel = FXOS8700_CHANNEL_MAGN_X,			\
	.num_channels = FXOS8700_NUM_MAG_CHANNELS,

#define FXOS8700_MODE_PROPS_HYBRID					\
	.mode = FXOS8700_MODE_HYBRID,					\
	.start_addr = FXOS8700_REG_OUTXMSB,				\
	.start_channel = FXOS8700_CHANNEL_ACCEL_X,			\
	.num_channels = FXOS8700_NUM_HYBRID_CHANNELS,			\

#define FXOS8700_MODE(n)						\
	COND_CODE_1(CONFIG_FXOS8700_MODE_ACCEL,				\
		    (FXOS8700_MODE_PROPS_ACCEL),			\
		    (COND_CODE_1(CONFIG_FXOS8700_MODE_MAGN,		\
		    (FXOS8700_MODE_PROPS_MAGN),				\
		    (FXOS8700_MODE_PROPS_HYBRID))))

#define FXOS8700_RESET_PROPS(n)						\
	.reset_gpio = GPIO_DT_SPEC_INST_GET(n, reset_gpios),

#define FXOS8700_RESET(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, reset_gpios),		\
		    (FXOS8700_RESET_PROPS(n)),				\
		    ())

#define FXOS8700_INTM_PROPS(n, m)					\
	.int_gpio = GPIO_DT_SPEC_INST_GET(n, int##m##_gpios),

#define FXOS8700_INT_PROPS(n)						\
	COND_CODE_1(CONFIG_FXOS8700_DRDY_INT1,				\
		    (FXOS8700_INTM_PROPS(n, 1)),			\
		    (FXOS8700_INTM_PROPS(n, 2)))

#define FXOS8700_INT(n)							\
	COND_CODE_1(CONFIG_FXOS8700_TRIGGER,				\
		    (FXOS8700_INT_PROPS(n)),				\
		    ())

#define FXOS8700_PULSE_PROPS(n)						\
	.pulse_cfg = DT_INST_PROP(n, pulse_cfg),			\
	.pulse_ths[0] = DT_INST_PROP(n, pulse_thsx),			\
	.pulse_ths[1] = DT_INST_PROP(n, pulse_thsy),			\
	.pulse_ths[2] = DT_INST_PROP(n, pulse_thsz),			\
	.pulse_tmlt = DT_INST_PROP(n, pulse_tmlt),			\
	.pulse_ltcy = DT_INST_PROP(n, pulse_ltcy),			\
	.pulse_wind = DT_INST_PROP(n, pulse_wind),

#define FXOS8700_PULSE(n)						\
	COND_CODE_1(CONFIG_FXOS8700_PULSE,				\
		    (FXOS8700_PULSE_PROPS(n)),				\
		    ())

#define FXOS8700_MAG_VECM_PROPS(n)					\
	.mag_vecm_cfg = DT_INST_PROP(n, mag_vecm_cfg),			\
	.mag_vecm_ths[0] = DT_INST_PROP(n, mag_vecm_ths_msb),		\
	.mag_vecm_ths[1] = DT_INST_PROP(n, mag_vecm_ths_lsb),

#define FXOS8700_MAG_VECM(n)						\
	COND_CODE_1(CONFIG_FXOS8700_MAG_VECM,				\
		    (FXOS8700_MAG_VECM_PROPS(n)),			\
		    ())

#define FXOS8700_CONFIG_I2C(n)						\
		.bus_cfg = { .i2c = I2C_DT_SPEC_INST_GET(n) },		\
		.ops = &fxos8700_i2c_ops,				\
		.power_mode = DT_INST_PROP(n, power_mode),		\
		.range = DT_INST_PROP(n, range),			\
		.inst_on_bus = FXOS8700_BUS_I2C,

#define FXOS8700_CONFIG_SPI(n)						\
		.bus_cfg = { .spi = SPI_DT_SPEC_INST_GET(n,		\
			SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0) },	\
		.ops = &fxos8700_spi_ops,				\
		.power_mode =  DT_INST_PROP(n, power_mode),		\
		.range = DT_INST_PROP(n, range),			\
		.inst_on_bus = FXOS8700_BUS_SPI,			\

#define FXOS8700_SPI_OPERATION (SPI_WORD_SET(8) |			\
				SPI_OP_MODE_MASTER)			\

#define FXOS8700_INIT(n)						\
	static const struct fxos8700_config fxos8700_config_##n = {	\
	COND_CODE_1(DT_INST_ON_BUS(n, spi),				\
		(FXOS8700_CONFIG_SPI(n)),				\
		(FXOS8700_CONFIG_I2C(n)))				\
		FXOS8700_RESET(n)					\
		FXOS8700_MODE(n)					\
		FXOS8700_INT(n)						\
		FXOS8700_PULSE(n)					\
		FXOS8700_MAG_VECM(n)					\
	};								\
									\
	static struct fxos8700_data fxos8700_data_##n;			\
									\
	SENSOR_DEVICE_DT_INST_DEFINE(n,					\
				     fxos8700_init,			\
				     NULL,				\
				     &fxos8700_data_##n,		\
				     &fxos8700_config_##n,		\
				     POST_KERNEL,			\
				     CONFIG_SENSOR_INIT_PRIORITY,	\
				     &fxos8700_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FXOS8700_INIT)
