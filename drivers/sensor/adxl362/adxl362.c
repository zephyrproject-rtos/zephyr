/* adxl362.c - ADXL362 Three-Axis Digital Accelerometers */
/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>
#include <sensor.h>
#include <init.h>
#include <gpio.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>
#include <spi.h>

#include "adxl362.h"


static struct adxl362_data adxl362_data;

static int adxl362_reg_access(struct adxl362_data *ctx, u8_t cmd,
			      u8_t reg_addr, void *data, size_t length)
{
	u8_t access[2] = { cmd, reg_addr };
	const struct spi_buf buf[2] = {
		{
			.buf = access,
			.len = 2
		},
		{
			.buf = data,
			.len = length
		}
	};
	struct spi_buf_set tx = {
		.buffers = buf,
	};

	if (cmd == ADXL362_READ_REG) {
		const struct spi_buf_set rx = {
			.buffers = buf,
			.count = 2
		};

		tx.count = 1;

		return spi_transceive(ctx->spi, &ctx->spi_cfg, &tx, &rx);
	}

	tx.count = 2;

	return spi_write(ctx->spi, &ctx->spi_cfg, &tx);
}

static inline int adxl362_set_reg(struct device *dev, u16_t register_value,
				  u8_t register_address, u8_t count)
{
	struct adxl362_data *adxl362_data = dev->driver_data;

	return adxl362_reg_access(adxl362_data,
				  ADXL362_WRITE_REG,
				  register_address,
				  &register_value,
				  count);
}

static inline int adxl362_get_reg(struct device *dev, u8_t *read_buf,
				  u8_t register_address, u8_t count)
{
	struct adxl362_data *adxl362_data = dev->driver_data;

	return adxl362_reg_access(adxl362_data,
				  ADXL362_READ_REG,
				  register_address,
				  read_buf, count);
}

static int adxl362_software_reset(struct device *dev)
{
	return adxl362_set_reg(dev, ADXL362_RESET_KEY,
			       ADXL362_REG_SOFT_RESET, 1);
}

static int adxl362_set_power_mode(struct device *dev, u8_t mode)
{
	u8_t old_power_ctl;
	u8_t new_power_ctl;
	int ret;

	ret = adxl362_get_reg(dev, &old_power_ctl, ADXL362_REG_POWER_CTL, 1);
	if (ret) {
		return ret;
	}

	new_power_ctl = old_power_ctl & ~ADXL362_POWER_CTL_MEASURE(0x3);
	new_power_ctl = new_power_ctl |
		      (mode *
		       ADXL362_POWER_CTL_MEASURE(ADXL362_MEASURE_ON));
	return adxl362_set_reg(dev, new_power_ctl, ADXL362_REG_POWER_CTL, 1);
}

/*
 * Output data rate map with allowed frequencies:
 * freq = freq_int + freq_milli / 1000
 *
 * Since we don't need a finer frequency resolution than milliHz, use u16_t
 * to save some flash.
 */
static const struct {
	u16_t freq_int;
	u16_t freq_milli; /* User should convert to uHz before setting the
			      * SENSOR_ATTR_SAMPLING_FREQUENCY attribute.
			      */
} adxl362_odr_map[] = {
	{ 12, 500 },
	{ 25, 0 },
	{ 50, 0 },
	{ 100, 0 },
	{ 200, 0 },
	{ 400, 0 },
};

static int adxl362_freq_to_odr_val(u16_t freq_int, u16_t freq_milli)
{
	size_t i;

	/* An ODR of 0 Hz is not allowed */
	if (freq_int == 0 && freq_milli == 0) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(adxl362_odr_map); i++) {
		if (freq_int < adxl362_odr_map[i].freq_int ||
		    (freq_int == adxl362_odr_map[i].freq_int &&
		     freq_milli <= adxl362_odr_map[i].freq_milli)) {
			return i;
		}
	}

	return -EINVAL;
}

static const struct adxl362_range {
	u16_t range;
	u8_t reg_val;
} adxl362_acc_range_map[] = {
	{2,	ADXL362_RANGE_2G},
	{4,	ADXL362_RANGE_4G},
	{8,	ADXL362_RANGE_8G},
};

static s32_t adxl362_range_to_reg_val(u16_t range)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(adxl362_acc_range_map); i++) {
		if (range <= adxl362_acc_range_map[i].range) {
			return adxl362_acc_range_map[i].reg_val;
		}
	}

	return -EINVAL;
}

