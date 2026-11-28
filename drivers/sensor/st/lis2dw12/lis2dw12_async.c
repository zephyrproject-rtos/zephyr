/* ST Microelectronics LIS2DW12 3-axis accelerometer driver
 *
 * Copyright (c) 2026 Sloc GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/regmap.h>
#include <string.h>

#include "lis2dw12.h"
#include "lis2dw12_async.h"

LOG_MODULE_DECLARE(LIS2DW12, CONFIG_SENSOR_LOG_LEVEL);

static int lis2dw12_get_requested_channels(const struct sensor_read_config *cfg, bool *has_accel,
					   bool *has_temp)
{
	*has_accel = false;
	*has_temp = false;

	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->channels[i].chan_idx != 0U) {
			return -ENOTSUP;
		}

		switch (cfg->channels[i].chan_type) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			*has_accel = true;
			break;
		case SENSOR_CHAN_DIE_TEMP:
			*has_temp = true;
			break;
		case SENSOR_CHAN_ALL:
			*has_accel = true;
			*has_temp = true;
			break;
		default:
			return -ENOTSUP;
		}
	}

	return (*has_accel || *has_temp) ? 0 : -ENOTSUP;
}

static void lis2dw12_one_shot_complete_cb(struct rtio *ctx, const struct rtio_sqe *sqe, int result,
					  void *arg)
{
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;
	const struct device *dev = arg;
	const struct lis2dw12_device_config *cfg = dev->config;
	struct lis2dw12_encoded_data *encoded;
	uint8_t *buf;
	uint32_t buf_len;
	uint8_t shift;
	int ret;

	ARG_UNUSED(result);

	ret = rtio_flush_completion_queue(ctx);
	if (ret < 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	ret = rtio_sqe_rx_buf(iodev_sqe, sizeof(*encoded), sizeof(*encoded), &buf, &buf_len);
	if (ret < 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	/* Apply the appropriate shift to the accelerometer data based on the power mode */
	encoded = (struct lis2dw12_encoded_data *)buf;
	if (encoded->has_accel) {
		shift = cfg->pm == LIS2DW12_CONT_LOW_PWR_12bit ? LIS2DW12_SHIFT_PM1
							       : LIS2DW12_SHIFT_PMOTHER;
		for (size_t i = 0; i < ARRAY_SIZE(encoded->accel); i++) {
			encoded->accel[i] >>= shift;
		}
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static void lis2dw12_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct lis2dw12_data *data = dev->data;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct lis2dw12_encoded_data *encoded;
	struct rtio_regs_list regs_list[2];
	struct rtio_regs regs = {
		.rtio_regs_list = regs_list,
		.rtio_regs_num = 0,
	};
	uint8_t *buf;
	uint32_t buf_len;
	uint64_t cycles;
	bool has_accel;
	bool has_temp;
	int ret;

	if ((data->rtio_ctx == NULL) || (data->iodev == NULL)) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	ret = lis2dw12_get_requested_channels(cfg, &has_accel, &has_temp);
	if (ret < 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	ret = rtio_sqe_rx_buf(iodev_sqe, sizeof(*encoded), sizeof(*encoded), &buf, &buf_len);
	if (ret < 0) {
		LOG_ERR("Failed to get a read buffer of size %zu bytes", sizeof(*encoded));
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	encoded = (struct lis2dw12_encoded_data *)buf;
	memset(encoded, 0, sizeof(*encoded));

	ret = sensor_clock_get_cycles(&cycles);
	if (ret < 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	encoded->timestamp_ns = sensor_clock_cycles_to_ns(cycles);
	encoded->gain = data->gain;
	encoded->has_accel = has_accel;
	encoded->has_temp = has_temp;

	if (has_accel) {
		regs_list[regs.rtio_regs_num].reg_addr = LIS2DW12_OUT_X_L;
		regs_list[regs.rtio_regs_num].bufp = (uint8_t *)encoded->accel;
		regs_list[regs.rtio_regs_num].len = sizeof(encoded->accel);
		regs.rtio_regs_num++;
	}

	if (has_temp) {
		regs_list[regs.rtio_regs_num].reg_addr = LIS2DW12_OUT_T_L;
		regs_list[regs.rtio_regs_num].bufp = (uint8_t *)&encoded->temp;
		regs_list[regs.rtio_regs_num].len = sizeof(encoded->temp);
		regs.rtio_regs_num++;
	}

	rtio_read_regs_async(data->rtio_ctx, data->iodev, RTIO_BUS_I2C, &regs, iodev_sqe, dev,
			     lis2dw12_one_shot_complete_cb);
}

void lis2dw12_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (cfg->is_streaming) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	lis2dw12_submit_one_shot(dev, iodev_sqe);
}
