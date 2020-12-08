/* adxl362.c - ADXL362 Three-Axis Digital Accelerometers */

/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl362

#include <kernel.h>
#include <string.h>
#include <drivers/sensor.h>
#include <init.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <drivers/spi.h>
#include <logging/log.h>

#include "adxl362.h"

LOG_MODULE_REGISTER(ADXL362, CONFIG_SENSOR_LOG_LEVEL);

static struct adxl362_data adxl362_data;

static int adxl362_reg_access(struct adxl362_data *ctx, uint8_t cmd,
			      uint8_t reg_addr, void *data, size_t length)
{
	uint8_t access[2] = { cmd, reg_addr };
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

static inline int adxl362_set_reg(const struct device *dev,
				  uint16_t register_value,
				  uint8_t register_address, uint8_t count)
{
	struct adxl362_data *adxl362_data = dev->data;

	return adxl362_reg_access(adxl362_data,
				  ADXL362_WRITE_REG,
				  register_address,
				  &register_value,
				  count);
}

int adxl362_reg_write_mask(const struct device *dev, uint8_t register_address,
			   uint8_t mask, uint8_t data)
{
	int ret;
	uint8_t tmp;
	struct adxl362_data *adxl362_data = dev->data;

	ret = adxl362_reg_access(adxl362_data,
				 ADXL362_READ_REG,
				 register_address,
				 &tmp,
				 1);

	if (ret) {
		return ret;
	}

	tmp &= ~mask;
	tmp |= data;

	return adxl362_reg_access(adxl362_data,
				  ADXL362_WRITE_REG,
				  register_address,
				  &tmp,
				  1);
}

static inline int adxl362_get_reg(const struct device *dev, uint8_t *read_buf,
				  uint8_t register_address, uint8_t count)
{
	struct adxl362_data *adxl362_data = dev->data;

	return adxl362_reg_access(adxl362_data,
				  ADXL362_READ_REG,
				  register_address,
				  read_buf, count);
}

#if defined(CONFIG_ADXL362_TRIGGER)
static int adxl362_interrupt_config(const struct device *dev,
				    uint8_t int1,
				    uint8_t int2)
{
	int ret;
	struct adxl362_data *adxl362_data = dev->data;

	ret = adxl362_reg_access(adxl362_data,
				  ADXL362_WRITE_REG,
				  ADXL362_REG_INTMAP1,
				  &int1,
				  1);

	if (ret) {
		return ret;
	}

	return ret = adxl362_reg_access(adxl362_data,
					ADXL362_WRITE_REG,
					ADXL362_REG_INTMAP2,
					&int2,
					1);
}

int adxl362_get_status(const struct device *dev, uint8_t *status)
{
	return adxl362_get_reg(dev, status, ADXL362_REG_STATUS, 1);
}

int adxl362_clear_data_ready(const struct device *dev)
{
	uint8_t buf;
	/* Reading any data register clears the data ready interrupt */
	return adxl362_get_reg(dev, &buf, ADXL362_REG_XDATA, 1);
}
#endif

static int adxl362_software_reset(const struct device *dev)
{
	return adxl362_set_reg(dev, ADXL362_RESET_KEY,
			       ADXL362_REG_SOFT_RESET, 1);
}

static int adxl362_set_power_mode(const struct device *dev, uint8_t mode)
{
	uint8_t old_power_ctl;
	uint8_t new_power_ctl;
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
 * Since we don't need a finer frequency resolution than milliHz, use uint16_t
 * to save some flash.
 */
static const struct {
	uint16_t freq_int;
	uint16_t freq_milli; /* User should convert to uHz before setting the
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

static int adxl362_freq_to_odr_val(uint16_t freq_int, uint16_t freq_milli)
{
	size_t i;

	/* An ODR of 0 Hz is not allowed */
	if (freq_int == 0U && freq_milli == 0U) {
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
	uint16_t range;
	uint8_t reg_val;
} adxl362_acc_range_map[] = {
	{2,	ADXL362_RANGE_2G},
	{4,	ADXL362_RANGE_4G},
	{8,	ADXL362_RANGE_8G},
};

static int32_t adxl362_range_to_reg_val(uint16_t range)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(adxl362_acc_range_map); i++) {
		if (range <= adxl362_acc_range_map[i].range) {
			return adxl362_acc_range_map[i].reg_val;
		}
	}

	return -EINVAL;
}

static int adxl362_set_range(const struct device *dev, uint8_t range)
{
	struct adxl362_data *adxl362_data = dev->data;
	uint8_t old_filter_ctl;
	uint8_t new_filter_ctl;
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

	adxl362_data->selected_range = range;
	return 0;
}

static int adxl362_set_output_rate(const struct device *dev, uint8_t out_rate)
{
	uint8_t old_filter_ctl;
	uint8_t new_filter_ctl;

	adxl362_get_reg(dev, &old_filter_ctl, ADXL362_REG_FILTER_CTL, 1);
	new_filter_ctl = old_filter_ctl & ~ADXL362_FILTER_CTL_ODR(0x7);
	new_filter_ctl = new_filter_ctl | ADXL362_FILTER_CTL_ODR(out_rate);
	adxl362_set_reg(dev, new_filter_ctl, ADXL362_REG_FILTER_CTL, 1);

	return 0;
}


static int axl362_acc_config(const struct device *dev,
			     enum sensor_channel chan,
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
			LOG_DBG("invalid range requested.");
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
			LOG_DBG("invalid output rate.");
			return -ENOTSUP;
		}

		return adxl362_set_output_rate(dev, out_rate);
	}
	break;
