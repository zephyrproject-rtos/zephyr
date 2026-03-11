/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm45686

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/rtio/rtio.h>

#if defined(CONFIG_SENSOR_ASYNC_API)
#include <zephyr/rtio/work.h>
#endif /* CONFIG_SENSOR_ASYNC_API */

#include "icm45686.h"
#include "icm45686_reg.h"
#include "icm45686_bus.h"
#include "icm45686_decoder.h"
#include "icm45686_trigger.h"
#include "icm45686_stream.h"

#include <zephyr/drivers/sensor/tdk_apex.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM45686, CONFIG_SENSOR_LOG_LEVEL);

static inline int inv_io_hal_read_reg(void *context, uint8_t reg, uint8_t *rbuffer, uint32_t rlen)
{
	const struct device *dev = context;
	struct icm45686_data *data = dev->data;

	return icm45686_reg_read_rtio(&data->bus, reg | REG_READ_BIT, rbuffer, rlen);
}

static inline int inv_io_hal_write_reg(void *context, uint8_t reg, const uint8_t *wbuffer,
				       uint32_t wlen)
{
	const struct device *dev = context;
	struct icm45686_data *data = dev->data;

	return icm45686_reg_write_rtio(&data->bus, reg, wbuffer, wlen);
}

void inv_sleep_us(uint32_t us)
{
	k_usleep(us);
}

static int icm45686_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int err;
	struct icm45686_data *data = dev->data;
	struct icm45686_encoded_data *edata = &data->edata;

#ifdef CONFIG_TDK_APEX
	if ((enum sensor_channel_tdk_apex)chan == SENSOR_CHAN_APEX_MOTION) {
		err = icm45686_apex_fetch_from_dmp(dev);
		return err;
	}
#endif
	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	err = icm456xx_get_register_data(&data->driver,
					 (inv_imu_sensor_data_t *)edata->payload.buf);

	LOG_HEXDUMP_DBG(edata->payload.buf, sizeof(edata->payload.buf), "ICM45686 data");

	return err;
}

static int icm45686_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
#ifdef CONFIG_TDK_APEX
	struct icm45686_data *drv_data = dev->data;

	__ASSERT_NO_MSG(val != NULL);

	if ((enum sensor_channel_tdk_apex)chan == SENSOR_CHAN_APEX_MOTION) {
		if (attr == SENSOR_ATTR_CONFIGURATION) {
			if (val->val1 == TDK_APEX_PEDOMETER) {
				icm45686_apex_enable(&drv_data->driver);
				icm45686_apex_enable_pedometer(dev, &drv_data->driver);
			} else if (val->val1 == TDK_APEX_TILT) {
				icm45686_apex_enable(&drv_data->driver);
				icm45686_apex_enable_tilt(&drv_data->driver);
			} else if (val->val1 == TDK_APEX_SMD) {
				icm45686_apex_enable(&drv_data->driver);
				icm45686_apex_enable_smd(&drv_data->driver);
			} else if (val->val1 == TDK_APEX_WOM) {
				icm45686_apex_enable_wom(&drv_data->driver);
			} else {
				LOG_ERR("Not supported ATTR value");
			}
		} else {
			LOG_ERR("Not supported ATTR");
			return -EINVAL;
		}
	} else {
		LOG_ERR("Unsupported channel");
		(void)drv_data;
		return -EINVAL;
	}
#endif
	return 0;
}

static int icm45686_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct icm45686_config *cfg = dev->config;
	int res = 0;

	__ASSERT_NO_MSG(val != NULL);

	switch (chan) {
	case SENSOR_CHAN_APEX_MOTION:
		if (attr == SENSOR_ATTR_CONFIGURATION) {
			val->val1 = cfg->apex;
		}
		break;
	default:
		LOG_ERR("Unsupported channel");
		res = -EINVAL;
		break;
	}

	return res;
}

static int icm45686_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct icm45686_data *data = dev->data;
#ifdef CONFIG_TDK_APEX
	const struct icm45686_config *cfg = dev->config;
