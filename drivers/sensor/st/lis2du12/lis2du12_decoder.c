/* ST Microelectronics LIS2DU12 / MIS2DU12 sensor read and decoder
 *
 * Copyright (c) 2026 Dotcom IoT LLP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2du12

#include "lis2du12.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(LIS2DU12, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Shift values for 2G, 4G, 8G, 16G full-scale ranges in m/s^2 (q31_t format):
 * 2G  = 19.6133 m/s^2  -> shift = 5 (max 32)
 * 4G  = 39.2266 m/s^2  -> shift = 6 (max 64)
 * 8G  = 78.4532 m/s^2  -> shift = 7 (max 128)
 * 16G = 156.9064 m/s^2 -> shift = 8 (max 256)
 */
static const int8_t lis2du12_accel_shift[] = {
	5, /* FS_2G */
	6, /* FS_4G */
	7, /* FS_8G */
	8, /* FS_16G */
};

static int lis2du12_fs_to_idx(uint8_t accel_fs)
{
	switch (accel_fs) {
	case 2:
		return 0;
	case 4:
		return 1;
	case 8:
		return 2;
	case 16:
		return 3;
	default:
		return 0;
	}
}

void lis2du12_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	const struct lis2du12_config *cfg = dev->config;
	struct lis2du12_data *data = dev->data;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint32_t min_buf_len = sizeof(struct lis2du12_encoded_data);
	struct lis2du12_encoded_data *edata;
	uint8_t *buf;
	uint32_t buf_len;
	uint64_t cycles;
	lis2du12_data_t xl_data;
	lis2du12_md_t md;
	int rc;

	if (read_cfg->is_streaming) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata = (struct lis2du12_encoded_data *)buf;

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);
	edata->header.accel_fs = data->accel_fs;

	md.fs = cfg->accel_range;
	if (lis2du12_data_get(ctx, &md, &xl_data) < 0) {
		LOG_ERR("Failed to read sample");
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
		return;
	}

	edata->acc[0] = xl_data.xl.raw[0];
	edata->acc[1] = xl_data.xl.raw[1];
	edata->acc[2] = xl_data.xl.raw[2];

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static int lis2du12_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	ARG_UNUSED(buffer);

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		*frame_count = 1;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int lis2du12_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					  size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static void lis2du12_convert_raw_to_q31(int16_t raw_val, uint8_t accel_fs, q31_t *out)
{
	/* Sensitivity in ug/LSB: accel_fs * 61 / 2 */
	int64_t sensitivity = (int64_t)accel_fs * GAIN_UNIT_XL / 2;
	int8_t shift = lis2du12_accel_shift[lis2du12_fs_to_idx(accel_fs)];

	/* Convert raw value to micro-m/s^2, then scale to q31_t with shift */
	int64_t micro_ms2 = (int64_t)raw_val * sensitivity * SENSOR_G / 1000000LL;

	*out = (q31_t)((micro_ms2 << (31 - shift)) / 1000000LL);
}

static int lis2du12_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct lis2du12_encoded_data *edata = (const struct lis2du12_encoded_data *)buffer;
	struct sensor_three_axis_data *out = (struct sensor_three_axis_data *)data_out;

	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = lis2du12_accel_shift[lis2du12_fs_to_idx(edata->header.accel_fs)];

		lis2du12_convert_raw_to_q31(edata->acc[0], edata->header.accel_fs,
					    &out->readings[0].x);
		lis2du12_convert_raw_to_q31(edata->acc[1], edata->header.accel_fs,
					    &out->readings[0].y);
		lis2du12_convert_raw_to_q31(edata->acc[2], edata->header.accel_fs,
					    &out->readings[0].z);

		*fit = 1;
		return 1;
	default:
		return -EINVAL;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = lis2du12_decoder_get_frame_count,
	.get_size_info = lis2du12_decoder_get_size_info,
	.decode = lis2du12_decoder_decode,
};

int lis2du12_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
