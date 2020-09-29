/* ST Microelectronics IIS2DLPC 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2dlpc.pdf
 */

#define DT_DRV_COMPAT st_iis2dlpc

#include <init.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <logging/log.h>
#include <drivers/sensor.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <drivers/spi.h>
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <drivers/i2c.h>
#endif

#include "iis2dlpc.h"

LOG_MODULE_REGISTER(IIS2DLPC, CONFIG_SENSOR_LOG_LEVEL);

/**
 * iis2dlpc_set_range - set full scale range for acc
 * @dev: Pointer to instance of struct device (I2C or SPI)
 * @range: Full scale range (2, 4, 8 and 16 G)
 */
static int iis2dlpc_set_range(const struct device *dev, uint16_t range)
{
	int err;
	struct iis2dlpc_data *iis2dlpc = dev->data;
	const struct iis2dlpc_device_config *cfg = dev->config;
	uint8_t shift_gain = 0U;
	uint8_t fs = IIS2DLPC_FS_TO_REG(range);

	err = iis2dlpc_full_scale_set(iis2dlpc->ctx, fs);

	if (cfg->pm == IIS2DLPC_CONT_LOW_PWR_12bit) {
		shift_gain = IIS2DLPC_SHFT_GAIN_NOLP1;
	}

	if (!err) {
		/* save internally gain for optimization */
		iis2dlpc->gain =
			IIS2DLPC_FS_TO_GAIN(IIS2DLPC_FS_TO_REG(range),
					    shift_gain);
	}

	return err;
}

/**
 * iis2dlpc_set_odr - set new sampling frequency
 * @dev: Pointer to instance of struct device (I2C or SPI)
 * @odr: Output data rate
 */
static int iis2dlpc_set_odr(const struct device *dev, uint16_t odr)
{
	struct iis2dlpc_data *iis2dlpc = dev->data;
	uint8_t val;

	/* check if power off */
	if (odr == 0U) {
		return iis2dlpc_data_rate_set(iis2dlpc->ctx,
					      IIS2DLPC_XL_ODR_OFF);
	}

	val =  IIS2DLPC_ODR_TO_REG(odr);
	if (val > IIS2DLPC_XL_ODR_1k6Hz) {
		LOG_ERR("ODR too high");
		return -ENOTSUP;
	}

	return iis2dlpc_data_rate_set(iis2dlpc->ctx, val);
}

static inline void iis2dlpc_convert(struct sensor_value *val, int raw_val,
				    float gain)
{
	int64_t dval;

	/* Gain is in ug/LSB */
	/* Convert to m/s^2 */
	dval = ((int64_t)raw_val * gain * SENSOR_G) / 1000000LL;
	val->val1 = dval / 1000000LL;
	val->val2 = dval % 1000000LL;
}

static inline void iis2dlpc_channel_get_acc(const struct device *dev,
					     enum sensor_channel chan,
					     struct sensor_value *val)
{
	int i;
	uint8_t ofs_start, ofs_stop;
	struct iis2dlpc_data *iis2dlpc = dev->data;
	struct sensor_value *pval = val;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		ofs_start = ofs_stop = 0U;
		break;
	case SENSOR_CHAN_ACCEL_Y:
		ofs_start = ofs_stop = 1U;
		break;
	case SENSOR_CHAN_ACCEL_Z:
		ofs_start = ofs_stop = 2U;
		break;
	default:
		ofs_start = 0U; ofs_stop = 2U;
		break;
	}

	for (i = ofs_start; i <= ofs_stop ; i++) {
		iis2dlpc_convert(pval++, iis2dlpc->acc[i], iis2dlpc->gain);
	}
}

static int iis2dlpc_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		iis2dlpc_channel_get_acc(dev, chan, val);
		return 0;
	default:
		LOG_DBG("Channel not supported");
		break;
	}

	return -ENOTSUP;
}

static int iis2dlpc_config(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return iis2dlpc_set_range(dev, sensor_ms2_to_g(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return iis2dlpc_set_odr(dev, val->val1);
	default:
		LOG_DBG("Acc attribute not supported");
		break;
	}

	return -ENOTSUP;
}

static int iis2dlpc_attr_set(const struct device *dev,
			      enum sensor_channel chan,
			      enum sensor_attribute attr,
			      const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return iis2dlpc_config(dev, chan, attr, val);
	default:
		LOG_DBG("Attr not supported on %d channel", chan);
		break;
	}

	return -ENOTSUP;
}

