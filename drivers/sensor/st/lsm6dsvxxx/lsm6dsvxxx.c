/* ST Microelectronics LSM6DSVXXX family IMU sensor
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsv320x.pdf
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "lsm6dsvxxx.h"
#include "lsm6dsvxxx_rtio.h"

#if DT_HAS_COMPAT_STATUS_OKAY(st_lsm6dsv320x)
#include "lsm6dsv320x.h"
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_lsm6dsv80x)
#include "lsm6dsv80x.h"
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_ism6hg256x)
#include "ism6hg256x.h"
#endif

LOG_MODULE_REGISTER(LSM6DSVXXX, CONFIG_SENSOR_LOG_LEVEL);

static int lsm6dsvxxx_accel_config(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return cfg->chip_api->accel_fs_set(dev, sensor_ms2_to_g(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return cfg->chip_api->accel_odr_set(dev, val->val1);
	case SENSOR_ATTR_CONFIGURATION:
		return cfg->chip_api->accel_mode_set(dev, val->val1);
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsvxxx_gyro_config(const struct device *dev,
			       enum sensor_channel chan,
			       enum sensor_attribute attr,
			       const struct sensor_value *val)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return cfg->chip_api->gyro_fs_set(dev, sensor_rad_to_degrees(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return cfg->chip_api->gyro_odr_set(dev, val->val1);
	case SENSOR_ATTR_CONFIGURATION:
		return cfg->chip_api->gyro_mode_set(dev, val->val1);
	default:
		LOG_DBG("Gyro attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsvxxx_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return lsm6dsvxxx_accel_config(dev, chan, attr, val);
	case SENSOR_CHAN_GYRO_XYZ:
		return lsm6dsvxxx_gyro_config(dev, chan, attr, val);
#ifdef CONFIG_LSM6DSVXXX_STREAM
	case SENSOR_CHAN_GBIAS_XYZ:
		return lsm6dsvxxx_gbias_config(dev, chan, attr, val);
#endif /* CONFIG_LSM6DSVXXX_STREAM */
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsvxxx_accel_get_config(const struct device *dev,
				       enum sensor_channel chan,
				       enum sensor_attribute attr,
				       struct sensor_value *val)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		return cfg->chip_api->accel_mode_get(dev, &val->val1);
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsvxxx_gyro_get_config(const struct device *dev,
				      enum sensor_channel chan,
				      enum sensor_attribute attr,
				      struct sensor_value *val)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		return cfg->chip_api->gyro_mode_get(dev, &val->val1);
	default:
		LOG_DBG("Gyro attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsvxxx_attr_get(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return lsm6dsvxxx_accel_get_config(dev, chan, attr, val);
	case SENSOR_CHAN_GYRO_XYZ:
		return lsm6dsvxxx_gyro_get_config(dev, chan, attr, val);
#ifdef CONFIG_LSM6DSVXXX_STREAM
	case SENSOR_CHAN_GBIAS_XYZ:
		return lsm6dsvxxx_gbias_get_config(dev, chan, attr, val);
#endif /* CONFIG_LSM6DSVXXX_STREAM */
	default:
		LOG_WRN("attr_get() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static void lsm6dsvxxx_one_shot_complete_cb(struct rtio *ctx, const struct rtio_sqe *sqe,
					    int result, void *arg)
{
	ARG_UNUSED(result);

	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;
	int err = 0;

	err = rtio_flush_completion_queue(ctx);

	if (err) {
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static void lsm6dsvxxx_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct lsm6dsvxxx_config *config = dev->config;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	uint32_t min_buf_len = sizeof(struct lsm6dsvxxx_rtio_data);
	uint64_t cycles;
	int rc = 0;
	uint8_t *buf;
	uint32_t buf_len;
	struct lsm6dsvxxx_rtio_data *edata;
	struct lsm6dsvxxx_data *data = dev->data;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		return;
	}

	edata = (struct lsm6dsvxxx_rtio_data *)buf;

	edata->has_accel = 0;
	edata->has_temp = 0;

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata->header.cfg = config;
	edata->header.is_fifo = false;
	edata->header.accel_fs = data->accel_fs;
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	for (int i = 0; i < num_channels; i++) {
		switch (channels[i].chan_type) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			edata->has_accel = 1;

			uint8_t xl_addr = lsm6dsvxxx_bus_reg(data->bus_type, data->out_xl);
			struct rtio_regs outx_regs;
			struct rtio_regs_list xl_regs_list[] = {
				{
					xl_addr,
					(uint8_t *)edata->accel,
					6,
				},
			};

			outx_regs.rtio_regs_list = xl_regs_list;
			outx_regs.rtio_regs_num = ARRAY_SIZE(xl_regs_list);

			/*
			 * Prepare rtio enabled bus to read LSM6DSVXXX_OUTX_L_A register
			 * where accelerometer data is available.
			 * Then lsm6dsvxxx_one_shot_complete_cb callback will be invoked.
			 *
			 * STMEMSC API equivalent code:
			 *
			 *   uint8_t accel_raw[6];
			 *
			 *   lsm6dsvxxx_acceleration_raw_get(&dev_ctx, accel_raw);
			 */
			rtio_read_regs_async(data->rtio_ctx, data->iodev, data->bus_type,
					     &outx_regs, iodev_sqe, dev,
					     lsm6dsvxxx_one_shot_complete_cb);
			break;

#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
		case SENSOR_CHAN_DIE_TEMP:
			edata->has_temp = 1;

			uint8_t t_addr = lsm6dsvxxx_bus_reg(data->bus_type, data->out_tp);
			struct rtio_regs outt_regs;
			struct rtio_regs_list t_regs_list[] = {
				{
					t_addr,
					(uint8_t *)&edata->temp,
					2,
				},
			};

			outt_regs.rtio_regs_list = t_regs_list;
			outt_regs.rtio_regs_num = ARRAY_SIZE(t_regs_list);

			/*
			 * Prepare rtio enabled bus to read LSM6DSVXX0X_OUT_TEMP_L register
			 * where temperature data is available.
			 * Then lsm6dsvxxx_one_shot_complete_cb callback will be invoked.
			 *
			 * STMEMSC API equivalent code:
			 *
			 *   int16_t val;
			 *
			 *   lsm6dsvxxx_temperature_raw_get(&dev_ctx, &val);
			 */
			rtio_read_regs_async(data->rtio_ctx, data->iodev, data->bus_type,
					     &outt_regs, iodev_sqe, dev,
					     lsm6dsvxxx_one_shot_complete_cb);
			break;
#endif

		default:
			continue;
		}
	}

	if (edata->has_accel == 0) {
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
	}
}

