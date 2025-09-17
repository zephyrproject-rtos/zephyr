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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM45686, CONFIG_SENSOR_LOG_LEVEL);

static inline int reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	return icm45686_bus_write(dev, reg, &val, 1);
}

static inline int reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	return icm45686_bus_read(dev, reg, val, 1);
}

static int icm45686_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	int err;
	struct icm45686_data *data = dev->data;
	struct icm45686_encoded_data *edata = &data->edata;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	err = icm45686_bus_read(dev,
				REG_ACCEL_DATA_X1_UI,
				edata->payload.buf,
				sizeof(edata->payload.buf));

	LOG_HEXDUMP_DBG(edata->payload.buf,
			sizeof(edata->payload.buf),
			"ICM45686 data");

	return err;
}

static int icm45686_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct icm45686_data *data = dev->data;

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
	default:
		return -ENOTSUP;
	}

	return 0;
}

#if defined(CONFIG_SENSOR_ASYNC_API)

static void icm45686_complete_result(struct rtio *ctx,
				     const struct rtio_sqe *sqe,
				     int result,
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

	struct rtio_sqe *write_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *read_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(data->rtio.ctx);

	if (!write_sqe || !read_sqe || !complete_sqe) {
		LOG_ERR("Failed to acquire RTIO SQEs");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	uint8_t val = REG_ACCEL_DATA_X1_UI | REG_READ_BIT;

	rtio_sqe_prep_tiny_write(write_sqe,
				 data->rtio.iodev,
				 RTIO_PRIO_HIGH,
				 &val,
				 1,
				NULL);
	write_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(read_sqe,
			   data->rtio.iodev,
			   RTIO_PRIO_HIGH,
			   edata->payload.buf,
			   sizeof(edata->payload.buf),
			   NULL);
	if (data->rtio.type == ICM45686_BUS_I2C) {
		read_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	} else if (data->rtio.type == ICM45686_BUS_I3C) {
		read_sqe->iodev_flags |= RTIO_IODEV_I3C_STOP | RTIO_IODEV_I3C_RESTART;
	}
	read_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(complete_sqe,
				      icm45686_complete_result,
				      (void *)dev,
				      iodev_sqe);

	rtio_submit(data->rtio.ctx, 0);
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
	uint8_t val;
	int err;

#if CONFIG_SPI_RTIO
	if ((data->rtio.type == ICM45686_BUS_SPI) && !spi_is_ready_iodev(data->rtio.iodev)) {
		LOG_ERR("Bus is not ready");
		return -ENODEV;
	}
#endif
#if CONFIG_I2C_RTIO
	if ((data->rtio.type == ICM45686_BUS_I2C) && !i2c_is_ready_iodev(data->rtio.iodev)) {
		LOG_ERR("Bus is not ready");
		return -ENODEV;
	}
#endif

	/** Soft-reset sensor to restore config to defaults,
	 * unless it's already handled by I3C initialization.
	 */
	if (data->rtio.type != ICM45686_BUS_I3C) {
		err = reg_write(dev, REG_MISC2, REG_MISC2_SOFT_RST(1));
		if (err) {
			LOG_ERR("Failed to write soft-reset: %d", err);
			return err;
		}
		/* Wait for soft-reset to take effect */
		k_sleep(K_MSEC(1));

		/* A complete soft-reset clears the bit */
		err = reg_read(dev, REG_MISC2, &read_val);
		if (err) {
			LOG_ERR("Failed to read soft-reset: %d", err);
			return err;
		}
		if ((read_val & REG_MISC2_SOFT_RST(1)) != 0) {
			LOG_ERR("Soft-reset command failed");
			return -EIO;
		}
	}

	/* Set Slew-rate to 10-ns typical, to allow proper SPI readouts */

	err = reg_write(dev, REG_DRIVE_CONFIG0, REG_DRIVE_CONFIG0_SPI_SLEW(2));
	if (err) {
		LOG_ERR("Failed to write slew-rate: %d", err);
		return err;
	}
	err = reg_write(dev, REG_DRIVE_CONFIG1, REG_DRIVE_CONFIG1_I3C_SLEW(3));
	if (err) {
		LOG_ERR("Failed to write slew-rate: %d", err);
		return err;
	}
	/* Wait for register to take effect */
	k_sleep(K_USEC(2));

	/* Confirm ID Value matches */
	err = reg_read(dev, REG_WHO_AM_I, &read_val);
	if (err) {
		LOG_ERR("Failed to read WHO_AM_I: %d", err);
		return err;
	}
	if (read_val != WHO_AM_I_ICM45686) {
		LOG_ERR("Unexpected WHO_AM_I value - expected: 0x%02x, actual: 0x%02x",
			WHO_AM_I_ICM45686, read_val);
		return -EIO;
	}

	/* Sensor Configuration */

	val = REG_PWR_MGMT0_ACCEL_MODE(cfg->settings.accel.pwr_mode) |
	      REG_PWR_MGMT0_GYRO_MODE(cfg->settings.gyro.pwr_mode);
	err = reg_write(dev, REG_PWR_MGMT0, val);
	if (err) {
		LOG_ERR("Failed to write Power settings: %d", err);
		return err;
	}

	val = REG_ACCEL_CONFIG0_ODR(cfg->settings.accel.odr) |
	      REG_ACCEL_CONFIG0_FS(cfg->settings.accel.fs);
	err = reg_write(dev, REG_ACCEL_CONFIG0, val);
	if (err) {
		LOG_ERR("Failed to write Accel settings: %d", err);
		return err;
	}

	val = REG_GYRO_CONFIG0_ODR(cfg->settings.gyro.odr) |
	      REG_GYRO_CONFIG0_FS(cfg->settings.gyro.fs);
	err = reg_write(dev, REG_GYRO_CONFIG0, val);
	if (err) {
		LOG_ERR("Failed to write Gyro settings: %d", err);
		return err;
	}

	/** Write Low-pass filter settings through indirect register access */
	uint8_t gyro_lpf_write_array[] = REG_IREG_PREPARE_WRITE_ARRAY(
						REG_IPREG_SYS1_OFFSET,
						REG_IPREG_SYS1_REG_172,
						REG_IPREG_SYS1_REG_172_GYRO_LPFBW_SEL(
							cfg->settings.gyro.lpf));

	err = icm45686_bus_write(dev, REG_IREG_ADDR_15_8, gyro_lpf_write_array,
				 sizeof(gyro_lpf_write_array));
	if (err) {
		LOG_ERR("Failed to set Gyro BW settings: %d", err);
		return err;
	}

	/** Wait before indirect register write is made effective
	 * before proceeding with next one.
	 */
	k_sleep(K_MSEC(1));

	uint8_t accel_lpf_write_array[] = REG_IREG_PREPARE_WRITE_ARRAY(
						REG_IPREG_SYS2_OFFSET,
						REG_IPREG_SYS2_REG_131,
						REG_IPREG_SYS2_REG_131_ACCEL_LPFBW_SEL(
							cfg->settings.accel.lpf));

	err = icm45686_bus_write(dev, REG_IREG_ADDR_15_8, accel_lpf_write_array,
				 sizeof(accel_lpf_write_array));
	if (err) {
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

	LOG_DBG("Init OK");

	return 0;
}

#define ICM45686_VALID_ACCEL_ODR(pwr_mode, odr)							   \
	((pwr_mode == ICM45686_DT_ACCEL_LP && odr >= ICM45686_DT_ACCEL_ODR_400) ||		   \
	 (pwr_mode == ICM45686_DT_ACCEL_LN && odr <= ICM45686_DT_ACCEL_ODR_12_5) ||		   \
	 (pwr_mode == ICM45686_DT_ACCEL_OFF))

#define ICM45686_VALID_GYRO_ODR(pwr_mode, odr)							   \
	((pwr_mode == ICM45686_DT_GYRO_LP && odr >= ICM45686_DT_GYRO_ODR_400) ||		   \
	 (pwr_mode == ICM45686_DT_GYRO_LN && odr <= ICM45686_DT_GYRO_ODR_12_5) ||		   \
	 (pwr_mode == ICM45686_DT_GYRO_OFF))

#define ICM45686_INIT(inst)									   \
												   \
	RTIO_DEFINE(icm45686_rtio_ctx_##inst, 8, 8);						   \
												   \
	COND_CODE_1(DT_INST_ON_BUS(inst, i3c),							   \
		    (I3C_DT_IODEV_DEFINE(icm45686_bus_##inst,					   \
					 DT_DRV_INST(inst))),					   \
	(COND_CODE_1(DT_INST_ON_BUS(inst, i2c),							   \
		    (I2C_DT_IODEV_DEFINE(icm45686_bus_##inst,					   \
					 DT_DRV_INST(inst))),					   \
		    ())));									   \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),							   \
		    (SPI_DT_IODEV_DEFINE(icm45686_bus_##inst,					   \
					 DT_DRV_INST(inst),					   \
					 SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,  \
					 0U)),							   \
		    ());									   \
												   \
												   \
	static const struct icm45686_config icm45686_cfg_##inst = {				   \
		.settings = {									   \
			.accel = {								   \
				.pwr_mode = DT_INST_PROP(inst, accel_pwr_mode),			   \
				.fs = DT_INST_PROP(inst, accel_fs),				   \
				.odr = DT_INST_PROP(inst, accel_odr),				   \
				.lpf = DT_INST_PROP_OR(inst, accel_lpf, 0),			   \
			},									   \
			.gyro = {								   \
				.pwr_mode = DT_INST_PROP(inst, gyro_pwr_mode),			   \
				.fs = DT_INST_PROP(inst, gyro_fs),				   \
				.odr = DT_INST_PROP(inst, gyro_odr),				   \
				.lpf = DT_INST_PROP_OR(inst, gyro_lpf, 0),			   \
			},									   \
			.fifo_watermark = DT_INST_PROP_OR(inst, fifo_watermark, 0),		   \
		},										   \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),			   \
	};											   \
	static struct icm45686_data icm45686_data_##inst = {					   \
		.edata.header = {								   \
			.accel_fs = DT_INST_PROP(inst, accel_fs),				   \
			.gyro_fs = DT_INST_PROP(inst, gyro_fs),					   \
		},										   \
		.rtio = {									   \
			.iodev = &icm45686_bus_##inst,						   \
			.ctx = &icm45686_rtio_ctx_##inst,					   \
			COND_CODE_1(DT_INST_ON_BUS(inst, i3c),					   \
				(.type = ICM45686_BUS_I3C,					   \
				 .i3c.id = I3C_DEVICE_ID_DT_INST(inst),),			   \
			(COND_CODE_1(DT_INST_ON_BUS(inst, i2c),					   \
				(.type = ICM45686_BUS_I2C), ())))				   \
			COND_CODE_1(DT_INST_ON_BUS(inst, spi),					   \
				(.type = ICM45686_BUS_SPI), ())					   \
		},										   \
	};											   \
												   \
	/* Build-time settings verification: Inform the user of invalid settings at build time */  \
	BUILD_ASSERT(ICM45686_VALID_ACCEL_ODR(DT_INST_PROP(inst, accel_pwr_mode),		   \
					      DT_INST_PROP(inst, accel_odr)),			   \
		     "Invalid accel ODR setting. Please check supported ODRs for LP and LN");	   \
	BUILD_ASSERT(ICM45686_VALID_GYRO_ODR(DT_INST_PROP(inst, gyro_pwr_mode),			   \
					     DT_INST_PROP(inst, gyro_odr)),			   \
		     "Invalid gyro ODR setting. Please check supported ODRs for LP and LN");	   \
												   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm45686_init,					   \
				     NULL,							   \
				     &icm45686_data_##inst,					   \
				     &icm45686_cfg_##inst,					   \
				     POST_KERNEL,						   \
				     CONFIG_SENSOR_INIT_PRIORITY,				   \
				     &icm45686_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ICM45686_INIT)