static int adxl362_set_range(struct device *dev, u8_t range)
{
	struct adxl362_data *adxl362_data = dev->driver_data;
	u8_t old_filter_ctl;
	u8_t new_filter_ctl;
	int ret;

	ret = adxl362_get_reg(dev, &old_filter_ctl, ADXL362_REG_FILTER_CTL, 1);
	if (ret) {
		return ret;
	}

	new_filter_ctl = old_filter_ctl & ~ADXL362_FILTER_CTL_RANGE(0x3);
	new_filter_ctl = new_filter_ctl | ADXL362_FILTER_CTL_RANGE(range);
	ret = adxl362_set_reg(dev, new_filter_ctl, ADXL362_REG_FILTER_CTL, 1);
	if (ret) {
		return ret;
	}

	adxl362_data->selected_range = (1 << range) * 2;
	return 0;
}

static int adxl362_set_output_rate(struct device *dev, u8_t out_rate)
{
	u8_t old_filter_ctl;
	u8_t new_filter_ctl;

	adxl362_get_reg(dev, &old_filter_ctl, ADXL362_REG_FILTER_CTL, 1);
	new_filter_ctl = old_filter_ctl & ~ADXL362_FILTER_CTL_ODR(0x7);
	new_filter_ctl = new_filter_ctl | ADXL362_FILTER_CTL_ODR(out_rate);
	adxl362_set_reg(dev, new_filter_ctl, ADXL362_REG_FILTER_CTL, 1);

	return 0;
}


