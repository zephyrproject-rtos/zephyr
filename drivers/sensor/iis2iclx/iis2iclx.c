/* ST Microelectronics IIS2ICLX 2-axis accelerometer sensor driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2iclx.pdf
 */

#define DT_DRV_COMPAT st_iis2iclx

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "iis2iclx.h"

LOG_MODULE_REGISTER(IIS2ICLX, CONFIG_SENSOR_LOG_LEVEL);

static const uint16_t iis2iclx_odr_map[] = {0, 12, 26, 52, 104, 208, 416, 833,
					1660, 3330, 6660};

#if defined(IIS2ICLX_ACCEL_ODR_RUNTIME)
static int iis2iclx_freq_to_odr_val(uint16_t freq)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(iis2iclx_odr_map); i++) {
		if (freq == iis2iclx_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

static int iis2iclx_odr_to_freq_val(uint16_t odr)
{
	/* for valid index, return value from map */
	if (odr < ARRAY_SIZE(iis2iclx_odr_map)) {
		return iis2iclx_odr_map[odr];
	}

	/* invalid index, return last entry */
	return iis2iclx_odr_map[ARRAY_SIZE(iis2iclx_odr_map) - 1];
}

#ifdef IIS2ICLX_ACCEL_FS_RUNTIME
static const uint16_t iis2iclx_accel_fs_map[] = {500, 3000, 1000, 2000};
static const uint16_t iis2iclx_accel_fs_sens[] = {1, 8, 2, 4};

static int iis2iclx_accel_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(iis2iclx_accel_fs_map); i++) {
		if (range == iis2iclx_accel_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

static inline int iis2iclx_reboot(const struct device *dev)
{
	struct iis2iclx_data *data = dev->data;

	if (iis2iclx_boot_set(data->ctx, 1) < 0) {
		return -EIO;
	}

	/* Wait sensor turn-on time as per datasheet */
	k_msleep(35);

	return 0;
}

static int iis2iclx_accel_set_fs_raw(const struct device *dev, uint8_t fs)
{
	struct iis2iclx_data *data = dev->data;

	if (iis2iclx_xl_full_scale_set(data->ctx, fs) < 0) {
		return -EIO;
	}

	data->accel_fs = fs;

	return 0;
}

static int iis2iclx_accel_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct iis2iclx_data *data = dev->data;

	if (iis2iclx_xl_data_rate_set(data->ctx, odr) < 0) {
		return -EIO;
	}

	data->accel_freq = iis2iclx_odr_to_freq_val(odr);

	return 0;
}

#ifdef IIS2ICLX_ACCEL_ODR_RUNTIME
static int iis2iclx_accel_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	odr = iis2iclx_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	if (iis2iclx_accel_set_odr_raw(dev, odr) < 0) {
		LOG_ERR("failed to set accelerometer sampling rate");
		return -EIO;
	}

	return 0;
}
#endif

#ifdef IIS2ICLX_ACCEL_FS_RUNTIME
static int iis2iclx_accel_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct iis2iclx_data *data = dev->data;

	fs = iis2iclx_accel_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (iis2iclx_accel_set_fs_raw(dev, fs) < 0) {
		LOG_ERR("failed to set accelerometer full-scale");
		return -EIO;
	}

	data->acc_gain = (iis2iclx_accel_fs_sens[fs] * GAIN_UNIT_XL);
	return 0;
}
#endif

