/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_fxos8700

#include "fxos8700.h"
#include <sys/util.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(FXOS8700, CONFIG_SENSOR_LOG_LEVEL);

/* Convert the range (8g, 4g, 2g) to the encoded FS register field value */
#define RANGE2FS(x) (__builtin_ctz(x) - 1)

int fxos8700_set_odr(const struct device *dev, const struct sensor_value *val)
{
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;
	int32_t dr = val->val1;

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

	LOG_DBG("Set ODR to 0x%x", (uint8_t)dr);

	return i2c_reg_update_byte(data->i2c, config->i2c_address,
				   FXOS8700_REG_CTRLREG1,
				   FXOS8700_CTRLREG1_DR_MASK, (uint8_t)dr);
}

static int fxos8700_set_mt_ths(const struct device *dev,
			       const struct sensor_value *val)
{
#ifdef CONFIG_FXOS8700_MOTION
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;
	uint64_t micro_ms2 = abs(val->val1 * 1000000LL + val->val2);
	uint64_t ths = micro_ms2 / FXOS8700_FF_MT_THS_SCALE;

	if (ths > FXOS8700_FF_MT_THS_MASK) {
		LOG_ERR("Threshold value is out of range");
		return -EINVAL;
	}

	LOG_DBG("Set FF_MT_THS to %d", (uint8_t)ths);

	return i2c_reg_update_byte(data->i2c, config->i2c_address,
				   FXOS8700_REG_FF_MT_THS,
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

	/* Read all the channels in one I2C transaction. The number of bytes to
	 * read and the starting register address depend on the mode
	 * configuration (accel-only, mag-only, or hybrid).
	 */
	num_bytes = config->num_channels * FXOS8700_BYTES_PER_CHANNEL_NORMAL;

	__ASSERT(num_bytes <= sizeof(buffer), "Too many bytes to read");

	if (i2c_burst_read(data->i2c, config->i2c_address, config->start_addr,
			   buffer, num_bytes)) {
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
	if (i2c_reg_read_byte(data->i2c, config->i2c_address, FXOS8700_REG_TEMP,
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
	struct fxos8700_data *data = dev->data;
	uint8_t val = *power;

	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      FXOS8700_REG_CTRLREG1,
			      &val)) {
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
	struct fxos8700_data *data = dev->data;

	return i2c_reg_update_byte(data->i2c, config->i2c_address,
				   FXOS8700_REG_CTRLREG1,
				   FXOS8700_CTRLREG1_ACTIVE_MASK,
				   power);
}

static int fxos8700_init(const struct device *dev)
{
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;
	struct sensor_value odr = {.val1 = 6, .val2 = 250000};
	const struct device *rst;

	/* Get the I2C device */
	data->i2c = device_get_binding(config->i2c_name);
	if (data->i2c == NULL) {
		LOG_ERR("Could not find I2C device");
		return -EINVAL;
	}

	if (config->reset_name) {
		/* Pulse RST pin high to perform a hardware reset of
		 * the sensor.
		 */
		rst = device_get_binding(config->reset_name);
		if (!rst) {
			LOG_ERR("Could not find reset GPIO device");
			return -EINVAL;
		}

		gpio_pin_configure(rst, config->reset_pin,
				   GPIO_OUTPUT_INACTIVE | config->reset_flags);

		gpio_pin_set(rst, config->reset_pin, 1);
		/* The datasheet does not mention how long to pulse
		 * the RST pin high in order to reset. Stay on the
		 * safe side and pulse for 1 millisecond.
		 */
		k_busy_wait(USEC_PER_MSEC);
		gpio_pin_set(rst, config->reset_pin, 0);
	} else {
		/* Software reset the sensor. Upon issuing a software
		 * reset command over the I2C interface, the sensor
		 * immediately resets and does not send any
		 * acknowledgment (ACK) of the written byte to the
		 * master. Therefore, do not check the return code of
		 * the I2C transaction.
		 */
		i2c_reg_write_byte(data->i2c, config->i2c_address,
				   FXOS8700_REG_CTRLREG2,
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
	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      FXOS8700_REG_WHOAMI, &data->whoami)) {
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

	if (i2c_reg_update_byte(data->i2c, config->i2c_address,
				FXOS8700_REG_CTRLREG2,
				FXOS8700_CTRLREG2_MODS_MASK,
				config->power_mode)) {
		LOG_ERR("Could not set power scheme");
		return -EIO;
	}

	/* Set the mode (accel-only, mag-only, or hybrid) */
	if (i2c_reg_update_byte(data->i2c, config->i2c_address,
				FXOS8700_REG_M_CTRLREG1,
				FXOS8700_M_CTRLREG1_MODE_MASK,
				config->mode)) {
		LOG_ERR("Could not set mode");
		return -EIO;
	}

	/* Set hybrid autoincrement so we can read accel and mag channels in
	 * one I2C transaction.
	 */
	if (i2c_reg_update_byte(data->i2c, config->i2c_address,
				FXOS8700_REG_M_CTRLREG2,
				FXOS8700_M_CTRLREG2_AUTOINC_MASK,
				FXOS8700_M_CTRLREG2_AUTOINC_MASK)) {
		LOG_ERR("Could not set hybrid autoincrement");
		return -EIO;
	}

	/* Set the full-scale range */
	if (i2c_reg_update_byte(data->i2c, config->i2c_address,
				FXOS8700_REG_XYZ_DATA_CFG,
				FXOS8700_XYZ_DATA_CFG_FS_MASK,
				RANGE2FS(config->range))) {
		LOG_ERR("Could not set range");
		return -EIO;
	}

	k_sem_init(&data->sem, 0, UINT_MAX);

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

static const struct sensor_driver_api fxos8700_driver_api = {
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
	.reset_name = DT_INST_GPIO_LABEL(n, reset_gpios),		\
	.reset_pin = DT_INST_GPIO_PIN(n, reset_gpios),			\
	.reset_flags = DT_INST_GPIO_FLAGS(n, reset_gpios),

#define FXOS8700_RESET(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, reset_gpios),		\
		    (FXOS8700_RESET_PROPS(n)),				\
		    ())

#define FXOS8700_INTM_PROPS(n, m)					\
	.gpio_name = DT_INST_GPIO_LABEL(n, int##m##_gpios),		\
	.gpio_pin = DT_INST_GPIO_PIN(n, int##m##_gpios),		\
	.gpio_flags = DT_INST_GPIO_FLAGS(n, int##m##_gpios),

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

#define FXOS8700_INIT(n)						\
	static const struct fxos8700_config fxos8700_config_##n = {	\
		.i2c_name = DT_INST_BUS_LABEL(n),			\
		.i2c_address = DT_INST_REG_ADDR(n),			\
		.power_mode = DT_INST_PROP(n, power_mode),		\
		.range = DT_INST_PROP(n, range),			\
		FXOS8700_RESET(n)					\
		FXOS8700_MODE(n)					\
		FXOS8700_INT(n)						\
		FXOS8700_PULSE(n)					\
		FXOS8700_MAG_VECM(n)					\
	};								\
									\
	static struct fxos8700_data fxos8700_data_##n;			\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    fxos8700_init,				\
			    device_pm_control_nop,			\
			    &fxos8700_data_##n,				\
			    &fxos8700_config_##n,			\
			    POST_KERNEL,				\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &fxos8700_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FXOS8700_INIT)