void lsm6dsvxxx_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		lsm6dsvxxx_submit_one_shot(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_LSM6DSVXXX_STREAM)) {
		lsm6dsvxxx_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

static DEVICE_API(sensor, lsm6dsvxxx_driver_api) = {
	.attr_set = lsm6dsvxxx_attr_set,
	.attr_get = lsm6dsvxxx_attr_get,
#ifdef CONFIG_SENSOR_ASYNC_API
	.get_decoder = lsm6dsvxxx_get_decoder,
	.submit = lsm6dsvxxx_submit,
#endif
};

static int lsm6dsvxxx_init(const struct device *dev)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;
	struct lsm6dsvxxx_data *data = dev->data;

	LOG_INF("Initialize device %s", dev->name);
	data->dev = dev;

	if (cfg->chip_api->init_chip(dev) < 0) {
		LOG_DBG("failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_LSM6DSVXXX_TRIGGER
	if (cfg->trig_enabled && (lsm6dsvxxx_init_interrupt(dev) < 0)) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}

#if defined(CONFIG_PM_DEVICE)
static int lsm6dsvxxx_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;

	return cfg->chip_api->pm_action(dev, action);
}
#endif /* CONFIG_PM_DEVICE */

/*
 * Device creation macro, shared by LSM6DSVXXX_DEFINE_SPI() and
 * LSM6DSVXXX_DEFINE_I2C().
 */

/* clang-format off */

#define LSM6DSVXXX_DEVICE_INIT(inst, prefix)							\
	PM_DEVICE_DT_INST_DEFINE(inst, lsm6dsvxxx_pm_action);					\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lsm6dsvxxx_init, PM_DEVICE_DT_INST_GET(inst),	\
				     &prefix##_data_##inst, &prefix##_config_##inst,		\
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,			\
				     &lsm6dsvxxx_driver_api);

#ifdef CONFIG_LSM6DSVXXX_TRIGGER
#define LSM6DSVXXX_CFG_IRQ(inst)					\
	.trig_enabled = true,						\
	.int1_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, { 0 }),	\
	.int2_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, { 0 }),	\
	.drdy_pulsed = DT_INST_PROP(inst, drdy_pulsed),                 \
	.drdy_pin = DT_INST_PROP(inst, drdy_pin)
