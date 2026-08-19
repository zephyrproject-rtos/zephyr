/*
 * Copyright (c) 2026 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT invensense_icm566xx

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/rtio/rtio.h>

#if defined(CONFIG_SENSOR_ASYNC_API)
#include <zephyr/rtio/work.h>
#endif /* CONFIG_SENSOR_ASYNC_API */

#include "icm566xx.h"
#include "icm566xx_bus.h"
#include "icm566xx_decoder.h"
#include "icm566xx_trigger.h"
#include "icm566xx_stream.h"

#include <zephyr/drivers/sensor/tdk_apex.h>
#include <zephyr/drivers/sensor/icm566xx.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM566XX, CONFIG_SENSOR_LOG_LEVEL);

static const struct device *g_icm566xx_dev;

static inline int inv_io_hal_read_reg(uint8_t reg, uint8_t *rbuffer, uint32_t rlen)
{
	const struct device *dev = g_icm566xx_dev;
	struct icm566xx_data *data = dev->data;

	return icm566xx_reg_read_rtio(&data->bus, reg | REG_READ_BIT, rbuffer, rlen);
}

static inline int inv_io_hal_write_reg(uint8_t reg, const uint8_t *wbuffer,
					uint32_t wlen)
{
	const struct device *dev = g_icm566xx_dev;
	struct icm566xx_data *data = dev->data;

	return icm566xx_reg_write_rtio(&data->bus, reg, wbuffer, wlen);
}

void icm566xx_inv_sleep_us(uint32_t us)
{
	k_usleep(us);
}

void icm566xx_accel_ms(uint8_t fs, int32_t in, bool high_res, int32_t *out_ms,
						int32_t *out_ums)
{
	int64_t sensitivity, total_ums;
	uint64_t full_scale_range_lsb = high_res ? 524288 : 32768;

	switch (fs) {
#if defined(CONFIG_DT_HAS_INVENSENSE_ICM56686_ENABLED)
	case ICM566XX_DT_ACCEL_FS_32:
		sensitivity = full_scale_range_lsb / 32;
		break;
#endif
	case ICM566XX_DT_ACCEL_FS_16:
		sensitivity = full_scale_range_lsb / 16;
		break;
	case ICM566XX_DT_ACCEL_FS_8:
		sensitivity = full_scale_range_lsb / 8;
		break;
	case ICM566XX_DT_ACCEL_FS_4:
		sensitivity = full_scale_range_lsb / 4;
		break;
	case ICM566XX_DT_ACCEL_FS_2:
		sensitivity = full_scale_range_lsb / 2;
		break;
	default:
		CODE_UNREACHABLE;
	}

	total_ums = ((int64_t)in * SENSOR_G) / sensitivity;
	*out_ms = (int32_t)(total_ums / 1000000LL);
	*out_ums = (int32_t)(total_ums % 1000000LL);
}

void icm566xx_gyro_rads(uint8_t fs, int32_t in, bool high_res, int32_t *out_rads,
						int32_t *out_urads)
{
	int64_t sensitivity, total_urads;
	uint64_t full_scale_range_lsb = high_res ? 524288 : 32768;

	switch (fs) {
#if defined(CONFIG_DT_HAS_INVENSENSE_ICM56686_ENABLED)
	case ICM566XX_DT_GYRO_FS_4000:
		sensitivity = (full_scale_range_lsb * 1000LL) / 4000LL;
		break;
#endif
	case ICM566XX_DT_GYRO_FS_2000:
		sensitivity = (full_scale_range_lsb * 1000LL) / 2000LL;
		break;
	case ICM566XX_DT_GYRO_FS_1000:
		sensitivity = (full_scale_range_lsb * 1000LL) / 1000LL;
		break;
	case ICM566XX_DT_GYRO_FS_500:
		sensitivity = (full_scale_range_lsb * 1000LL) / 500LL;
		break;
	case ICM566XX_DT_GYRO_FS_250:
		sensitivity = (full_scale_range_lsb * 1000LL) / 250LL;
		break;
	case ICM566XX_DT_GYRO_FS_125:
		sensitivity = (full_scale_range_lsb * 1000LL) / 125LL;
		break;
	case ICM566XX_DT_GYRO_FS_62_5:
		sensitivity = (full_scale_range_lsb * 10000LL) / 625LL;
		break;
	case ICM566XX_DT_GYRO_FS_31_25:
		sensitivity = (full_scale_range_lsb * 100000LL) / 3125LL;
		break;
	case ICM566XX_DT_GYRO_FS_15_625:
		sensitivity = (full_scale_range_lsb * 1000000LL) / 15625LL;
		break;
	default:
		CODE_UNREACHABLE;
	}

	total_urads = ((int64_t)in * SENSOR_PI * 10000LL) / (sensitivity * 180LL);
	*out_rads = (int32_t)(total_urads / 1000000LL);
	*out_urads = (int32_t)(total_urads % 1000000LL);
}