static int iis2iclx_accel_config(const struct device *dev,
				   enum sensor_channel chan,
				   enum sensor_attribute attr,
				   const struct sensor_value *val)
{
	switch (attr) {
#ifdef IIS2ICLX_ACCEL_FS_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return iis2iclx_accel_range_set(dev, sensor_ms2_to_g(val));
#endif
#ifdef IIS2ICLX_ACCEL_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return iis2iclx_accel_odr_set(dev, val->val1);
#endif
	default:
		LOG_ERR("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int iis2iclx_attr_set(const struct device *dev,
			       enum sensor_channel chan,
			       enum sensor_attribute attr,
			       const struct sensor_value *val)
{
#if defined(CONFIG_IIS2ICLX_SENSORHUB)
	struct iis2iclx_data *data = dev->data;
#endif /* CONFIG_IIS2ICLX_SENSORHUB */

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return iis2iclx_accel_config(dev, chan, attr, val);
#if defined(CONFIG_IIS2ICLX_SENSORHUB)
	case SENSOR_CHAN_MAGN_XYZ:
	case SENSOR_CHAN_PRESS:
	case SENSOR_CHAN_HUMIDITY:
		if (!data->shub_inited) {
			LOG_ERR("shub not inited.");
			return -ENOTSUP;
		}

		return iis2iclx_shub_config(dev, chan, attr, val);
#endif /* CONFIG_IIS2ICLX_SENSORHUB */
	default:
		LOG_ERR("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int iis2iclx_sample_fetch_accel(const struct device *dev)
{
	struct iis2iclx_data *data = dev->data;
	int16_t buf[2];

	if (iis2iclx_acceleration_raw_get(data->ctx, buf) < 0) {
		LOG_ERR("Failed to read sample");
		return -EIO;
	}

	data->acc[0] = sys_le16_to_cpu(buf[0]);
	data->acc[1] = sys_le16_to_cpu(buf[1]);

	return 0;
}

#if defined(CONFIG_IIS2ICLX_ENABLE_TEMP)
static int iis2iclx_sample_fetch_temp(const struct device *dev)
{
	struct iis2iclx_data *data = dev->data;
	int16_t buf;

	if (iis2iclx_temperature_raw_get(data->ctx, &buf) < 0) {
		LOG_ERR("Failed to read sample");
		return -EIO;
	}

	data->temp_sample = sys_le16_to_cpu(buf);

	return 0;
}
#endif

#if defined(CONFIG_IIS2ICLX_SENSORHUB)
static int iis2iclx_sample_fetch_shub(const struct device *dev)
{
	struct iis2iclx_data *data = dev->data;

	if (!data->shub_inited) {
		LOG_WRN("attr_set() shub not inited.");
		return 0;
	}

	if (iis2iclx_shub_fetch_external_devs(dev) < 0) {
		LOG_ERR("failed to read ext shub devices");
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_IIS2ICLX_SENSORHUB */

static int iis2iclx_sample_fetch(const struct device *dev,
				   enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		iis2iclx_sample_fetch_accel(dev);

#if defined(CONFIG_IIS2ICLX_SENSORHUB)
		iis2iclx_sample_fetch_shub(dev);
#endif
		break;
#if defined(CONFIG_IIS2ICLX_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		iis2iclx_sample_fetch_temp(dev);
		break;
#endif
	case SENSOR_CHAN_ALL:
		iis2iclx_sample_fetch_accel(dev);
#if defined(CONFIG_IIS2ICLX_ENABLE_TEMP)
		iis2iclx_sample_fetch_temp(dev);
#endif
#if defined(CONFIG_IIS2ICLX_SENSORHUB)
		iis2iclx_sample_fetch_shub(dev);
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void iis2iclx_accel_convert(struct sensor_value *val, int raw_val,
					    uint32_t sensitivity)
{
	int64_t dval;

	/* Sensitivity is exposed in ug/LSB */
	/* Convert to m/s^2 */
	dval = (int64_t)(raw_val) * sensitivity * SENSOR_G_DOUBLE;
	val->val1 = (int32_t)(dval / 1000000);
	val->val2 = (int32_t)(dval % 1000000);

}

static inline int iis2iclx_accel_get_channel(enum sensor_channel chan,
					       struct sensor_value *val,
					       struct iis2iclx_data *data,
					       uint32_t sensitivity)
{
	uint8_t i;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		iis2iclx_accel_convert(val, data->acc[0], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		iis2iclx_accel_convert(val, data->acc[1], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		for (i = 0; i < 2; i++) {
			iis2iclx_accel_convert(val++, data->acc[i], sensitivity);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int iis2iclx_accel_channel_get(enum sensor_channel chan,
					struct sensor_value *val,
					struct iis2iclx_data *data)
{
	return iis2iclx_accel_get_channel(chan, val, data, data->acc_gain);
}

#if defined(CONFIG_IIS2ICLX_ENABLE_TEMP)
static void iis2iclx_temp_channel_get(struct sensor_value *val,
					  struct iis2iclx_data *data)
{
	/* val = temp_sample / 256 + 25 */
	val->val1 = data->temp_sample / 256 + 25;
	val->val2 = (data->temp_sample % 256) * (1000000 / 256);
}
#endif

#if defined(CONFIG_IIS2ICLX_SENSORHUB)
static inline void iis2iclx_magn_convert(struct sensor_value *val, int raw_val,
					   uint16_t sensitivity)
{
	double dval;

	/* Sensitivity is exposed in mgauss/LSB */
	dval = (double)(raw_val * sensitivity);
	val->val1 = (int32_t)dval / 1000000;
	val->val2 = (int32_t)dval % 1000000;
}

static inline int iis2iclx_magn_get_channel(enum sensor_channel chan,
					      struct sensor_value *val,
					      struct iis2iclx_data *data)
{
	int16_t sample[3];
	int idx;

	idx = iis2iclx_shub_get_idx(SENSOR_CHAN_MAGN_XYZ);
	if (idx < 0) {
		LOG_ERR("external magn not supported");
		return -ENOTSUP;
	}


	sample[0] = (int16_t)(data->ext_data[idx][0] |
			    (data->ext_data[idx][1] << 8));
	sample[1] = (int16_t)(data->ext_data[idx][2] |
			    (data->ext_data[idx][3] << 8));
	sample[2] = (int16_t)(data->ext_data[idx][4] |
			    (data->ext_data[idx][5] << 8));

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		iis2iclx_magn_convert(val, sample[0], data->magn_gain);
		break;
	case SENSOR_CHAN_MAGN_Y:
		iis2iclx_magn_convert(val, sample[1], data->magn_gain);
		break;
	case SENSOR_CHAN_MAGN_Z:
		iis2iclx_magn_convert(val, sample[2], data->magn_gain);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		iis2iclx_magn_convert(val, sample[0], data->magn_gain);
		iis2iclx_magn_convert(val + 1, sample[1], data->magn_gain);
		iis2iclx_magn_convert(val + 2, sample[2], data->magn_gain);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void iis2iclx_hum_convert(struct sensor_value *val,
					  struct iis2iclx_data *data)
{
	float rh;
	int16_t raw_val;
	struct hts221_data *ht = &data->hts221;
	int idx;

	idx = iis2iclx_shub_get_idx(SENSOR_CHAN_HUMIDITY);
	if (idx < 0) {
		LOG_DBG("external press/temp not supported");
		return;
	}

	raw_val = ((int16_t)(data->ext_data[idx][0] |
			   (data->ext_data[idx][1] << 8)));

	/* find relative humidty by linear interpolation */
	rh = (ht->y1 - ht->y0) * raw_val + ht->x1 * ht->y0 - ht->x0 * ht->y1;
	rh /= (ht->x1 - ht->x0);

	/* convert humidity to integer and fractional part */
	val->val1 = rh;
	val->val2 = rh * 1000000;
}

static inline void iis2iclx_press_convert(struct sensor_value *val,
					    struct iis2iclx_data *data)
{
	int32_t raw_val;
	int idx;

	idx = iis2iclx_shub_get_idx(SENSOR_CHAN_PRESS);
	if (idx < 0) {
		LOG_DBG("external press/temp not supported");
		return;
	}

	raw_val = (int32_t)(data->ext_data[idx][0] |
			  (data->ext_data[idx][1] << 8) |
			  (data->ext_data[idx][2] << 16));

	/* Pressure sensitivity is 4096 LSB/hPa */
	/* Convert raw_val to val in kPa */
	val->val1 = (raw_val >> 12) / 10;
	val->val2 = (raw_val >> 12) % 10 * 100000 +
		(((int32_t)((raw_val) & 0x0FFF) * 100000L) >> 12);
}

static inline void iis2iclx_temp_convert(struct sensor_value *val,
					   struct iis2iclx_data *data)
{
	int16_t raw_val;
	int idx;

	idx = iis2iclx_shub_get_idx(SENSOR_CHAN_PRESS);
	if (idx < 0) {
		LOG_DBG("external press/temp not supported");
		return;
	}

	raw_val = (int16_t)(data->ext_data[idx][3] |
			  (data->ext_data[idx][4] << 8));

	/* Temperature sensitivity is 100 LSB/deg C */
	val->val1 = raw_val / 100;
	val->val2 = (int32_t)raw_val % 100 * (10000);
}
#endif

static int iis2iclx_channel_get(const struct device *dev,
				  enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct iis2iclx_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		iis2iclx_accel_channel_get(chan, val, data);
		break;
#if defined(CONFIG_IIS2ICLX_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		iis2iclx_temp_channel_get(val, data);
		break;
#endif
#if defined(CONFIG_IIS2ICLX_SENSORHUB)
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		if (!data->shub_inited) {
			LOG_ERR("shub not inited.");
			return -ENOTSUP;
		}

		iis2iclx_magn_get_channel(chan, val, data);
		break;

	case SENSOR_CHAN_HUMIDITY:
		if (!data->shub_inited) {
			LOG_ERR("shub not inited.");
			return -ENOTSUP;
		}

		iis2iclx_hum_convert(val, data);
		break;

	case SENSOR_CHAN_PRESS:
		if (!data->shub_inited) {
			LOG_ERR("shub not inited.");
			return -ENOTSUP;
		}

		iis2iclx_press_convert(val, data);
		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		if (!data->shub_inited) {
			LOG_ERR("attr_set() shub not inited.");
			return -ENOTSUP;
		}

		iis2iclx_temp_convert(val, data);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api iis2iclx_driver_api = {
	.attr_set = iis2iclx_attr_set,
#if CONFIG_IIS2ICLX_TRIGGER
	.trigger_set = iis2iclx_trigger_set,
#endif
	.sample_fetch = iis2iclx_sample_fetch,
	.channel_get = iis2iclx_channel_get,
};

static int iis2iclx_init_chip(const struct device *dev)
{
	struct iis2iclx_data *iis2iclx = dev->data;
	uint8_t chip_id;

	iis2iclx->dev = dev;

	if (iis2iclx_device_id_get(iis2iclx->ctx, &chip_id) < 0) {
		LOG_ERR("Failed reading chip id");
		return -EIO;
	}

	LOG_INF("chip id 0x%x", chip_id);

	if (chip_id != IIS2ICLX_ID) {
		LOG_ERR("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	/* reset device */
	if (iis2iclx_reset_set(iis2iclx->ctx, 1) < 0) {
		return -EIO;
	}

	k_usleep(100);

	if (iis2iclx_accel_set_fs_raw(dev,
				     IIS2ICLX_DEFAULT_ACCEL_FULLSCALE) < 0) {
		LOG_ERR("failed to set accelerometer full-scale");
		return -EIO;
	}
	iis2iclx->acc_gain = IIS2ICLX_DEFAULT_ACCEL_SENSITIVITY;

	iis2iclx->accel_freq = iis2iclx_odr_to_freq_val(CONFIG_IIS2ICLX_ACCEL_ODR);
	if (iis2iclx_accel_set_odr_raw(dev, CONFIG_IIS2ICLX_ACCEL_ODR) < 0) {
		LOG_ERR("failed to set accelerometer sampling rate");
		return -EIO;
	}

	/* Set FIFO bypass mode */
	if (iis2iclx_fifo_mode_set(iis2iclx->ctx, IIS2ICLX_BYPASS_MODE) < 0) {
		LOG_ERR("failed to set FIFO mode");
		return -EIO;
	}

	if (iis2iclx_block_data_update_set(iis2iclx->ctx, 1) < 0) {
		LOG_ERR("failed to set BDU mode");
		return -EIO;
	}

	return 0;
}

static int iis2iclx_init(const struct device *dev)
{
	const struct iis2iclx_config * const config = dev->config;
	struct iis2iclx_data *data = dev->data;

	data->bus = device_get_binding(config->bus_name);
	if (!data->bus) {
		LOG_DBG("master not found: %s",
			    config->bus_name);
		return -EINVAL;
	}

	config->bus_init(dev);

	if (iis2iclx_init_chip(dev) < 0) {
		LOG_ERR("failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_IIS2ICLX_TRIGGER
	if (iis2iclx_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

#ifdef CONFIG_IIS2ICLX_SENSORHUB
	if (iis2iclx_shub_init(dev) < 0) {
		LOG_INF("failed to initialize external chips");
		data->shub_inited = false;
	}
	data->shub_inited = true;
#endif

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "IIS2ICLX driver enabled without any devices"
#endif

/*
 * Device creation macro, shared by IIS2ICLX_DEFINE_SPI() and
 * IIS2ICLX_DEFINE_I2C().
 */

#define IIS2ICLX_DEVICE_INIT(inst)					\
	DEVICE_DT_INST_DEFINE(inst,					\
			    iis2iclx_init,				\
			    device_pm_control_nop,			\
			    &iis2iclx_data_##inst,			\
			    &iis2iclx_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &iis2iclx_driver_api);

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#define IIS2ICLX_HAS_CS(inst) DT_INST_SPI_DEV_HAS_CS_GPIOS(inst)

#define IIS2ICLX_DATA_SPI_CS(inst)					\
	{ .cs_ctrl = {							\
		.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(inst),		\
		.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(inst),	\
		},							\
	}

#define IIS2ICLX_DATA_SPI(inst)						\
	COND_CODE_1(IIS2ICLX_HAS_CS(inst),				\
		    (IIS2ICLX_DATA_SPI_CS(inst)),			\
		    ({}))

#define IIS2ICLX_SPI_CS_PTR(inst)					\
	COND_CODE_1(IIS2ICLX_HAS_CS(inst),				\
		    (&(iis2iclx_data_##inst.cs_ctrl)),			\
		    (NULL))

#define IIS2ICLX_SPI_CS_LABEL(inst)					\
	COND_CODE_1(IIS2ICLX_HAS_CS(inst),				\
		    (DT_INST_SPI_DEV_CS_GPIOS_LABEL(inst)), (NULL))

#define IIS2ICLX_SPI_CFG(inst)						\
	(&(struct iis2iclx_spi_cfg) {					\
		.spi_conf = {						\
			.frequency =					\
				DT_INST_PROP(inst, spi_max_frequency),	\
			.operation = (SPI_WORD_SET(8) |			\
				      SPI_OP_MODE_MASTER |		\
				      SPI_MODE_CPOL |			\
				      SPI_MODE_CPHA),			\
			.slave = DT_INST_REG_ADDR(inst),		\
			.cs = IIS2ICLX_SPI_CS_PTR(inst),		\
		},							\
		.cs_gpios_label = IIS2ICLX_SPI_CS_LABEL(inst),		\
	})


#ifdef CONFIG_IIS2ICLX_TRIGGER
#define IIS2ICLX_CFG_IRQ(inst) \
		.irq_dev_name = DT_INST_GPIO_LABEL(inst, drdy_gpios),	\
		.irq_pin = DT_INST_GPIO_PIN(inst, drdy_gpios),		\
		.irq_flags = DT_INST_GPIO_FLAGS(inst, drdy_gpios),	\
		.int_pin = DT_INST_PROP(inst, int_pin)
#else
#define IIS2ICLX_CFG_IRQ(inst)
#endif /* CONFIG_IIS2ICLX_TRIGGER */

#define IIS2ICLX_CONFIG_SPI(inst)					\
	{								\
		.bus_name = DT_INST_BUS_LABEL(inst),			\
		.bus_init = iis2iclx_spi_init,				\
		.bus_cfg = { .spi_cfg = IIS2ICLX_SPI_CFG(inst)	},	\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, drdy_gpios),	\
			(IIS2ICLX_CFG_IRQ(inst)), ())			\
	}

#define IIS2ICLX_DEFINE_SPI(inst)					\
	static struct iis2iclx_data iis2iclx_data_##inst =		\
		IIS2ICLX_DATA_SPI(inst);				\
	static const struct iis2iclx_config iis2iclx_config_##inst =	\
		IIS2ICLX_CONFIG_SPI(inst);				\
	IIS2ICLX_DEVICE_INIT(inst)

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define IIS2ICLX_CONFIG_I2C(inst)					\
	{								\
		.bus_name = DT_INST_BUS_LABEL(inst),			\
		.bus_init = iis2iclx_i2c_init,				\
		.bus_cfg = { .i2c_slv_addr = DT_INST_REG_ADDR(inst), },	\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, drdy_gpios),	\
			(IIS2ICLX_CFG_IRQ(inst)), ())			\
	}

#define IIS2ICLX_DEFINE_I2C(inst)					\
	static struct iis2iclx_data iis2iclx_data_##inst;		\
	static const struct iis2iclx_config iis2iclx_config_##inst =	\
		IIS2ICLX_CONFIG_I2C(inst);				\
	IIS2ICLX_DEVICE_INIT(inst)
/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define IIS2ICLX_DEFINE(inst)						\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),				\
		    (IIS2ICLX_DEFINE_SPI(inst)),			\
		    (IIS2ICLX_DEFINE_I2C(inst)))

DT_INST_FOREACH_STATUS_OKAY(IIS2ICLX_DEFINE)