#else
#define LSM6DSVXXX_CFG_IRQ(inst)
#endif /* CONFIG_LSM6DSVXXX_TRIGGER */

#define LSM6DSVXXX_CONFIG_COMMON(inst, prefix)					\
	.chip_api = &prefix##_chip_api,						\
	.accel_bit_shift = prefix##_accel_bit_shift,				\
	.accel_scaler = prefix##_accel_scaler,					\
	.accel_odr = DT_INST_PROP(inst, accel_odr),				\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, accel_hg_odr),			\
		(.accel_hg_odr = DT_INST_PROP(inst, accel_hg_odr),))		\
	.accel_range = DT_INST_ENUM_IDX(inst, accel_range),			\
	.gyro_odr = DT_INST_PROP(inst, gyro_odr),				\
	.gyro_range = DT_INST_PROP(inst, gyro_range),				\
										\
	IF_ENABLED(CONFIG_LSM6DSVXXX_STREAM,					\
		   (.fifo_wtm = DT_INST_PROP(inst, fifo_watermark),		\
		    .accel_batch  = DT_INST_PROP(inst, accel_fifo_batch_rate),	\
		    .gyro_batch  = DT_INST_PROP(inst, gyro_fifo_batch_rate),	\
		    .sflp_odr  = DT_INST_PROP(inst, sflp_odr),			\
		    .sflp_fifo_en  = DT_INST_PROP(inst, sflp_fifo_enable),	\
		    .temp_batch  = DT_INST_PROP(inst, temp_fifo_batch_rate),))	\
										\
	IF_ENABLED(UTIL_OR(DT_INST_NODE_HAS_PROP(inst, int1_gpios),		\
			   DT_INST_NODE_HAS_PROP(inst, int2_gpios)),		\
		   (LSM6DSVXXX_CFG_IRQ(inst)))

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#define LSM6DSVXXX_SPI_OP  (SPI_WORD_SET(8) |				\
			 SPI_OP_MODE_MASTER |				\
			 SPI_MODE_CPOL |				\
			 SPI_MODE_CPHA)					\