void icm566xx_temp_c(int32_t in, int32_t *out_c, uint32_t *out_uc)
{
	int64_t sensitivity = 13248; /* value equivalent for x100 1c */

	/* Offset by 25 degrees Celsius */
	int64_t in100 = (in * 100) + (25 * sensitivity);

	/* Whole celsius */
	*out_c = in100 / sensitivity;

	/* Micro celsius */
	*out_uc = ((in100 - (*out_c) * sensitivity) * INT64_C(1000000)) / sensitivity;
}

static int icm566xx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int err;
	struct icm566xx_data *data = dev->data;
	struct icm566xx_encoded_data *edata = &data->edata;
	inv_imu_sensor_data_t imu_data;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	err = icm566xx_get_register_data(&data->driver, &imu_data);

	edata->payload.accel.x = imu_data.accel_data[0];
	edata->payload.accel.y = imu_data.accel_data[1];
	edata->payload.accel.z = imu_data.accel_data[2];

	edata->payload.gyro.x = imu_data.gyro_data[0];
	edata->payload.gyro.y = imu_data.gyro_data[1];
	edata->payload.gyro.z = imu_data.gyro_data[2];

	edata->payload.temp = imu_data.temp_data;

	LOG_HEXDUMP_DBG(edata->payload.buf, sizeof(edata->payload.buf), "ICM566XX data");

	return err;
}

static pwr_mgmt0_accel_mode_t icm566xx_get_accel_mode(inv_imu_device_t *s)
{
	pwr_mgmt0_t pwr_mgmt0;

	icm566xx_get_endianness(s);
	icm566xx_read_reg(s, PWR_MGMT0, 1, (uint8_t *)&pwr_mgmt0);

	return pwr_mgmt0.accel_mode;
}

static pwr_mgmt0_gyro_mode_t icm566xx_get_gyro_mode(inv_imu_device_t *s)
{
	pwr_mgmt0_t pwr_mgmt0;

	icm566xx_get_endianness(s);
	icm566xx_read_reg(s, PWR_MGMT0, 1, (uint8_t *)&pwr_mgmt0);

	return pwr_mgmt0.gyro_mode;
}