static int iis2dlpc_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct iis2dlpc_data *iis2dlpc = dev->data;
	const struct iis2dlpc_device_config *cfg = dev->config;
	uint8_t shift;
	union axis3bit16_t buf;

	/* fetch raw data sample */
	if (iis2dlpc_acceleration_raw_get(iis2dlpc->ctx, buf.u8bit) < 0) {
		LOG_DBG("Failed to fetch raw data sample");
		return -EIO;
	}

	/* adjust to resolution */
	if (cfg->pm == IIS2DLPC_CONT_LOW_PWR_12bit) {
		shift = IIS2DLPC_SHIFT_PM1;
	} else {
		shift = IIS2DLPC_SHIFT_PMOTHER;
	}

	iis2dlpc->acc[0] = sys_le16_to_cpu(buf.i16bit[0]) >> shift;
	iis2dlpc->acc[1] = sys_le16_to_cpu(buf.i16bit[1]) >> shift;
	iis2dlpc->acc[2] = sys_le16_to_cpu(buf.i16bit[2]) >> shift;

	return 0;
}

static const struct sensor_driver_api iis2dlpc_driver_api = {
	.attr_set = iis2dlpc_attr_set,
#if CONFIG_IIS2DLPC_TRIGGER
	.trigger_set = iis2dlpc_trigger_set,
#endif /* CONFIG_IIS2DLPC_TRIGGER */
	.sample_fetch = iis2dlpc_sample_fetch,
	.channel_get = iis2dlpc_channel_get,
};

static int iis2dlpc_init_interface(const struct device *dev)
{
	struct iis2dlpc_data *iis2dlpc = dev->data;
	const struct iis2dlpc_device_config *cfg = dev->config;

	iis2dlpc->bus = device_get_binding(cfg->bus_name);
	if (!iis2dlpc->bus) {
		LOG_DBG("master bus not found: %s", cfg->bus_name);
		return -EINVAL;
	}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	iis2dlpc_spi_init(dev);
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	iis2dlpc_i2c_init(dev);
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif

	return 0;
}

static int iis2dlpc_set_power_mode(struct iis2dlpc_data *iis2dlpc,
				    iis2dlpc_mode_t pm)
{
	uint8_t regval = IIS2DLPC_CONT_LOW_PWR_12bit;

	switch (pm) {
	case IIS2DLPC_CONT_LOW_PWR_2:
	case IIS2DLPC_CONT_LOW_PWR_3:
	case IIS2DLPC_CONT_LOW_PWR_4:
	case IIS2DLPC_HIGH_PERFORMANCE:
		regval = pm;
		break;
	default:
		LOG_DBG("Apply default Power Mode");
		break;
	}

	return iis2dlpc_write_reg(iis2dlpc->ctx, IIS2DLPC_CTRL1, &regval, 1);
}