#endif

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		icm45686_accel_ms(data->edata.header.accel_fs, data->edata.payload.accel.x, false,
				  &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm45686_accel_ms(data->edata.header.accel_fs, data->edata.payload.accel.y, false,
				  &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm45686_accel_ms(data->edata.header.accel_fs, data->edata.payload.accel.z, false,
				  &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm45686_gyro_rads(data->edata.header.gyro_fs, data->edata.payload.gyro.x, false,
				   &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm45686_gyro_rads(data->edata.header.gyro_fs, data->edata.payload.gyro.y, false,
				   &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm45686_gyro_rads(data->edata.header.gyro_fs, data->edata.payload.gyro.z, false,
				   &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm45686_temp_c(data->edata.payload.temp, &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		icm45686_accel_ms(data->edata.header.accel_fs, data->edata.payload.accel.x, false,
				  &val[0].val1, &val[0].val2);
		icm45686_accel_ms(data->edata.header.accel_fs, data->edata.payload.accel.y, false,
				  &val[1].val1, &val[1].val2);
		icm45686_accel_ms(data->edata.header.accel_fs, data->edata.payload.accel.z, false,
				  &val[2].val1, &val[2].val2);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm45686_gyro_rads(data->edata.header.gyro_fs, data->edata.payload.gyro.x, false,
				   &val->val1, &val->val2);
		icm45686_gyro_rads(data->edata.header.gyro_fs, data->edata.payload.gyro.y, false,
				   &val[1].val1, &val[1].val2);
		icm45686_gyro_rads(data->edata.header.gyro_fs, data->edata.payload.gyro.z, false,
				   &val[2].val1, &val[2].val2);
		break;
#ifdef CONFIG_TDK_APEX
	case SENSOR_CHAN_APEX_MOTION:
		if (cfg->apex == TDK_APEX_PEDOMETER) {
			val[0].val1 = data->pedometer_cnt;
			val[1].val1 = data->pedometer_activity;
			icm45686_apex_pedometer_cadence_convert(&val[2], data->pedometer_cadence,
								data->dmp_odr_hz);
		} else if (cfg->apex == TDK_APEX_WOM) {
			val[0].val1 = (data->apex_status & ICM45686_APEX_STATUS_MASK_WOM_X) ? 1 : 0;
			val[1].val1 = (data->apex_status & ICM45686_APEX_STATUS_MASK_WOM_Y) ? 1 : 0;
			val[2].val1 = (data->apex_status & ICM45686_APEX_STATUS_MASK_WOM_Z) ? 1 : 0;
		} else if ((cfg->apex == TDK_APEX_TILT) || (cfg->apex == TDK_APEX_SMD)) {
			val[0].val1 = data->apex_status;
		}
#endif

	default:
		return -ENOTSUP;
	}

	return 0;
}

#if defined(CONFIG_SENSOR_ASYNC_API)

static void icm45686_complete_result(struct rtio *ctx, const struct rtio_sqe *sqe, int result,
				     void *arg)
{
	ARG_UNUSED(result);

	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;
	struct rtio_cqe *cqe;
	int err = 0;

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			err = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	if (err) {
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}

	LOG_DBG("One-shot fetch completed");
}

static inline void icm45686_submit_one_shot(const struct device *dev,
					    struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	uint32_t min_buf_len = sizeof(struct icm45686_encoded_data);
	int err;
	uint8_t *buf;
	uint32_t buf_len;
	struct icm45686_encoded_data *edata;
	struct icm45686_data *data = dev->data;
	struct rtio_sqe *read_sqe;

	err = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (err != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	edata = (struct icm45686_encoded_data *)buf;

	err = icm45686_encode(dev, channels, num_channels, buf);
	if (err != 0) {
		LOG_ERR("Failed to encode sensor data");
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	err = icm45686_prep_reg_read_rtio_async(&data->bus, ACCEL_DATA_X1_UI | REG_READ_BIT,
						edata->payload.buf, sizeof(edata->payload.buf),
						&read_sqe);
	if (err < 0) {
		LOG_ERR("Fail to prepare read: %d", err);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}
	read_sqe->flags |= RTIO_SQE_CHAINED;

	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(data->bus.rtio.ctx);

	if (!complete_sqe) {
		LOG_ERR("Failed to acquire complete read-sqe");
		rtio_sqe_drop_all(data->bus.rtio.ctx);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}
	rtio_sqe_prep_callback_no_cqe(complete_sqe, icm45686_complete_result, (void *)dev,
				      iodev_sqe);

	rtio_submit(data->bus.rtio.ctx, 0);
}

static void icm45686_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		icm45686_submit_one_shot(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_ICM45686_STREAM)) {
		icm45686_stream_submit(dev, iodev_sqe);
	} else {
		LOG_ERR("Streaming not supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

#endif /* CONFIG_SENSOR_ASYNC_API */

static DEVICE_API(sensor, icm45686_driver_api) = {
	.sample_fetch = icm45686_sample_fetch,
	.channel_get = icm45686_channel_get,
	.attr_set = icm45686_attr_set,
	.attr_get = icm45686_attr_get,
#if defined(CONFIG_ICM45686_TRIGGER)
	.trigger_set = icm45686_trigger_set,
#endif /* CONFIG_ICM45686_TRIGGER */
#if defined(CONFIG_SENSOR_ASYNC_API)
	.get_decoder = icm45686_get_decoder,
	.submit = icm45686_submit,
#endif /* CONFIG_SENSOR_ASYNC_API */
};

static int icm45686_init(const struct device *dev)
{
	struct icm45686_data *data = dev->data;
	const struct icm45686_config *cfg = dev->config;
	uint8_t read_val = 0;
	int err;

	/* Initialize serial interface and device */
	data->driver.transport.context = (struct device *)dev;
	data->driver.transport.read_reg = inv_io_hal_read_reg;
	data->driver.transport.write_reg = inv_io_hal_write_reg;
	data->driver.transport.serif_type = data->bus.rtio.type;
	data->driver.transport.sleep_us = inv_sleep_us;

#if CONFIG_SPI_RTIO
	if (data->bus.rtio.type == ICM45686_BUS_SPI && !spi_is_ready_iodev(data->bus.rtio.iodev)) {
		LOG_ERR("Bus is not ready");
		return -ENODEV;
	}
#endif
#if CONFIG_I2C_RTIO
	if (data->bus.rtio.type == ICM45686_BUS_I2C && !i2c_is_ready_iodev(data->bus.rtio.iodev)) {
		LOG_ERR("Bus is not ready");
		return -ENODEV;
	}
#endif
	/* Set Slew-rate to 10-ns typical, to allow proper SPI readouts */
	if (data->bus.rtio.type == ICM45686_BUS_SPI) {
		drive_config0_t drive_config0;

		drive_config0.pads_spi_slew = DRIVE_CONFIG0_PADS_SPI_SLEW_TYP_10NS;
		err = icm456xx_write_reg(&data->driver, DRIVE_CONFIG0, 1,
					 (uint8_t *)&drive_config0);
		inv_sleep_us(2); /* Takes effect 1.5 us after the register is programmed */
	}

	/** Soft-reset sensor to restore config to defaults,
	 * unless it's already handled by I3C initialization.
	 */
	if (data->bus.rtio.type != ICM45686_BUS_I3C) {
		err = icm456xx_soft_reset(&data->driver);
		if (err < 0) {
			LOG_ERR("Soft reset failed err %d", err);
			return err;
		}
	}

	/* Confirm ID Value matches */
	err = icm456xx_get_who_am_i(&data->driver, &read_val);
	if (err < 0) {
		LOG_ERR("ID read failed: %d", err);
		return err;
	}

	/* Sensor Configuration */
	err = icm456xx_set_accel_mode(&data->driver, cfg->settings.accel.pwr_mode);
	if (err < 0) {
		LOG_ERR("Failed to set accel mode");
		return err;
	}

	err = icm456xx_set_gyro_mode(&data->driver, cfg->settings.gyro.pwr_mode);
	if (err < 0) {
		LOG_ERR("Failed to set gyro mode");
		return err;
	}

	err = icm456xx_set_accel_frequency(&data->driver, cfg->settings.accel.odr);
	if (err < 0) {
		LOG_ERR("Failed to set Accel frequency: %d", err);
		return err;
	}

	err = icm456xx_set_accel_fsr(&data->driver, cfg->settings.accel.fs);
	if (err < 0) {
		LOG_ERR("Failed to set Accel fsr: %d", err);
		return err;
	}

	err = icm456xx_set_gyro_frequency(&data->driver, cfg->settings.gyro.odr);
	if (err < 0) {
		LOG_ERR("Failed to set Gyro frequency: %d", err);
		return err;
	}

	err = icm456xx_set_gyro_fsr(&data->driver, cfg->settings.gyro.fs);
	if (err < 0) {
		LOG_ERR("Failed to set Gyro fsr: %d", err);
		return err;
	}

	/** Write Low-pass filter settings through indirect register access */
	err = icm456xx_set_gyro_ln_bw(&data->driver, cfg->settings.gyro.lpf);
	if (err < 0) {
		LOG_ERR("Failed to set Gyro BW settings: %d", err);
		return err;
	}

	/** Wait before indirect register write is made effective
	 * before proceeding with next one.
	 */
	k_sleep(K_MSEC(1));

	icm456xx_set_accel_ln_bw(&data->driver, cfg->settings.accel.lpf);
	if (err < 0) {
		LOG_ERR("Failed to set Accel BW settings: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_ICM45686_TRIGGER)) {
		err = icm45686_trigger_init(dev);
		if (err) {
			LOG_ERR("Failed to initialize triggers: %d", err);
			return err;
		}
	} else if (IS_ENABLED(CONFIG_ICM45686_STREAM)) {
		err = icm45686_stream_init(dev);
		if (err) {
			LOG_ERR("Failed to initialize streaming: %d", err);
			return err;
		}
	}

#ifdef CONFIG_TDK_APEX
	/* Initialize APEX */
	err = icm456xx_edmp_disable(&data->driver);
	if (err < 0) {
		LOG_ERR("APEX Disable failed");
		return err;
	}

	k_sleep(K_MSEC(100));

	inv_imu_int_state_t int_config;

	memset(&int_config, INV_IMU_DISABLE, sizeof(int_config));
	int_config.INV_EDMP_EVENT = INV_IMU_ENABLE;
	err = icm456xx_set_config_int(&data->driver, INV_IMU_INT1, &int_config);

	err = icm456xx_edmp_init_apex(&data->driver);
	if (err < 0) {
		LOG_ERR("APEX Initialization failed");
		return err;
	}
#endif
	LOG_DBG("Init OK");

	return 0;
}

#define ICM45686_VALID_ACCEL_ODR(pwr_mode, odr) \
	((pwr_mode == ICM45686_DT_ACCEL_LP && odr >= ICM45686_DT_ACCEL_ODR_400) || \
	 (pwr_mode == ICM45686_DT_ACCEL_LN && odr <= ICM45686_DT_ACCEL_ODR_12_5) || \
	 (pwr_mode == ICM45686_DT_ACCEL_OFF))

#define ICM45686_VALID_GYRO_ODR(pwr_mode, odr) \
	((pwr_mode == ICM45686_DT_GYRO_LP && odr >= ICM45686_DT_GYRO_ODR_400) || \
	 (pwr_mode == ICM45686_DT_GYRO_LN && odr <= ICM45686_DT_GYRO_ODR_12_5) || \
	 (pwr_mode == ICM45686_DT_GYRO_OFF))

#define ICM45686_INIT(inst) \
 \
	RTIO_DEFINE(icm45686_rtio_ctx_##inst, 32, 32); \
 \
	COND_CODE_1(DT_INST_ON_BUS(inst, i3c), \
		    (I3C_DT_IODEV_DEFINE(icm45686_bus_##inst, \
					 DT_DRV_INST(inst))), \
	(COND_CODE_1(DT_INST_ON_BUS(inst, i2c), \
		    (I2C_DT_IODEV_DEFINE(icm45686_bus_##inst, \
					 DT_DRV_INST(inst))), \
		    ()))); \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi), \
			(SPI_DT_IODEV_DEFINE(icm45686_bus_##inst, \
				DT_DRV_INST(inst), \
				SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)), \
		    ()); \
 \
	static const struct icm45686_config icm45686_cfg_##inst = { \
		.settings = { \
			.accel = { \
				.pwr_mode = DT_INST_PROP(inst, accel_pwr_mode), \
				.fs = DT_INST_PROP(inst, accel_fs), \
				.odr = DT_INST_PROP(inst, accel_odr), \
				.lpf = DT_INST_PROP_OR(inst, accel_lpf, 0), \
			}, \
			.gyro = { \
				.pwr_mode = DT_INST_PROP(inst, gyro_pwr_mode), \
				.fs = DT_INST_PROP(inst, gyro_fs), \
				.odr = DT_INST_PROP(inst, gyro_odr), \
				.lpf = DT_INST_PROP_OR(inst, gyro_lpf, 0), \
			}, \
			.fifo_watermark = DT_INST_PROP_OR(inst, fifo_watermark, 0), \
			.fifo_watermark_equals = DT_INST_PROP(inst, fifo_watermark_equals), \
		}, \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}), \
		.apex = DT_INST_ENUM_IDX(inst, apex), \
	}; \
	static struct icm45686_data icm45686_data_##inst = { \
		.edata.header = { \
			.accel_fs = DT_INST_PROP(inst, accel_fs), \
			.gyro_fs = DT_INST_PROP(inst, gyro_fs), \
		}, \
		.bus.rtio = { \
			.iodev = &icm45686_bus_##inst, \
			.ctx = &icm45686_rtio_ctx_##inst, \
			COND_CODE_1(DT_INST_ON_BUS(inst, i3c), \
				(.type = ICM45686_BUS_I3C, \
				 .i3c.id = I3C_DEVICE_ID_DT_INST(inst),), \
			(COND_CODE_1(DT_INST_ON_BUS(inst, i2c), \
				(.type = ICM45686_BUS_I2C), ()))) \
			COND_CODE_1(DT_INST_ON_BUS(inst, spi), \
				(.type = ICM45686_BUS_SPI), ()) \
		}, \
	}; \
 \
	/* Build-time settings verification: Inform the user of invalid settings at build time */ \
	BUILD_ASSERT(ICM45686_VALID_ACCEL_ODR(DT_INST_PROP(inst, accel_pwr_mode), \
					      DT_INST_PROP(inst, accel_odr)), \
		     "Invalid accel ODR setting. Please check supported ODRs for LP and LN"); \
	BUILD_ASSERT(ICM45686_VALID_GYRO_ODR(DT_INST_PROP(inst, gyro_pwr_mode), \
					     DT_INST_PROP(inst, gyro_odr)), \
		     "Invalid gyro ODR setting. Please check supported ODRs for LP and LN"); \
		\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm45686_init, \
				     NULL, \
				     &icm45686_data_##inst, \
				     &icm45686_cfg_##inst,\
				     POST_KERNEL, \
				     CONFIG_SENSOR_INIT_PRIORITY, \
				     &icm45686_driver_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT invensense_icm45605
DT_INST_FOREACH_STATUS_OKAY(ICM45686_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT invensense_icm45605s
DT_INST_FOREACH_STATUS_OKAY(ICM45686_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT invensense_icm45686
DT_INST_FOREACH_STATUS_OKAY(ICM45686_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT invensense_icm45686s
DT_INST_FOREACH_STATUS_OKAY(ICM45686_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT invensense_icm45688p
DT_INST_FOREACH_STATUS_OKAY(ICM45686_INIT)
