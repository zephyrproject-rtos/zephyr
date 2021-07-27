/* ST Microelectronics LIS2DS12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2ds12.pdf
 */

#define DT_DRV_COMPAT st_lis2ds12

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "lis2ds12.h"

LOG_MODULE_REGISTER(LIS2DS12, CONFIG_SENSOR_LOG_LEVEL);

static struct lis2ds12_data lis2ds12_data;

static struct lis2ds12_config lis2ds12_config = {
	.comm_master_dev_name = DT_INST_BUS_LABEL(0),
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	.bus_init = lis2ds12_spi_init,
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	.bus_init = lis2ds12_i2c_init,
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif
#ifdef CONFIG_LIS2DS12_TRIGGER
	.irq_port	= DT_INST_GPIO_LABEL(0, irq_gpios),
	.irq_pin	= DT_INST_GPIO_PIN(0, irq_gpios),
	.irq_flags	= DT_INST_GPIO_FLAGS(0, irq_gpios),
#endif
};

static int lis2ds12_set_odr(const struct device *dev, uint16_t odr)
{
	const struct lis2ds12_data *data = dev->data;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)data->ctx;
	uint8_t val;

	/* check if power off */
	if (odr == 0U) {
		return lis2ds12_xl_data_rate_set(ctx, LIS2DS12_XL_ODR_OFF);
	}

	val = LIS2DS12_HR_ODR_TO_REG(odr);
	if (val > LIS2DS12_XL_ODR_800Hz_HR) {
		LOG_ERR("ODR too high");
		return -EINVAL;
	}

	return lis2ds12_xl_data_rate_set(ctx, val);
}

static int lis2ds12_set_range(const struct device *dev, uint8_t range)
{
	int err;
	struct lis2ds12_data *data = dev->data;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)data->ctx;

	switch (range) {
	default:
	case 2U:
		err = lis2ds12_xl_full_scale_set(ctx, LIS2DS12_2g);
		data->gain = lis2ds12_from_fs2g_to_mg(1);
		break;
	case 4U:
		err = lis2ds12_xl_full_scale_set(ctx, LIS2DS12_4g);
		data->gain = lis2ds12_from_fs4g_to_mg(1);
		break;
	case 8U:
		err = lis2ds12_xl_full_scale_set(ctx, LIS2DS12_8g);
		data->gain = lis2ds12_from_fs8g_to_mg(1);
		break;
	case 16U:
		err = lis2ds12_xl_full_scale_set(ctx, LIS2DS12_16g);
		data->gain = lis2ds12_from_fs16g_to_mg(1);
		break;
	}

	return err;
}

static int lis2ds12_accel_config(const struct device *dev,
				 enum sensor_channel chan,
				 enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return lis2ds12_set_range(dev, sensor_ms2_to_g(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lis2ds12_set_odr(dev, val->val1);
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lis2ds12_attr_set(const struct device *dev,
			     enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return lis2ds12_accel_config(dev, chan, attr, val);
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int lis2ds12_sample_fetch_accel(const struct device *dev)
{
	struct lis2ds12_data *data = dev->data;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)data->ctx;
	int16_t buf[3];

	/* fetch raw data sample */
	if (lis2ds12_acceleration_raw_get(ctx, buf) < 0) {
		LOG_ERR("Failed to fetch raw data sample");
		return -EIO;
	}

	data->sample_x = sys_le16_to_cpu(buf[0]);
	data->sample_y = sys_le16_to_cpu(buf[1]);
	data->sample_z = sys_le16_to_cpu(buf[2]);

	return 0;
}

static int lis2ds12_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		lis2ds12_sample_fetch_accel(dev);
		break;
#if defined(CONFIG_LIS2DS12_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		/* ToDo:
		lis2ds12_sample_fetch_temp(dev)
		*/
		break;
#endif
	case SENSOR_CHAN_ALL:
		lis2ds12_sample_fetch_accel(dev);
#if defined(CONFIG_LIS2DS12_ENABLE_TEMP)
		/* ToDo:
		lis2ds12_sample_fetch_temp(dev)
		*/
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void lis2ds12_convert(struct sensor_value *val, int raw_val,
				    float gain)
{
	int64_t dval;

	/* Gain is in mg/LSB */
	/* Convert to m/s^2 */
	dval = ((int64_t)raw_val * gain * SENSOR_G) / 1000;
	val->val1 = dval / 1000000LL;
	val->val2 = dval % 1000000LL;
}

static inline int lis2ds12_get_channel(enum sensor_channel chan,
					     struct sensor_value *val,
					     struct lis2ds12_data *data,
					     float gain)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		lis2ds12_convert(val, data->sample_x, gain);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		lis2ds12_convert(val, data->sample_y, gain);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		lis2ds12_convert(val, data->sample_z, gain);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		lis2ds12_convert(val, data->sample_x, gain);
		lis2ds12_convert(val + 1, data->sample_y, gain);
		lis2ds12_convert(val + 2, data->sample_z, gain);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lis2ds12_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct lis2ds12_data *data = dev->data;

	return lis2ds12_get_channel(chan, val, data, data->gain);
}

static const struct sensor_driver_api lis2ds12_api_funcs = {
	.attr_set = lis2ds12_attr_set,
#if defined(CONFIG_LIS2DS12_TRIGGER)
	.trigger_set = lis2ds12_trigger_set,
#endif
	.sample_fetch = lis2ds12_sample_fetch,
	.channel_get = lis2ds12_channel_get,
};

static int lis2ds12_init(const struct device *dev)
{
	const struct lis2ds12_config * const config = dev->config;
	struct lis2ds12_data *data = dev->data;
	stmdev_ctx_t *ctx;
	uint8_t chip_id;
	int ret;

	data->comm_master = device_get_binding(config->comm_master_dev_name);
	if (!data->comm_master) {
		LOG_ERR("master not found: %s",
			    config->comm_master_dev_name);
		return -EINVAL;
	}

	config->bus_init(dev);

	ctx = (stmdev_ctx_t *)data->ctx;
	/* check chip ID */
	ret = lis2ds12_device_id_get(ctx, &chip_id);
	if (ret < 0) {
		LOG_ERR("Not able to read dev id");
		return ret;
	}

	if (chip_id != LIS2DS12_ID) {
		LOG_ERR("Invalid chip ID 0x%02x", chip_id);
		return -EINVAL;
	}

	/* reset device */
	ret = lis2ds12_reset_set(ctx, PROPERTY_ENABLE);
	if (ret < 0) {
		return ret;
	}

	k_busy_wait(100);

	LOG_DBG("chip id 0x%x", chip_id);

#ifdef CONFIG_LIS2DS12_TRIGGER
	if (lis2ds12_trigger_init(dev) < 0) {
		LOG_ERR("Failed to initialize triggers.");
		return -EIO;
	}
#endif

	/* set sensor default odr */
	ret = lis2ds12_set_odr(dev, 12);
	if (ret < 0) {
		LOG_ERR("odr init error (12.5 Hz)");
		return ret;
	}

	/* set sensor default scale */
	ret = lis2ds12_set_range(dev, CONFIG_LIS2DS12_FS);
	if (ret < 0) {
		LOG_ERR("range init error %d", CONFIG_LIS2DS12_FS);
		return ret;
	}

	return 0;
}

DEVICE_DT_INST_DEFINE(0, lis2ds12_init, NULL,
		    &lis2ds12_data, &lis2ds12_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &lis2ds12_api_funcs);