#endif
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int adxl362_attr_set_thresh(const struct device *dev,
				   enum sensor_channel chan,
				   enum sensor_attribute attr,
				   const struct sensor_value *val)
{
	uint8_t reg;
	uint16_t threshold = val->val1;
	size_t ret;

	if (chan != SENSOR_CHAN_ACCEL_X &&
	    chan != SENSOR_CHAN_ACCEL_Y &&
	    chan != SENSOR_CHAN_ACCEL_Z) {
		return -EINVAL;
	}

	if (threshold > 2047) {
		return -EINVAL;
	}

	/* Configure motion threshold. */
	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		reg = ADXL362_REG_THRESH_ACT_L;
	} else {
		reg = ADXL362_REG_THRESH_INACT_L;
	}

	ret = adxl362_set_reg(dev, (threshold & 0x7FF), reg, 2);

	return ret;
}

static int adxl362_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_UPPER_THRESH:
	case SENSOR_ATTR_LOWER_THRESH:
		return adxl362_attr_set_thresh(dev, chan, attr, val);
	default:
		/* Do nothing */
		break;
	}

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return axl362_acc_config(dev, chan, attr, val);
	default:
		LOG_DBG("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int adxl362_fifo_setup(const struct device *dev, uint8_t mode,
			      uint16_t water_mark_lvl, uint8_t en_temp_read)
{
	uint8_t write_val;
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

static int adxl362_setup_activity_detection(const struct device *dev,
					    uint8_t ref_or_abs,
					    uint16_t threshold,
					    uint8_t time)
{
	uint8_t old_act_inact_reg;
	uint8_t new_act_inact_reg;
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

static int adxl362_setup_inactivity_detection(const struct device *dev,
					      uint8_t ref_or_abs,
					      uint16_t threshold,
					      uint16_t time)
{
	uint8_t old_act_inact_reg;
	uint8_t new_act_inact_reg;
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

int adxl362_set_interrupt_mode(const struct device *dev, uint8_t mode)
{
	uint8_t old_act_inact_reg;
	uint8_t new_act_inact_reg;
	int ret;

	LOG_DBG("Mode: %d", mode);

	if (mode != ADXL362_MODE_DEFAULT &&
	    mode != ADXL362_MODE_LINK &&
	    mode != ADXL362_MODE_LOOP) {
		    LOG_ERR("Wrong mode");
		    return -EINVAL;
	}

	/* Select desired interrupt mode. */
	ret = adxl362_get_reg(dev, &old_act_inact_reg,
			      ADXL362_REG_ACT_INACT_CTL, 1);
	if (ret) {
		return ret;
	}

	new_act_inact_reg = old_act_inact_reg &
			    ~ADXL362_ACT_INACT_CTL_LINKLOOP(3);
	new_act_inact_reg |= old_act_inact_reg |
			    ADXL362_ACT_INACT_CTL_LINKLOOP(mode);

	ret = adxl362_set_reg(dev, new_act_inact_reg,
			      ADXL362_REG_ACT_INACT_CTL, 1);

	if (ret) {
		return ret;
	}

	return 0;
}

static int adxl362_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct adxl362_data *data = dev->data;
	int16_t buf[4];
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	ret = adxl362_get_reg(dev, (uint8_t *)buf, ADXL362_REG_XDATA_L,
			      sizeof(buf));
	if (ret) {
		return ret;
	}

	data->acc_x = sys_le16_to_cpu(buf[0]);
	data->acc_y = sys_le16_to_cpu(buf[1]);
	data->acc_z = sys_le16_to_cpu(buf[2]);
	data->temp = sys_le16_to_cpu(buf[3]);

	return 0;
}

static inline int adxl362_range_to_scale(int range)
{
	/* See table 1 in specifications section of datasheet */
	switch (range) {
	case ADXL362_RANGE_2G:
		return ADXL362_ACCEL_2G_LSB_PER_G;
	case ADXL362_RANGE_4G:
		return ADXL362_ACCEL_4G_LSB_PER_G;
	case ADXL362_RANGE_8G:
		return ADXL362_ACCEL_8G_LSB_PER_G;
	default:
		return -EINVAL;
	}
}

static void adxl362_accel_convert(struct sensor_value *val, int accel,
				  int range)
{
	int scale = adxl362_range_to_scale(range);
	long micro_ms2 = accel * SENSOR_G / scale;

	__ASSERT_NO_MSG(scale != -EINVAL);

	val->val1 = micro_ms2 / 1000000;
	val->val2 = micro_ms2 % 1000000;
}

static void adxl362_temp_convert(struct sensor_value *val, int temp)
{
	/* See sensitivity and bias specifications in table 1 of datasheet */
	int milli_c = (temp - ADXL362_TEMP_BIAS_LSB) * ADXL362_TEMP_MC_PER_LSB;

	val->val1 = milli_c / 1000;
	val->val2 = (milli_c % 1000) * 1000;
}

static int adxl362_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adxl362_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X: /* Acceleration on the X axis, in m/s^2. */
		adxl362_accel_convert(val, data->acc_x, data->selected_range);
		break;
	case SENSOR_CHAN_ACCEL_Y: /* Acceleration on the Y axis, in m/s^2. */
		adxl362_accel_convert(val, data->acc_y, data->selected_range);
		break;
	case SENSOR_CHAN_ACCEL_Z: /* Acceleration on the Z axis, in m/s^2. */
		adxl362_accel_convert(val, data->acc_z,  data->selected_range);
		break;
	case SENSOR_CHAN_DIE_TEMP: /* Temperature in degrees Celsius. */
		adxl362_temp_convert(val, data->temp);
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
#ifdef CONFIG_ADXL362_TRIGGER
	.trigger_set = adxl362_trigger_set,
#endif
};

static int adxl362_chip_init(const struct device *dev)
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
	ret =
	adxl362_setup_activity_detection(dev,
					 CONFIG_ADXL362_ABS_REF_MODE,
					 CONFIG_ADXL362_ACTIVITY_THRESHOLD,
					 CONFIG_ADXL362_ACTIVITY_TIME);
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
	ret =
	adxl362_setup_inactivity_detection(dev,
					   CONFIG_ADXL362_ABS_REF_MODE,
					   CONFIG_ADXL362_INACTIVITY_THRESHOLD,
					   CONFIG_ADXL362_INACTIVITY_TIME);
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
static int adxl362_init(const struct device *dev)
{
	const struct adxl362_config *config = dev->config;
	struct adxl362_data *data = dev->data;
	uint8_t value;
	int err;

	data->spi = device_get_binding(config->spi_name);
	if (!data->spi) {
		LOG_DBG("spi device not found: %s", config->spi_name);
		return -EINVAL;
	}

	data->spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
	data->spi_cfg.frequency = config->spi_max_frequency;
	data->spi_cfg.slave = config->spi_slave;

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	data->adxl362_cs_ctrl.gpio_dev =
				device_get_binding(config->gpio_cs_port);
	if (!data->adxl362_cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	data->adxl362_cs_ctrl.gpio_pin = config->cs_gpio;
	data->adxl362_cs_ctrl.gpio_dt_flags = config->cs_flags;
	data->adxl362_cs_ctrl.delay = 0U;

	data->spi_cfg.cs = &data->adxl362_cs_ctrl;
#endif

	err = adxl362_software_reset(dev);

	if (err) {
		LOG_ERR("adxl362_software_reset failed, error %d\n", err);
		return -ENODEV;
	}

	k_sleep(K_MSEC(5));

	adxl362_get_reg(dev, &value, ADXL362_REG_PARTID, 1);
	if (value != ADXL362_PART_ID) {
		LOG_ERR("Failed: %d\n", value);
		return -ENODEV;
	}

	if (adxl362_chip_init(dev) < 0) {
		return -ENODEV;
	}

#if defined(CONFIG_ADXL362_TRIGGER)
	if (adxl362_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return -EIO;
	}

	if (adxl362_interrupt_config(dev,
				     config->int1_config,
				     config->int2_config) < 0) {
		LOG_ERR("Failed to configure interrupt");
		return -EIO;
	}
#endif

	return 0;
}

static const struct adxl362_config adxl362_config = {
	.spi_name = DT_INST_BUS_LABEL(0),
	.spi_slave = DT_INST_REG_ADDR(0),
	.spi_max_frequency = DT_INST_PROP(0, spi_max_frequency),
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	.gpio_cs_port = DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
	.cs_gpio = DT_INST_SPI_DEV_CS_GPIOS_PIN(0),
	.cs_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0),
#endif
#if defined(CONFIG_ADXL362_TRIGGER)
	.gpio_port = DT_INST_GPIO_LABEL(0, int1_gpios),
	.int_gpio = DT_INST_GPIO_PIN(0, int1_gpios),
	.int_flags = DT_INST_GPIO_FLAGS(0, int1_gpios),
#endif
};

DEVICE_DT_INST_DEFINE(0, adxl362_init, device_pm_control_nop,
		    &adxl362_data, &adxl362_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &adxl362_api_funcs);