#define LSM6DSVXXX_SPI_RTIO_DEFINE(inst, prefix)			\
	SPI_DT_IODEV_DEFINE(prefix##_iodev_##inst,			\
		DT_DRV_INST(inst), LSM6DSVXXX_SPI_OP);			\
	RTIO_DEFINE(prefix##_rtio_ctx_##inst, 8, 8);

#define LSM6DSVXXX_CONFIG_SPI(inst, prefix)				\
	{								\
		STMEMSC_CTX_SPI(&prefix##_config_##inst.stmemsc_cfg),	\
		.stmemsc_cfg = {					\
			.spi = SPI_DT_SPEC_INST_GET(inst,		\
					   LSM6DSVXXX_SPI_OP),		\
		},							\
									\
		LSM6DSVXXX_CONFIG_COMMON(inst, prefix)			\
	}

#define LSM6DSVXXX_DEFINE_SPI(inst, prefix)				\
	IF_ENABLED(CONFIG_SPI_RTIO,					\
		   (LSM6DSVXXX_SPI_RTIO_DEFINE(inst, prefix)));		\
	static struct lsm6dsvxxx_data prefix##_data_##inst =	{	\
		IF_ENABLED(CONFIG_SPI_RTIO,				\
			(.rtio_ctx = &prefix##_rtio_ctx_##inst,		\
			 .iodev = &prefix##_iodev_##inst,		\
			 .bus_type = RTIO_BUS_SPI,))			\
	};								\
	static const struct lsm6dsvxxx_config prefix##_config_##inst =	\
		LSM6DSVXXX_CONFIG_SPI(inst, prefix);

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LSM6DSVXXX_I2C_RTIO_DEFINE(inst, prefix)			\
	I2C_DT_IODEV_DEFINE(prefix##_iodev_##inst, DT_DRV_INST(inst));	\
	RTIO_DEFINE(prefix##_rtio_ctx_##inst, 8, 8);

#define LSM6DSVXXX_CONFIG_I2C(inst, prefix)				\
	{								\
		STMEMSC_CTX_I2C(&prefix##_config_##inst.stmemsc_cfg),	\
		.stmemsc_cfg = {					\
			.i2c = I2C_DT_SPEC_INST_GET(inst),		\
		},							\
		LSM6DSVXXX_CONFIG_COMMON(inst, prefix)			\
	}

#define LSM6DSVXXX_DEFINE_I2C(inst, prefix)				\
	IF_ENABLED(CONFIG_I2C_RTIO,					\
		   (LSM6DSVXXX_I2C_RTIO_DEFINE(inst, prefix)));		\
	static struct lsm6dsvxxx_data prefix##_data_##inst =	{	\
		IF_ENABLED(CONFIG_I2C_RTIO,				\
			(.rtio_ctx = &prefix##_rtio_ctx_##inst,		\
			 .iodev = &prefix##_iodev_##inst,		\
			 .bus_type = RTIO_BUS_I2C,))			\
	};								\
	static const struct lsm6dsvxxx_config prefix##_config_##inst =	\
		LSM6DSVXXX_CONFIG_I2C(inst, prefix);

/*
 * Instantiation macros used when a device is on an I3C bus.
 */

#define LSM6DSVXXX_I3C_RTIO_DEFINE(inst, prefix)				\
	I3C_DT_IODEV_DEFINE(prefix##_i3c_iodev_##inst, DT_DRV_INST(inst));	\
	RTIO_DEFINE(prefix##_rtio_ctx_##inst, 8, 8);

#define LSM6DSVXXX_CONFIG_I3C(inst, prefix)						\
	{										\
		STMEMSC_CTX_I3C(&prefix##_config_##inst.stmemsc_cfg),			\
		.stmemsc_cfg = {							\
			.i3c = &prefix##_data_##inst.i3c_dev,				\
		},									\
		.i3c.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),				\
		.i3c.dev_id = I3C_DEVICE_ID_DT_INST(inst),				\
		IF_ENABLED(CONFIG_LSM6DSVXXX_TRIGGER,					\
			  (.int_en_i3c = DT_INST_PROP(inst, int_en_i3c),		\
			   .bus_act_sel = DT_INST_ENUM_IDX(inst, bus_act_sel_us),))	\
		LSM6DSVXXX_CONFIG_COMMON(inst, prefix)					\
	}

#define LSM6DSVXXX_DEFINE_I3C(inst, prefix)				\
	IF_ENABLED(CONFIG_I3C_RTIO,					\
		   (LSM6DSVXXX_I3C_RTIO_DEFINE(inst, prefix)));		\
	static struct lsm6dsvxxx_data prefix##_data_##inst = {		\
		IF_ENABLED(CONFIG_I3C_RTIO,				\
			(.rtio_ctx = &prefix##_rtio_ctx_##inst,		\
			 .iodev = &prefix##_i3c_iodev_##inst,		\
			 .bus_type = RTIO_BUS_I3C,))			\
	};								\
	static const struct lsm6dsvxxx_config prefix##_config_##inst =	\
		LSM6DSVXXX_CONFIG_I3C(inst, prefix);

#define LSM6DSVXXX_DEFINE_I3C_OR_I2C(inst, prefix)			\
	COND_CODE_0(DT_INST_PROP_BY_IDX(inst, reg, 1),			\
		    (LSM6DSVXXX_DEFINE_I2C(inst, prefix)),		\
		    (LSM6DSVXXX_DEFINE_I3C(inst, prefix)))

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define LSM6DSVXXX_DEFINE(inst, prefix)						\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),					\
		(LSM6DSVXXX_DEFINE_SPI(inst, prefix)),				\
		(COND_CODE_1(DT_INST_ON_BUS(inst, i3c),				\
			     (LSM6DSVXXX_DEFINE_I3C_OR_I2C(inst, prefix)),	\
			     (LSM6DSVXXX_DEFINE_I2C(inst, prefix)))));		\
	LSM6DSVXXX_DEVICE_INIT(inst, prefix)

/* clang-format on */

#define DT_DRV_COMPAT st_lsm6dsv320x
DT_INST_FOREACH_STATUS_OKAY_VARGS(LSM6DSVXXX_DEFINE, DT_DRV_COMPAT)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT st_lsm6dsv80x
DT_INST_FOREACH_STATUS_OKAY_VARGS(LSM6DSVXXX_DEFINE, DT_DRV_COMPAT)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT st_ism6hg256x
DT_INST_FOREACH_STATUS_OKAY_VARGS(LSM6DSVXXX_DEFINE, DT_DRV_COMPAT)
#undef DT_DRV_COMPAT
