/* ST Microelectronics IIS3DWB accelerometer senor
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis3dwb.pdf
 */

#define DT_DRV_COMPAT st_iis3dwb

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "iis3dwb.h"
#include "iis3dwb_rtio.h"

LOG_MODULE_REGISTER(IIS3DWB, CONFIG_SENSOR_LOG_LEVEL);

static int32_t iis3dwb_set_range_raw(const struct device *dev, uint8_t range)
{
	struct iis3dwb_data *data = dev->data;
	const struct iis3dwb_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	data->range = range;
	return iis3dwb_xl_full_scale_set(ctx, range);
}

static int32_t iis3dwb_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct iis3dwb_data *data = dev->data;
	const struct iis3dwb_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	data->odr = odr;
	return iis3dwb_xl_data_rate_set(ctx, odr);
}

static int iis3dwb_odr_set(const struct device *dev, const struct sensor_value *val)
{
	iis3dwb_odr_xl_t odr;

	odr = IIS3DWB_XL_ODR_OFF;

	if (val->val1) {
		if (val->val1 > 26) {
			LOG_ERR("%s: odr %d Hz not supported", dev->name, val->val1);
			return -EINVAL;
		}

		odr = IIS3DWB_XL_ODR_26k7Hz;
	}

	if (iis3dwb_set_odr_raw(dev, odr)) {
		LOG_ERR("failed to set sampling rate");
		return -EIO;
	}

	return 0;
}

static int iis3dwb_set_fs(const struct device *dev, int32_t fs)
{
	int ret;
	uint8_t range;

	if (fs <= 2) {
		range = IIS3DWB_DT_FS_2G;
	} else if (fs <= 4) {
		range = IIS3DWB_DT_FS_4G;
	} else if (fs <= 8) {
		range = IIS3DWB_DT_FS_8G;
	} else if (fs <= 16) {
		range = IIS3DWB_DT_FS_16G;
	} else {
		LOG_ERR("fs [%d] not supported.", fs);
		return -EINVAL;
	}

	ret = iis3dwb_set_range_raw(dev, range);
	if (ret < 0) {
		LOG_ERR("%s: range init error %d", dev->name, range);
		return ret;
	}

	LOG_DBG("%s: set fs to %d g", dev->name, fs);
	return ret;
}

static int iis3dwb_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return iis3dwb_set_fs(dev, sensor_ms2_to_g(val));

	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return iis3dwb_odr_set(dev, val);
	default:
		LOG_ERR("operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static void iis3dwb_one_shot_complete_cb(struct rtio *ctx, const struct rtio_sqe *sqe, int result,
					 void *arg)
{
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;
	int err = 0;

	ARG_UNUSED(result);

	err = rtio_flush_completion_queue(ctx);

	if (err) {
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static void iis3dwb_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	uint32_t min_buf_len = sizeof(struct iis3dwb_rtio_data);
	uint64_t cycles;
	int rc = 0;
	uint8_t *buf;
	uint32_t buf_len;
	struct iis3dwb_rtio_data *edata;
	struct iis3dwb_data *data = dev->data;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	edata = (struct iis3dwb_rtio_data *)buf;

	edata->has_accel = 0;
	edata->has_temp = 0;

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata->header.is_fifo = false;
	edata->header.range = data->range;
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	for (int i = 0; i < num_channels; i++) {
		switch (channels[i].chan_type) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			edata->has_accel = 1;

			struct rtio_regs outx_regs;
			struct rtio_regs_list xl_regs_list[] = {
				{
					0x80 | IIS3DWB_OUTX_L_A, /* SPI read transaction */
					(uint8_t *)edata->accel,
					6,
				},
			};

			outx_regs.rtio_regs_list = xl_regs_list;
			outx_regs.rtio_regs_num = ARRAY_SIZE(xl_regs_list);

			/*
			 * Prepare rtio enabled bus to read IIS3DWB_OUTX_L_A register
			 * where accelerometer data is available.
			 * Then iis3dwb_one_shot_complete_cb callback will be invoked.
			 *
			 * STMEMSC API equivalent code:
			 *
			 *   uint8_t accel_raw[6];
			 *
			 *   iis3dwb_acceleration_raw_get(&dev_ctx, accel_raw);
			 */
			rtio_read_regs_async(data->rtio_ctx, data->iodev, RTIO_BUS_SPI, &outx_regs,
					     iodev_sqe, dev, iis3dwb_one_shot_complete_cb);
			break;

#if defined(CONFIG_IIS3DWB_ENABLE_TEMP)
		case SENSOR_CHAN_DIE_TEMP:
			edata->has_temp = 1;

			struct rtio_regs outt_regs;
			struct rtio_regs_list t_regs_list[] = {
				{
					0x80 | IIS3DWB_OUT_TEMP_L, /* SPI read transaction */
					(uint8_t *)&edata->temp,
					2,
				},
			};

			outt_regs.rtio_regs_list = t_regs_list;
			outt_regs.rtio_regs_num = ARRAY_SIZE(t_regs_list);

			/*
			 * Prepare rtio enabled bus to read IIS3DWB_OUT_TEMP_L register
			 * where temperature data is available.
			 * Then iis3dwb_one_shot_complete_cb callback will be invoked.
			 *
			 * STMEMSC API equivalent code:
			 *
			 *   int16_t val;
			 *
			 *   iis3dwb_temperature_raw_get(&dev_ctx, &val);
			 */
			rtio_read_regs_async(data->rtio_ctx, data->iodev, RTIO_BUS_SPI, &outt_regs,
					     iodev_sqe, dev, iis3dwb_one_shot_complete_cb);
			break;
#endif

		default:
			continue;
		}
	}

	if (edata->has_accel == 0 && edata->has_temp == 0) {
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
	}
}

