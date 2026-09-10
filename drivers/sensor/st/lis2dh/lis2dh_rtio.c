/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2dh

#include <errno.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/work.h>
#include <zephyr/sys/byteorder.h>

#include "lis2dh.h"
#include "lis2dh_decoder.h"

static bool lis2dh_rtio_channel_supported(const struct sensor_chan_spec *channel)
{
	return channel->chan_type == SENSOR_CHAN_ACCEL_XYZ && channel->chan_idx == 0U;
}

static void lis2dh_submit_one_shot(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	struct lis2dh_data *lis2dh = dev->data;
	struct lis2dh_encoded_header *header;
	uint8_t *buffer;
	uint32_t buffer_len;
	uint64_t timestamp_ns;
	int status;
	size_t i;

	if (cfg->count == 0U) {
		rtio_iodev_sqe_err(iodev_sqe, -EINVAL);
		return;
	}

	for (i = 0U; i < cfg->count; i++) {
		if (!lis2dh_rtio_channel_supported(&cfg->channels[i])) {
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}

	lis2dh_lock(dev);
#ifdef CONFIG_LIS2DH_FIFO
	if (lis2dh_fifo_is_busy(dev)) {
		status = -EBUSY;
		goto finish;
	}
#endif

	status =
		rtio_sqe_rx_buf(iodev_sqe, sizeof(*header) + LIS2DH_ENCODED_SAMPLE_SIZE,
				sizeof(*header) + LIS2DH_ENCODED_SAMPLE_SIZE, &buffer, &buffer_len);
	if (status < 0) {
		goto finish;
	}

	status = lis2dh->hw_tf->read_data(dev, LIS2DH_REG_ACCEL_X_LSB, buffer + sizeof(*header),
					  LIS2DH_ENCODED_SAMPLE_SIZE);
	if (status < 0) {
		goto finish;
	}

	timestamp_ns = lis2dh_timestamp_ns();
	header = (struct lis2dh_encoded_header *)buffer;
	header->timestamp_ns = timestamp_ns;
	header->period_ns = 0U;
	header->scale = lis2dh->scale;
	header->sample_count = 1U;
	header->trigger = 0U;
	header->shift = lis2dh_encoded_shift(lis2dh->scale);
	header->is_fifo = 0U;
	memset(header->reserved, 0, sizeof(header->reserved));

finish:
	lis2dh_unlock(dev);
	if (status < 0) {
		rtio_iodev_sqe_err(iodev_sqe, status);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static void lis2dh_submit_work(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		lis2dh_submit_one_shot(iodev_sqe);
		return;
	}

#ifdef CONFIG_LIS2DH_STREAM
	lis2dh_stream_submit(cfg->sensor, iodev_sqe);
#else
	rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
#endif
}

void lis2dh_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req;

	ARG_UNUSED(dev);

	req = rtio_work_req_alloc();
	if (req == NULL) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

#ifdef CONFIG_LIS2DH_STREAM
	struct lis2dh_data *data = dev->data;

	if (atomic_ptr_cas(&data->stream_handoff, iodev_sqe, NULL)) {
		atomic_ptr_set(&data->stream_pending, iodev_sqe);
	}
#endif
	rtio_work_req_submit(req, iodev_sqe, lis2dh_submit_work);
}