static int axl362_acc_config(struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	switch (attr) {
#if defined(CONFIG_ADXL362_ACCEL_RANGE_RUNTIME)
	case SENSOR_ATTR_FULL_SCALE:
	{
		int range_reg;

		range_reg = adxl362_range_to_reg_val(sensor_ms2_to_g(val));
		if (range_reg < 0) {
			SYS_LOG_DBG("invalid range requested.");
			return -ENOTSUP;
		}

		return adxl362_set_range(dev, range_reg);
	}
	break;
#endif
#if defined(CONFIG_ADXL362_ACCEL_ODR_RUNTIME)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
	{
		int out_rate;

		out_rate = adxl362_freq_to_odr_val(val->val1,
						   val->val2 / 1000);
		if (out_rate < 0) {
			SYS_LOG_DBG("invalid output rate.");
			return -ENOTSUP;
		}

		return adxl362_set_output_rate(dev, out_rate);
	}
	break;
#endif
	default:
		SYS_LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int adxl362_attr_set(struct device *dev, enum sensor_channel chan,
		    enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return axl362_acc_config(dev, chan, attr, val);

	default:
		SYS_LOG_DBG("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}


static int adxl362_read_temperature(struct device *dev, s32_t *temp_celsius)
{
	u8_t raw_temp_data[2];
	int ret;

	/* Reads the temperature of the device. */
	ret = adxl362_get_reg(dev, raw_temp_data, ADXL362_REG_TEMP_L, 2);
	if (ret) {
		return ret;
	}

	*temp_celsius = (s32_t)(raw_temp_data[1] << 8) + raw_temp_data[0];
	*temp_celsius *= 65;

	return ret;
}

static int adxl362_fifo_setup(struct device *dev, u8_t mode,
			      u16_t water_mark_lvl, u8_t en_temp_read)
{
	u8_t write_val;
	int ret;

	write_val = ADXL362_FIFO_CTL_FIFO_MODE(mode) |
		   (en_temp_read * ADXL362_FIFO_CTL_FIFO_TEMP) |
		   ADXL362_FIFO_CTL_AH;
	ret = adxl362_set_reg(dev, write_val, ADXL362_REG_FIFO_CTL, 1);
	if (ret) {
		return ret;
	}

	ret = adxl362_set_reg(dev, water_mark_lvl, ADXL362_REG_FIFO_SAMPLES, 2);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adxl362_setup_activity_detection(struct device *dev,
					    u8_t ref_or_abs,
					    u16_t threshold,
					    u8_t time)
{
	u8_t old_act_inact_reg;
	u8_t new_act_inact_reg;
	int ret;

	/**
	 * mode
	 *              must be one of the following:
	 *			ADXL362_FIFO_DISABLE      -  FIFO is disabled.
	 *			ADXL362_FIFO_OLDEST_SAVED -  Oldest saved mode.
	 *			ADXL362_FIFO_STREAM       -  Stream mode.
	 *			ADXL362_FIFO_TRIGGERED    -  Triggered mode.
	 * water_mark_lvl
	 *              Specifies the number of samples to store in the FIFO.
	 * en_temp_read
	 *              Store Temperature Data to FIFO.
	 *              1 - temperature data is stored in the FIFO
	 *                  together with x-, y- and x-axis data.
	 *          0 - temperature data is skipped.
	 */

	/* Configure motion threshold and activity timer. */
	ret = adxl362_set_reg(dev, (threshold & 0x7FF),
			      ADXL362_REG_THRESH_ACT_L, 2);
	if (ret) {
		return ret;
	}

	ret = adxl362_set_reg(dev, time, ADXL362_REG_TIME_ACT, 1);
	if (ret) {
		return ret;
	}

	/* Enable activity interrupt and select a referenced or absolute
	 * configuration.
	 */
	ret = adxl362_get_reg(dev, &old_act_inact_reg,
			      ADXL362_REG_ACT_INACT_CTL, 1);
	if (ret) {
		return ret;
	}

	new_act_inact_reg = old_act_inact_reg & ~ADXL362_ACT_INACT_CTL_ACT_REF;
	new_act_inact_reg |= ADXL362_ACT_INACT_CTL_ACT_EN |
			  (ref_or_abs * ADXL362_ACT_INACT_CTL_ACT_REF);
	ret = adxl362_set_reg(dev, new_act_inact_reg,
			      ADXL362_REG_ACT_INACT_CTL, 1);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adxl362_setup_inactivity_detection(struct device *dev,
					      u8_t ref_or_abs,
					      u16_t threshold,
					      u16_t time)
{
	u8_t old_act_inact_reg;
	u8_t new_act_inact_reg;
	int ret;

	/* Configure motion threshold and inactivity timer. */
	ret = adxl362_set_reg(dev, (threshold & 0x7FF),
			      ADXL362_REG_THRESH_INACT_L, 2);
	if (ret) {
		return ret;
	}

	ret = adxl362_set_reg(dev, time, ADXL362_REG_TIME_INACT_L, 2);
	if (ret) {
		return ret;
	}

	/* Enable inactivity interrupt and select a referenced or
	 * absolute configuration.
	 */
	ret = adxl362_get_reg(dev, &old_act_inact_reg,
			      ADXL362_REG_ACT_INACT_CTL, 1);
	if (ret) {
		return ret;
	}

	new_act_inact_reg = old_act_inact_reg &
			    ~ADXL362_ACT_INACT_CTL_INACT_REF;
	new_act_inact_reg |= ADXL362_ACT_INACT_CTL_INACT_EN |
			     (ref_or_abs * ADXL362_ACT_INACT_CTL_INACT_REF);
	ret = adxl362_set_reg(dev, new_act_inact_reg,
			      ADXL362_REG_ACT_INACT_CTL, 1);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adxl362_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct adxl362_data *data = dev->driver_data;
	u8_t buf[2];
	s16_t x, y, z;
	int ret;

	ret = adxl362_get_reg(dev, buf, ADXL362_REG_XDATA_L, 2);
	if (ret) {
		return ret;
	}

	x = (buf[1] << 8) + buf[0];
	ret = adxl362_get_reg(dev, buf, ADXL362_REG_YDATA_L, 2);
	if (ret) {
		return ret;
	}

	y = (buf[1] << 8) + buf[0];
	ret = adxl362_get_reg(dev, buf, ADXL362_REG_ZDATA_L, 2);
	if (ret) {
		return ret;
	}

	z = (buf[1] << 8) + buf[0];

	data->acc_x = (s32_t)x * (adxl362_data.selected_range / 2);
	data->acc_y = (s32_t)y * (adxl362_data.selected_range / 2);
	data->acc_z = (s32_t)z * (adxl362_data.selected_range / 2);

	ret = adxl362_read_temperature(dev, &data->temp);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adxl362_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adxl362_data *data = dev->driver_data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X: /* Acceleration on the X axis, in m/s^2. */
		val->val1 = data->acc_x / 1000;
		val->val2 = (data->acc_x % 1000) * 1000;
		break;
	case SENSOR_CHAN_ACCEL_Y: /* Acceleration on the Y axis, in m/s^2. */
		val->val1 = data->acc_y / 1000;
		val->val2 = (data->acc_y % 1000) * 1000;
		break;
	case SENSOR_CHAN_ACCEL_Z: /* Acceleration on the Z axis, in m/s^2. */
		val->val1 = data->acc_z / 1000;
		val->val2 = (data->acc_z % 1000) * 1000;
		break;
	case SENSOR_CHAN_DIE_TEMP: /* Temperature in degrees Celsius. */
		val->val1 = data->temp / 1000;
		val->val2 = (data->temp % 1000) * 1000;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api adxl362_api_funcs = {
	.attr_set     = adxl362_attr_set,
	.sample_fetch = adxl362_sample_fetch,
	.channel_get  = adxl362_channel_get,
};

static int adxl362_chip_init(struct device *dev)
{
	int ret;

	/* Configures activity detection.
	 *	Referenced/Absolute Activity or Inactivity Select.
	 *		0 - absolute mode.
	 *		1 - referenced mode.
	 *	threshold
	 *		11-bit unsigned value that the adxl362 samples are
	 *		compared to.
	 *	time
	 *		8-bit value written to the activity timer register.
	 *		The amount of time (in seconds) is:
	 *			time / ODR,
	 *		where ODR - is the output data rate.
	 */
	ret = adxl362_setup_activity_detection(dev, 0, 250, 1);
	if (ret) {
		return ret;
	}

	/* Configures inactivity detection.
	 *	Referenced/Absolute Activity or Inactivity Select.
	 *		0 - absolute mode.
	 *		1 - referenced mode.
	 *	threshold
	 *		11-bit unsigned value that the adxl362 samples are
	 *		compared to.
	 *	time
	 *		16-bit value written to the activity timer register.
	 *		The amount of time (in seconds) is:
	 *			time / ODR,
	 *		where ODR - is the output data rate.
	 */
	ret = adxl362_setup_inactivity_detection(dev, 0, 100, 1);
	if (ret) {
		return ret;
	}

	/* Configures the FIFO feature. */
	ret = adxl362_fifo_setup(dev, ADXL362_FIFO_DISABLE, 0, 0);
	if (ret) {
		return ret;
	}

	/* Selects the measurement range.
	 * options are:
	 *		ADXL362_RANGE_2G  -  +-2 g
	 *		ADXL362_RANGE_4G  -  +-4 g
	 *		ADXL362_RANGE_8G  -  +-8 g
	 */
	ret = adxl362_set_range(dev, ADXL362_DEFAULT_RANGE_ACC);
	if (ret) {
		return ret;
	}

	/* Selects the Output Data Rate of the device.
	 * Options are:
	 *		ADXL362_ODR_12_5_HZ  -  12.5Hz
	 *		ADXL362_ODR_25_HZ    -  25Hz
	 *		ADXL362_ODR_50_HZ    -  50Hz
	 *		ADXL362_ODR_100_HZ   -  100Hz
	 *		ADXL362_ODR_200_HZ   -  200Hz
	 *		ADXL362_ODR_400_HZ   -  400Hz
	 */
	ret = adxl362_set_output_rate(dev, ADXL362_DEFAULT_ODR_ACC);
	if (ret) {
		return ret;
	}

	/* Places the device into measure mode. */
	ret = adxl362_set_power_mode(dev, 1);
	if (ret) {
		return ret;
	}

	return 0;
}

/**
 * @brief Initializes communication with the device and checks if the part is
 *        present by reading the device id.
 *
 * @return  0 - the initialization was successful and the device is present;
 *         -1 - an error occurred.
 *
 */
static int adxl362_init(struct device *dev)
{
	struct adxl362_data *data = dev->driver_data;
	u8_t value;
	int ret;

	data->spi = device_get_binding(CONFIG_ADXL362_SPI_DEV_NAME);
	if (!data->spi) {
		SYS_LOG_DBG("spi device not found: %s",
			    CONFIG_ADXL362_SPI_DEV_NAME);
		return -EINVAL;
	}

	data->spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
		SPI_MODE_CPOL | SPI_MODE_CPHA;
	data->spi_cfg.frequency = 8000000;
	data->spi_cfg.slave = CONFIG_ADXL362_SPI_DEV_SLAVE;

	adxl362_software_reset(dev);

	adxl362_get_reg(dev, &value, ADXL362_REG_PARTID, 1);
	if (value != ADXL362_PART_ID) {
		return -ENODEV;
	}

	if (adxl362_chip_init(dev) < 0) {
		return -ENODEV;
	}

	return 0;
}

DEVICE_AND_API_INIT(adxl362, CONFIG_ADXL362_DEV_NAME, adxl362_init,
		    &adxl362_data, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &adxl362_api_funcs);