static int icm566xx_accel_config(struct icm566xx_data *drv_data, enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	if (attr == SENSOR_ATTR_CONFIGURATION) {
		icm566xx_set_accel_mode(&drv_data->driver, val->val1);
	} else if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		pwr_mgmt0_accel_mode_t c_mode = icm566xx_get_accel_mode(&drv_data->driver);

		icm566xx_set_accel_frequency(&drv_data->driver, val->val1);

		if (val->val1 && c_mode == PWR_MGMT0_ACCEL_MODE_OFF) {
			if (val->val1 >= ACCEL_CONFIG0_ACCEL_ODR_12_5_HZ) {
				icm566xx_set_accel_mode(&drv_data->driver, PWR_MGMT0_ACCEL_MODE_LN);
			} else {
				icm566xx_set_accel_mode(&drv_data->driver, PWR_MGMT0_ACCEL_MODE_LP);
			}
		} else if (val->val1 == 0) {
			icm566xx_set_accel_mode(&drv_data->driver, PWR_MGMT0_ACCEL_MODE_OFF);
		} else {
			LOG_ERR("Wrong config with accel mode");
		}
	} else if (attr == SENSOR_ATTR_FULL_SCALE) {
		icm566xx_set_accel_fsr(&drv_data->driver, val->val1);
	} else if ((enum sensor_attribute_icm566xx)attr == SENSOR_ATTR_BW_FILTER_LPF) {
		icm566xx_set_accel_ln_bw(&drv_data->driver, val->val1);
	} else if ((enum sensor_attribute_icm566xx)attr == SENSOR_ATTR_AVERAGING) {
		icm566xx_set_accel_lp_avg(&drv_data->driver, val->val1);
	} else {
		LOG_ERR("Unsupported attribute");
		return -EINVAL;
	}
	return 0;
}

static int icm566xx_gyro_config(struct icm566xx_data *drv_data, enum sensor_attribute attr,
				const struct sensor_value *val)
{
	if (attr == SENSOR_ATTR_CONFIGURATION) {
		icm566xx_set_gyro_mode(&drv_data->driver, val->val1);
	} else if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		pwr_mgmt0_gyro_mode_t c_mode = icm566xx_get_gyro_mode(&drv_data->driver);

		icm566xx_set_gyro_frequency(&drv_data->driver, val->val1);

		if (val->val1 && c_mode == PWR_MGMT0_GYRO_MODE_OFF) {
			if (val->val1 >= GYRO_CONFIG0_GYRO_ODR_12_5_HZ) {
				icm566xx_set_gyro_mode(&drv_data->driver, PWR_MGMT0_GYRO_MODE_LN);
			} else {
				icm566xx_set_gyro_mode(&drv_data->driver, PWR_MGMT0_GYRO_MODE_LP);
			}
		} else if (val->val1 == 0) {
			icm566xx_set_gyro_mode(&drv_data->driver, PWR_MGMT0_GYRO_MODE_OFF);
		} else {
			LOG_ERR("Wrong config with gyro mode");
		}
	} else if (attr == SENSOR_ATTR_FULL_SCALE) {
		icm566xx_set_gyro_fsr(&drv_data->driver, val->val1);
	} else if ((enum sensor_attribute_icm566xx)attr == SENSOR_ATTR_BW_FILTER_LPF) {
		icm566xx_set_gyro_ln_bw(&drv_data->driver, val->val1);
	} else {
		LOG_ERR("Unsupported attribute");
		return -EINVAL;
	}
	return 0;
}

static int icm566xx_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct icm566xx_data *drv_data = dev->data;

	__ASSERT_NO_MSG(val != NULL);

	if ((enum sensor_channel_tdk_apex)chan == SENSOR_CHAN_APEX_MOTION) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	} else if (SENSOR_CHANNEL_IS_ACCEL(chan)) {
		icm566xx_accel_config(drv_data, attr, val);
	} else if (SENSOR_CHANNEL_IS_GYRO(chan)) {
		icm566xx_gyro_config(drv_data, attr, val);
	} else {
		LOG_ERR("Unsupported channel");
		(void)drv_data;
		return -EINVAL;
	}

	return 0;
}

static int icm566xx_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct icm566xx_config *cfg = dev->config;
	int res = 0;

	__ASSERT_NO_MSG(val != NULL);

	switch (chan) {
	case SENSOR_CHAN_APEX_MOTION:
		if (attr == SENSOR_ATTR_CONFIGURATION) {
			val->val1 = cfg->apex;
		}
		break;
	default:
		res = -EINVAL;
		break;
	}

	return res;
}

