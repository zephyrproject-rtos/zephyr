/* ST Microelectronics IIS3DWB 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis3dwb.pdf
 */

#define DT_DRV_COMPAT st_iis3dwb

#include <init.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <logging/log.h>
#include <drivers/sensor.h>

#include "iis3dwb.h"

LOG_MODULE_REGISTER(IIS3DWB, CONFIG_SENSOR_LOG_LEVEL);

/**
 * iis3dwb_set_range_raw - set full scale range for acc
 * @dev: Pointer to instance of struct device
 * @range: Full scale range (2, 4, 8 and 16 G)
 */
static int iis3dwb_set_range_raw(const struct device *dev, uint8_t fs)
{
	int err;
	struct iis3dwb_data *iis3dwb = dev->data;
	const struct iis3dwb_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	switch (fs) {
	case 0:
		iis3dwb->gain = IIS3DWB_FS_2G_GAIN;
		break;
	case 1:
		iis3dwb->gain = IIS3DWB_FS_16G_GAIN;
		break;
	case 2:
		iis3dwb->gain = IIS3DWB_FS_4G_GAIN;
		break;
	case 3:
		iis3dwb->gain = IIS3DWB_FS_8G_GAIN;
		break;
	default:
		return -EINVAL;
	}

	err = iis3dwb_xl_full_scale_set(ctx, fs);

	return err;
}

/**
 * iis3dwb_set_odr_raw - set new sampling frequency
 * @dev: Pointer to instance of struct device
 * @odr: Output data rate
 */
static int iis3dwb_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct iis3dwb_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	return iis3dwb_xl_data_rate_set(ctx, odr);
}

static inline void iis3dwb_convert(struct sensor_value *val, int raw_val,
				    float gain)
{
	int64_t dval;

	/* Gain is in ug/LSB */
	/* Convert to m/s^2 */
	dval = ((int64_t)raw_val * gain * SENSOR_G) / 1000000LL;
	val->val1 = dval / 1000000LL;
	val->val2 = dval % 1000000LL;
}

static inline void iis3dwb_channel_get_acc(const struct device *dev,
					     enum sensor_channel chan,
					     struct sensor_value *val)
{
	int i;
	uint8_t ofs_start, ofs_stop;
	struct iis3dwb_data *iis3dwb = dev->data;
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
		iis3dwb_convert(pval++, iis3dwb->acc[i], iis3dwb->gain);
	}
}

static int iis3dwb_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		iis3dwb_channel_get_acc(dev, chan, val);
		return 0;
	default:
		LOG_DBG("Channel not supported");
		break;
	}

	return -ENOTSUP;
}

static const uint16_t iis3dwb_accel_fs_map[] = {2, 16, 4, 8};