void iis3dwb_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		iis3dwb_submit_one_shot(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_IIS3DWB_STREAM)) {
		iis3dwb_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

static DEVICE_API(sensor, iis3dwb_driver_api) = {
	.attr_set = iis3dwb_attr_set,
	.get_decoder = iis3dwb_get_decoder,
	.submit = iis3dwb_submit,
};

static int iis3dwb_init_chip(const struct device *dev)
{
	const struct iis3dwb_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t chip_id, rst;

	if (iis3dwb_device_id_get(ctx, &chip_id) < 0) {
		LOG_ERR("Failed reading chip id");
		return -EIO;
	}

	if (chip_id != IIS3DWB_ID) {
		LOG_ERR("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	/*
	 *  Restore default configuration
	 */
	iis3dwb_reset_set(ctx, PROPERTY_ENABLE);
	WAIT_FOR((iis3dwb_reset_get(ctx, &rst) == 0) && !rst, 100 * USEC_PER_MSEC, k_msleep(10));

	/* Enable Block Data Update */
	iis3dwb_block_data_update_set(ctx, PROPERTY_ENABLE);

	return 0;
}

static int iis3dwb_init(const struct device *dev)
{
	const struct iis3dwb_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	if (iis3dwb_init_chip(dev) < 0) {
		LOG_ERR("Failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_IIS3DWB_TRIGGER
	if (cfg->trig_enabled && iis3dwb_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	/* set sensor default scale (used to convert sample values) */
	LOG_DBG("%s: range is %d", dev->name, cfg->range);
	ret = iis3dwb_set_range_raw(dev, cfg->range);
	if (ret < 0) {
		LOG_ERR("%s: range init error %d", dev->name, cfg->range);
		return ret;
	}

	/* set sensor filter setting */
	LOG_DBG("%s: filter is %d", dev->name, cfg->filter);
	ret = iis3dwb_xl_filt_path_on_out_set(ctx, cfg->filter);
	if (ret < 0) {
		LOG_ERR("%s: filter init error %d", dev->name, cfg->filter);
		return ret;
	}

	/* set sensor default odr */
	LOG_DBG("%s: odr: %d", dev->name, cfg->odr);
	ret = iis3dwb_set_odr_raw(dev, cfg->odr);
	if (ret < 0) {
		LOG_ERR("%s: odr init error", dev->name);
		return ret;
	}

	return 0;
}

#define IIS3DWB_SPI_OPERATION								\
	(SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA)

#define IIS3DWB_SPI_RTIO_DEFINE(inst)					\
	SPI_DT_IODEV_DEFINE(iis3dwb_iodev_##inst,			\
		DT_DRV_INST(inst), IIS3DWB_SPI_OPERATION);		\
	RTIO_DEFINE(iis3dwb_rtio_ctx_##inst, 8, 8);

#ifdef CONFIG_IIS3DWB_TRIGGER
#define IIS3DWB_CFG_IRQ(inst)							\
	.trig_enabled = true,							\
	.int1_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0}),		\
	.int2_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, {0}),		\
	.drdy_pulsed = DT_INST_PROP(inst, drdy_pulsed),				\
	.drdy_pin = DT_INST_PROP(inst, drdy_pin),
#else
#define IIS3DWB_CFG_IRQ(inst)
#endif /* CONFIG_IIS3DWB_TRIGGER */

#define IIS3DWB_CONFIG(inst)								\
	{										\
		STMEMSC_CTX_SPI(&iis3dwb_config_##inst.stmemsc_cfg),			\
		.stmemsc_cfg = {							\
			.spi = SPI_DT_SPEC_INST_GET(inst, IIS3DWB_SPI_OPERATION),	\
		},									\
											\
		.range = DT_INST_PROP(inst, range),					\
		.filter = DT_INST_PROP(inst, filter),					\
		.odr = DT_INST_PROP(inst, odr),						\
											\
		IF_ENABLED(CONFIG_IIS3DWB_STREAM,					\
			(.fifo_wtm = DT_INST_PROP(inst, fifo_watermark),		\
			.accel_batch  = DT_INST_PROP(inst, accel_fifo_batch_rate),	\
			.temp_batch  = DT_INST_PROP(inst, temp_fifo_batch_rate),	\
			.ts_batch  = DT_INST_PROP(inst, timestamp_fifo_batch_rate),))	\
											\
		IF_ENABLED(UTIL_OR(DT_INST_NODE_HAS_PROP(inst, int1_gpios),		\
				   DT_INST_NODE_HAS_PROP(inst, int2_gpios)),		\
			   (IIS3DWB_CFG_IRQ(inst)))					\
	}

#define IIS3DWB_DEFINE(inst)									\
	IIS3DWB_SPI_RTIO_DEFINE(inst);								\
	static struct iis3dwb_data iis3dwb_data_##inst =					\
				{								\
				.rtio_ctx = &iis3dwb_rtio_ctx_##inst,				\
				.iodev = &iis3dwb_iodev_##inst,					\
				};								\
	static const struct iis3dwb_config iis3dwb_config_##inst =				\
		IIS3DWB_CONFIG(inst);								\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, iis3dwb_init, NULL,					\
			      &iis3dwb_data_##inst, &iis3dwb_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &iis3dwb_driver_api);		\

DT_INST_FOREACH_STATUS_OKAY(IIS3DWB_DEFINE)