static int iis2dlpc_init(const struct device *dev)
{
	struct iis2dlpc_data *iis2dlpc = dev->data;
	const struct iis2dlpc_device_config *cfg = dev->config;
	uint8_t wai;

	if (iis2dlpc_init_interface(dev)) {
		return -EINVAL;
	}

	/* check chip ID */
	if (iis2dlpc_device_id_get(iis2dlpc->ctx, &wai) < 0) {
		return -EIO;
	}

	if (wai != IIS2DLPC_ID) {
		LOG_ERR("Invalid chip ID");
		return -EINVAL;
	}

	/* reset device */
	if (iis2dlpc_reset_set(iis2dlpc->ctx, PROPERTY_ENABLE) < 0) {
		return -EIO;
	}

	k_busy_wait(100);

	if (iis2dlpc_block_data_update_set(iis2dlpc->ctx,
					   PROPERTY_ENABLE) < 0) {
		return -EIO;
	}

	/* set power mode */
	if (iis2dlpc_set_power_mode(iis2dlpc, CONFIG_IIS2DLPC_POWER_MODE)) {
		return -EIO;
	}

	/* set default odr and full scale for acc */
	if (iis2dlpc_data_rate_set(iis2dlpc->ctx, IIS2DLPC_DEFAULT_ODR) < 0) {
		return -EIO;
	}

	if (iis2dlpc_full_scale_set(iis2dlpc->ctx, IIS2DLPC_ACC_FS) < 0) {
		return -EIO;
	}

	iis2dlpc->gain =
		IIS2DLPC_FS_TO_GAIN(IIS2DLPC_ACC_FS,
				    cfg->pm == IIS2DLPC_CONT_LOW_PWR_12bit ?
				    IIS2DLPC_SHFT_GAIN_NOLP1 : 0);

#ifdef CONFIG_IIS2DLPC_TRIGGER
	if (iis2dlpc_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupts");
		return -EIO;
	}

#ifdef CONFIG_IIS2DLPC_PULSE
	if (iis2dlpc_tap_mode_set(iis2dlpc->ctx, cfg->pulse_trigger) < 0) {
		LOG_ERR("Failed to select pulse trigger mode");
		return -EIO;
	}

	if (iis2dlpc_tap_threshold_x_set(iis2dlpc->ctx,
					 cfg->pulse_ths[0]) < 0) {
		LOG_ERR("Failed to set tap X axis threshold");
		return -EIO;
	}

	if (iis2dlpc_tap_threshold_y_set(iis2dlpc->ctx,
					 cfg->pulse_ths[1]) < 0) {
		LOG_ERR("Failed to set tap Y axis threshold");
		return -EIO;
	}

	if (iis2dlpc_tap_threshold_z_set(iis2dlpc->ctx,
					 cfg->pulse_ths[2]) < 0) {
		LOG_ERR("Failed to set tap Z axis threshold");
		return -EIO;
	}

	if (iis2dlpc_tap_detection_on_x_set(iis2dlpc->ctx,
					    CONFIG_IIS2DLPC_PULSE_X) < 0) {
		LOG_ERR("Failed to set tap detection on X axis");
		return -EIO;
	}

	if (iis2dlpc_tap_detection_on_y_set(iis2dlpc->ctx,
					    CONFIG_IIS2DLPC_PULSE_Y) < 0) {
		LOG_ERR("Failed to set tap detection on Y axis");
		return -EIO;
	}

	if (iis2dlpc_tap_detection_on_z_set(iis2dlpc->ctx,
					    CONFIG_IIS2DLPC_PULSE_Z) < 0) {
		LOG_ERR("Failed to set tap detection on Z axis");
		return -EIO;
	}

	if (iis2dlpc_tap_shock_set(iis2dlpc->ctx, cfg->pulse_shock) < 0) {
		LOG_ERR("Failed to set tap shock duration");
		return -EIO;
	}

	if (iis2dlpc_tap_dur_set(iis2dlpc->ctx, cfg->pulse_ltncy) < 0) {
		LOG_ERR("Failed to set tap latency");
		return -EIO;
	}

	if (iis2dlpc_tap_quiet_set(iis2dlpc->ctx, cfg->pulse_quiet) < 0) {
		LOG_ERR("Failed to set tap quiet time");
		return -EIO;
	}
#endif /* CONFIG_IIS2DLPC_PULSE */
#endif /* CONFIG_IIS2DLPC_TRIGGER */

	return 0;
}

const struct iis2dlpc_device_config iis2dlpc_cfg = {
	.bus_name = DT_INST_BUS_LABEL(0),
	.pm = CONFIG_IIS2DLPC_POWER_MODE,
#ifdef CONFIG_IIS2DLPC_TRIGGER
	.int_gpio_port = DT_INST_GPIO_LABEL(0, drdy_gpios),
	.int_gpio_pin = DT_INST_GPIO_PIN(0, drdy_gpios),
	.int_gpio_flags = DT_INST_GPIO_FLAGS(0, drdy_gpios),
#if defined(CONFIG_IIS2DLPC_INT_PIN_1)
	.int_pin = 1,
#elif defined(CONFIG_IIS2DLPC_INT_PIN_2)
	.int_pin = 2,
#endif /* CONFIG_IIS2DLPC_INT_PIN_* */

#ifdef CONFIG_IIS2DLPC_PULSE
#if defined(CONFIG_IIS2DLPC_ONLY_SINGLE)
	.pulse_trigger = IIS2DLPC_ONLY_SINGLE,
#elif defined(CONFIG_IIS2DLPC_SINGLE_DOUBLE)
	.pulse_trigger = IIS2DLPC_BOTH_SINGLE_DOUBLE,
#endif
	.pulse_ths[0] = CONFIG_IIS2DLPC_PULSE_THSX,
	.pulse_ths[1] = CONFIG_IIS2DLPC_PULSE_THSY,
	.pulse_ths[2] = CONFIG_IIS2DLPC_PULSE_THSZ,
	.pulse_shock = CONFIG_IIS2DLPC_PULSE_SHOCK,
	.pulse_ltncy = CONFIG_IIS2DLPC_PULSE_LTNCY,
	.pulse_quiet = CONFIG_IIS2DLPC_PULSE_QUIET,
#endif /* CONFIG_IIS2DLPC_PULSE */
#endif /* CONFIG_IIS2DLPC_TRIGGER */
};

struct iis2dlpc_data iis2dlpc_data;

DEVICE_AND_API_INIT(iis2dlpc, DT_INST_LABEL(0), iis2dlpc_init,
	     &iis2dlpc_data, &iis2dlpc_cfg, POST_KERNEL,
	     CONFIG_SENSOR_INIT_PRIORITY, &iis2dlpc_driver_api);