static int iis3dwb_accel_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(iis3dwb_accel_fs_map); i++) {
		if (range == iis3dwb_accel_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int iis3dwb_set_range(const struct device *dev, int32_t range)
{
	int fs;

	fs = iis3dwb_accel_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (iis3dwb_set_range_raw(dev, fs) < 0) {
		LOG_ERR("failed to set accelerometer full-scale");
		return -EIO;
	}

	return 0;
}

static int iis3dwb_freq_to_odr_val(uint16_t freq)
{
	int odr;

	switch (freq) {
	case 0:
		odr = 0;
		break;
	case 26700:
		odr = 5;
		break;
	default:
		odr = -EINVAL;
		break;
	}

	return odr;
}

static int iis3dwb_set_odr(const struct device *dev, uint16_t freq)
{
	int odr;

	odr = iis3dwb_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	if (iis3dwb_set_odr_raw(dev, odr) < 0) {
		LOG_ERR("failed to set accelerometer sampling rate");
		return -EIO;
	}

	return 0;
}

static int iis3dwb_dev_config(const struct device *dev,
			       enum sensor_channel chan,
			       enum sensor_attribute attr,
			       const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return iis3dwb_set_range(dev, sensor_ms2_to_g(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return iis3dwb_set_odr(dev, val->val1);
	default:
		LOG_DBG("Acc attribute not supported");
		break;
	}

	return -ENOTSUP;
}

static int iis3dwb_attr_set(const struct device *dev,
			      enum sensor_channel chan,
			      enum sensor_attribute attr,
			      const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return iis3dwb_dev_config(dev, chan, attr, val);
	default:
		LOG_DBG("Attr not supported on %d channel", chan);
		break;
	}

	return -ENOTSUP;
}

static int iis3dwb_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct iis3dwb_data *iis3dwb = dev->data;
	const struct iis3dwb_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int16_t buf[3];

	/* fetch raw data sample */
	if (iis3dwb_acceleration_raw_get(ctx, buf) < 0) {
		LOG_DBG("Failed to fetch raw data sample");
		return -EIO;
	}

	iis3dwb->acc[0] = sys_le16_to_cpu(buf[0]);
	iis3dwb->acc[1] = sys_le16_to_cpu(buf[1]);
	iis3dwb->acc[2] = sys_le16_to_cpu(buf[2]);

	return 0;
}

static const struct sensor_driver_api iis3dwb_driver_api = {
	.attr_set = iis3dwb_attr_set,
#if CONFIG_IIS3DWB_TRIGGER
	.trigger_set = iis3dwb_trigger_set,
#endif /* CONFIG_IIS3DWB_TRIGGER */
	.sample_fetch = iis3dwb_sample_fetch,
	.channel_get = iis3dwb_channel_get,
};

static int iis3dwb_init(const struct device *dev)
{
	struct iis3dwb_data *iis3dwb = dev->data;
	const struct iis3dwb_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t wai, rst;
	uint8_t filter;

	iis3dwb->dev = dev;

	/* check chip ID */
	if (iis3dwb_device_id_get(ctx, &wai) < 0) {
		return -EIO;
	}

	if (wai != IIS3DWB_ID) {
		LOG_ERR("Invalid chip ID");
		return -EINVAL;
	}

	/* reset device */
	if (iis3dwb_reset_set(ctx, PROPERTY_ENABLE) < 0) {
		return -EIO;
	}

	do {
		k_sleep(K_USEC(1));
		iis3dwb_reset_get(ctx, &rst);
	} while (rst);

	if (iis3dwb_block_data_update_set(ctx, PROPERTY_ENABLE) < 0) {
		return -EIO;
	}

	LOG_INF("odr is %d", cfg->odr);
	if (iis3dwb_set_odr_raw(dev, cfg->odr) < 0) {
		LOG_ERR("odr init error %d", cfg->odr);
		return -EIO;
	}

	LOG_INF("filter type is %d", cfg->filt_type);
	LOG_INF("filter configuration is %d", cfg->filt_cfg);
	LOG_INF("filter number of stages %d", cfg->filt_num);

	if (cfg->filt_type) {
		filter = 0x10 | cfg->filt_cfg;
	} else {
		if (cfg->filt_num == 2) {
			filter = 0x80 | cfg->filt_cfg;
		} else {
			filter = 0x00;
		}
	}
	if (iis3dwb_xl_hp_path_on_out_set(ctx, filter) < 0) {
		LOG_ERR("filter init error %d", cfg->range);
		return -EIO;
	}

	LOG_INF("range is %d", cfg->range);
	if (iis3dwb_set_range_raw(dev, cfg->range) < 0) {
		LOG_ERR("range init error %d", cfg->range);
		return -EIO;
	}

#ifdef CONFIG_IIS3DWB_TRIGGER
	if (iis3dwb_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupts");
		return -EIO;
	}
#endif /* CONFIG_IIS3DWB_TRIGGER */

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "IIS3DWB driver enabled without any devices"
#endif

/*
 * Device creation macro, used by IIS3DWB_DEFINE_SPI()
 */

#define IIS3DWB_DEVICE_INIT(inst)					\
	DEVICE_DT_INST_DEFINE(inst,					\
			    iis3dwb_init,				\
			    NULL,					\
			    &iis3dwb_data_##inst,			\
			    &iis3dwb_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &iis3dwb_driver_api);


#ifdef CONFIG_IIS3DWB_TRIGGER
#define IIS3DWB_CFG_IRQ(inst) \
	.gpio_drdy = GPIO_DT_SPEC_INST_GET(inst, drdy_gpios),		\
	.drdy_int = DT_INST_PROP(inst, drdy_int),
#else
#define IIS3DWB_CFG_IRQ(inst)
#endif /* CONFIG_IIS3DWB_TRIGGER */

#define IIS3DWB_SPI_OPERATION (SPI_WORD_SET(8) |			\
				SPI_OP_MODE_MASTER |			\
				SPI_MODE_CPOL |				\
				SPI_MODE_CPHA)				\

/*
 * Instantiation macros used when a device is on a SPI bus.
 */
#define IIS3DWB_CONFIG_SPI(inst)					\
	{								\
		.ctx = {						\
			.read_reg =					\
			   (stmdev_read_ptr) stmemsc_spi_read,		\
			.write_reg =					\
			   (stmdev_write_ptr) stmemsc_spi_write,	\
			.handle =					\
			   (void *)&iis3dwb_config_##inst.spi,		\
		},							\
		.spi = SPI_DT_SPEC_INST_GET(inst,			\
					   IIS3DWB_SPI_OPERATION,	\
					   0),				\
		.range = DT_INST_PROP(inst, range),			\
		.odr = DT_INST_PROP(inst, odr),				\
		.filt_type = DT_INST_PROP(inst, filter_type),		\
		.filt_cfg = DT_INST_PROP(inst, filter_config),		\
		.filt_num = DT_INST_PROP(inst, filter_stages),		\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, drdy_gpios),	\
			(IIS3DWB_CFG_IRQ(inst)), ())			\
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define IIS3DWB_DEFINE(inst)						\
	static struct iis3dwb_data iis3dwb_data_##inst;			\
	static const struct iis3dwb_config iis3dwb_config_##inst =	\
		    IIS3DWB_CONFIG_SPI(inst);				\
	IIS3DWB_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(IIS3DWB_DEFINE)