static int icm566xx_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct icm566xx_data *data = dev->data;
	bool is_high_res = false;

	if (chan > SENSOR_CHAN_DIE_TEMP ||
		(chan >= SENSOR_CHAN_MAGN_X && chan <= SENSOR_CHAN_MAGN_XYZ)) {
		return -ENOTSUP;
	}

#if INV_IMU_20BIT_REG_DATA_SUPPORTED
	is_high_res = true;
#endif

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		icm566xx_accel_ms(data->edata.header.accel_fs, data->edata.payload.accel.x,
				  is_high_res, &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm566xx_accel_ms(data->edata.header.accel_fs, data->edata.payload.accel.y,
				  is_high_res, &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm566xx_accel_ms(data->edata.header.accel_fs, data->edata.payload.accel.z,
				  is_high_res, &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm566xx_gyro_rads(data->edata.header.gyro_fs, data->edata.payload.gyro.x,
				   is_high_res, &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm566xx_gyro_rads(data->edata.header.gyro_fs, data->edata.payload.gyro.y,
				   is_high_res, &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm566xx_gyro_rads(data->edata.header.gyro_fs, data->edata.payload.gyro.z,
				   is_high_res, &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm566xx_temp_c(data->edata.payload.temp, &val->val1, &val->val2);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		icm566xx_accel_ms(data->edata.header.accel_fs, data->edata.payload.accel.x,
				  is_high_res, &val[0].val1, &val[0].val2);
		icm566xx_accel_ms(data->edata.header.accel_fs, data->edata.payload.accel.y,
				  is_high_res, &val[1].val1, &val[1].val2);
		icm566xx_accel_ms(data->edata.header.accel_fs, data->edata.payload.accel.z,
				  is_high_res, &val[2].val1, &val[2].val2);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm566xx_gyro_rads(data->edata.header.gyro_fs, data->edata.payload.gyro.x,
				   is_high_res, &val[0].val1, &val[0].val2);
		icm566xx_gyro_rads(data->edata.header.gyro_fs, data->edata.payload.gyro.y,
				   is_high_res, &val[1].val1, &val[1].val2);
		icm566xx_gyro_rads(data->edata.header.gyro_fs, data->edata.payload.gyro.z,
				   is_high_res, &val[2].val1, &val[2].val2);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#if defined(CONFIG_SENSOR_ASYNC_API)

static void icm566xx_complete_result(struct rtio *ctx, const struct rtio_sqe *sqe, int result,
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

static inline void icm566xx_submit_one_shot(const struct device *dev,
					    struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	uint32_t min_buf_len = sizeof(struct icm566xx_encoded_data);
	int err;
	uint8_t *buf;
	uint32_t buf_len;
	struct icm566xx_encoded_data *edata;
	struct icm566xx_data *data = dev->data;
	struct rtio_sqe *read_sqe;

	err = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (err != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	edata = (struct icm566xx_encoded_data *)buf;

	err = icm566xx_encode(dev, channels, num_channels, buf);
	if (err != 0) {
		LOG_ERR("Failed to encode sensor data");
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	err = icm566xx_prep_reg_read_rtio_async(&data->bus, ACCEL_DATA_X_0 | REG_READ_BIT,
						edata->payload.buf, 14, &read_sqe);

	if (err < 0) {
		LOG_ERR("Fail to prepare read: %d", err);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}
	read_sqe->flags |= RTIO_SQE_CHAINED;

#if defined(CONFIG_DT_HAS_INVENSENSE_ICM56686_ENABLED)
	struct rtio_sqe *ext_read_sqe = NULL;

	err = icm566xx_prep_reg_read_rtio_async(&data->bus, EXT_DATA_X | REG_READ_BIT,
						edata->payload.ext_data, 3, &ext_read_sqe);
	if (err < 0) {
		rtio_sqe_drop_all(data->bus.rtio.ctx);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	ext_read_sqe->flags |= RTIO_SQE_CHAINED;
#endif

	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(data->bus.rtio.ctx);

	if (!complete_sqe) {
		LOG_ERR("Failed to acquire complete read-sqe");
		rtio_sqe_drop_all(data->bus.rtio.ctx);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}
	rtio_sqe_prep_callback_no_cqe(complete_sqe, icm566xx_complete_result, (void *)dev,
				      iodev_sqe);

	rtio_submit(data->bus.rtio.ctx, 0);
}

static void icm566xx_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		icm566xx_submit_one_shot(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_ICM566XX_STREAM)) {
		icm566xx_stream_submit(dev, iodev_sqe);
	} else {
		LOG_ERR("Streaming not supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

#endif /* CONFIG_SENSOR_ASYNC_API */

static DEVICE_API(sensor, icm566xx_driver_api) = {
	.sample_fetch = icm566xx_sample_fetch,
	.channel_get = icm566xx_channel_get,
	.attr_set = icm566xx_attr_set,
	.attr_get = icm566xx_attr_get,
#if defined(CONFIG_ICM566XX_TRIGGER)
	.trigger_set = icm566xx_trigger_set,
#endif /* CONFIG_ICM566XX_TRIGGER */
#if defined(CONFIG_SENSOR_ASYNC_API)
	.get_decoder = icm566xx_get_decoder,
	.submit = icm566xx_submit,
#endif /* CONFIG_SENSOR_ASYNC_API */
};

static int icm566xx_init(const struct device *dev)
{
	struct icm566xx_data *data = dev->data;
	const struct icm566xx_config *cfg = dev->config;
	uint8_t read_val = 0;
	int err;

	/* Initialize serial interface and device */
	g_icm566xx_dev = dev;
	data->driver.transport.read_reg = inv_io_hal_read_reg;
	data->driver.transport.write_reg = inv_io_hal_write_reg;
	data->driver.transport.sleep_us = icm566xx_inv_sleep_us;

	switch (data->bus.rtio.type) {
	case ICM566XX_BUS_SPI:
		data->driver.transport.serif_type = UI_SPI4;
		break;
	case ICM566XX_BUS_I2C:
		data->driver.transport.serif_type = UI_I2C;
		break;
	case ICM566XX_BUS_I3C:
		data->driver.transport.serif_type = UI_I3C;
		break;
	default:
		LOG_ERR("Unknown bus type");
	}

#if CONFIG_SPI_RTIO
	if (data->bus.rtio.type == ICM566XX_BUS_SPI && !spi_is_ready_iodev(data->bus.rtio.iodev)) {
		LOG_ERR("Bus is not ready");
		return -ENODEV;
	}
#endif
#if CONFIG_I2C_RTIO
	if (data->bus.rtio.type == ICM566XX_BUS_I2C && !i2c_is_ready_iodev(data->bus.rtio.iodev)) {
		LOG_ERR("Bus is not ready");
		return -ENODEV;
	}
#endif
	/* Set Slew-rate to 10-ns typical, to allow proper SPI readouts */
	if (data->bus.rtio.type == ICM566XX_BUS_SPI) {
		drive_config0_t drive_config0;

		drive_config0.pads_spi_slew = DRIVE_CONFIG0_PADS_SPI_SLEW_TYP_10NS;
		err = icm566xx_write_reg(&data->driver, DRIVE_CONFIG0, 1,
								(uint8_t *)&drive_config0);
		icm566xx_inv_sleep_us(2); /* Takes effect 1.5 us after the register is programmed */
	}

	/** Soft-reset sensor to restore config to defaults,
	 * unless it's already handled by I3C initialization.
	 */
	if (data->bus.rtio.type != ICM566XX_BUS_I3C) {
		err = icm566xx_soft_reset(&data->driver);
		if (err < 0) {
			LOG_ERR("Soft reset failed err %d", err);
			return err;
		}
	}

	/* Confirm ID Value matches */
	err = icm566xx_get_who_am_i(&data->driver, &read_val);
	if (err < 0) {
		LOG_ERR("ID read failed: %d", err);
		return err;
	}

	/* Sensor Configuration */
	err = icm566xx_set_accel_mode(&data->driver, cfg->settings.accel.pwr_mode);
	if (err < 0) {
		LOG_ERR("Failed to set accel mode");
		return err;
	}

	err = icm566xx_set_gyro_mode(&data->driver, cfg->settings.gyro.pwr_mode);
	if (err < 0) {
		LOG_ERR("Failed to set gyro mode");
		return err;
	}

	err = icm566xx_set_accel_frequency(&data->driver, cfg->settings.accel.odr);
	if (err < 0) {
		LOG_ERR("Failed to set Accel frequency: %d", err);
		return err;
	}

	err = icm566xx_set_gyro_frequency(&data->driver, cfg->settings.gyro.odr);
	if (err < 0) {
		LOG_ERR("Failed to set Gyro frequency: %d", err);
		return err;
	}

#if INV_IMU_20BIT_REG_DATA_SUPPORTED == 0
	err = icm566xx_set_accel_fsr(&data->driver, cfg->settings.accel.fs);
	if (err < 0) {
		LOG_ERR("Failed to set Accel fsr: %d", err);
		return err;
	}

	err = icm566xx_set_gyro_fsr(&data->driver, cfg->settings.gyro.fs);
	if (err < 0) {
		LOG_ERR("Failed to set Gyro fsr: %d", err);
		return err;
	}
#endif

	/** Write Low-pass filter settings through indirect register access */
	err = icm566xx_set_gyro_ln_bw(&data->driver, cfg->settings.gyro.lpf);
	if (err < 0) {
		LOG_ERR("Failed to set Gyro BW settings: %d", err);
		return err;
	}

	/** Wait before indirect register write is made effective
	 * before proceeding with next one.
	 */
	k_sleep(K_MSEC(1));

	icm566xx_set_accel_ln_bw(&data->driver, cfg->settings.accel.lpf);
	if (err < 0) {
		LOG_ERR("Failed to set Accel BW settings: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_ICM566XX_TRIGGER)) {
		err = icm566xx_trigger_init(dev);
		if (err) {
			LOG_ERR("Failed to initialize triggers: %d", err);
			return err;
		}
	} else if (IS_ENABLED(CONFIG_ICM566XX_STREAM)) {
		err = icm566xx_stream_init(dev);
		if (err) {
			LOG_ERR("Failed to initialize streaming: %d", err);
			return err;
		}
	} else {
		/*
		 * Compliant with MISRA C - No trigger or stream mode enabled,
		 * intentional fall-through
		 */
	}

	LOG_DBG("Init OK");

	return 0;
}

#define ICM566XX_VALID_ACCEL_ODR(pwr_mode, odr)                                                    \
	((pwr_mode == ICM566XX_DT_ACCEL_LP && odr > ICM566XX_DT_ACCEL_ODR_400) ||                 \
	 (pwr_mode == ICM566XX_DT_ACCEL_LN && odr < ICM566XX_DT_ACCEL_ODR_12_5) ||                \
	 (pwr_mode == ICM566XX_DT_ACCEL_OFF))

#define ICM566XX_VALID_GYRO_ODR(pwr_mode, odr)                                                     \
	((pwr_mode == ICM566XX_DT_GYRO_LP && odr > ICM566XX_DT_GYRO_ODR_400) ||                   \
	 (pwr_mode == ICM566XX_DT_GYRO_LN && odr < ICM566XX_DT_GYRO_ODR_12_5) ||                  \
	 (pwr_mode == ICM566XX_DT_GYRO_OFF))

#define ICM566XX_INIT(inst)                                                                        \
                                                                                                   \
	RTIO_DEFINE(icm566xx_rtio_ctx_##inst, 32, 32);                                             \
                                                                                                   \
	COND_CODE_1(DT_INST_ON_BUS(inst, i3c), \
		    (I3C_DT_IODEV_DEFINE(icm566xx_bus_##inst, \
					 DT_DRV_INST(inst))), \
	(COND_CODE_1(DT_INST_ON_BUS(inst, i2c), \
		    (I2C_DT_IODEV_DEFINE(icm566xx_bus_##inst, \
					 DT_DRV_INST(inst))), \
		    ())));                    \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi), \
			(SPI_DT_IODEV_DEFINE(icm566xx_bus_##inst, \
				DT_DRV_INST(inst), \
				SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)), \
		    ());                                              \
                                                                                                   \
	static const struct icm566xx_config icm566xx_cfg_##inst = {                                \
		.settings =                                                                        \
			{                                                                          \
				.accel =                                                           \
					{                                                          \
						.pwr_mode = DT_INST_PROP(inst, accel_pwr_mode),    \
						.fs = DT_INST_PROP(inst, accel_fs),                \
						.odr = DT_INST_PROP(inst, accel_odr),              \
						.lpf = DT_INST_PROP_OR(inst, accel_lpf, 0),        \
					},                                                         \
				.gyro =                                                            \
					{                                                          \
						.pwr_mode = DT_INST_PROP(inst, gyro_pwr_mode),     \
						.fs = DT_INST_PROP(inst, gyro_fs),                 \
						.odr = DT_INST_PROP(inst, gyro_odr),               \
						.lpf = DT_INST_PROP_OR(inst, gyro_lpf, 0),         \
					},                                                         \
				.fifo_watermark = DT_INST_PROP_OR(inst, fifo_watermark, 0),        \
				.fifo_watermark_equals =                                           \
					DT_INST_PROP(inst, fifo_watermark_equals),                 \
			},                                                                         \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
		.apex = DT_INST_ENUM_IDX(inst, apex),                                              \
	};                                                                                         \
	static struct icm566xx_data icm566xx_data_##inst = {                                       \
		.edata.header =                                                                    \
			{                                                                          \
				.accel_fs = DT_INST_PROP(inst, accel_fs),                          \
				.gyro_fs = DT_INST_PROP(inst, gyro_fs),                            \
			},                                                                         \
		.bus.rtio = {                                                                      \
			.iodev = &icm566xx_bus_##inst,                                             \
			.ctx = &icm566xx_rtio_ctx_##inst,                                          \
			COND_CODE_1(DT_INST_ON_BUS(inst, i3c), \
				(.type = ICM566XX_BUS_I3C, \
				 .i3c.id = I3C_DEVICE_ID_DT_INST(inst),), \
			(COND_CODE_1(DT_INST_ON_BUS(inst, i2c), \
				(.type = ICM566XX_BUS_I2C), ())))                                  \
					  COND_CODE_1(DT_INST_ON_BUS(inst, spi), \
				(.type = ICM566XX_BUS_SPI), ()) },   \
	};                                                                                         \
                                                                                                   \
	/* Build-time settings verification: Inform the user of invalid settings at build time */  \
	BUILD_ASSERT(ICM566XX_VALID_ACCEL_ODR(DT_INST_PROP(inst, accel_pwr_mode),                  \
					      DT_INST_PROP(inst, accel_odr)),                      \
		     "Invalid accel ODR setting. Please check supported ODRs for LP and LN");      \
	BUILD_ASSERT(ICM566XX_VALID_GYRO_ODR(DT_INST_PROP(inst, gyro_pwr_mode),                    \
					     DT_INST_PROP(inst, gyro_odr)),                        \
		     "Invalid gyro ODR setting. Please check supported ODRs for LP and LN");       \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm566xx_init, NULL, &icm566xx_data_##inst,             \
				     &icm566xx_cfg_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &icm566xx_driver_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT invensense_icm56622
DT_INST_FOREACH_STATUS_OKAY(ICM566XX_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT invensense_icm56686
DT_INST_FOREACH_STATUS_OKAY(ICM566XX_INIT)
