/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2dh

#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/byteorder.h>

#include "lis2dh.h"
#include "lis2dh_decoder.h"

/* Check cancellation, not sensor data, while waiting for a hardware event. */
#define LIS2DH_CANCEL_INTERVAL K_MSEC(10)

static uint8_t lis2dh_stream_route(enum sensor_trigger_type trigger)
{
	return trigger == SENSOR_TRIG_FIFO_FULL ? LIS2DH_EN_FIFO_OVRN_INT1
						: LIS2DH_EN_FIFO_WTM_INT1;
}

static void lis2dh_stream_fail(const struct device *dev, int error)
{
	struct lis2dh_data *data = dev->data;
	struct rtio_iodev_sqe *sqe = data->streaming_sqe;

	data->streaming_sqe = NULL;
	(void)lis2dh_fifo_stop(dev);
	if (sqe != NULL) {
		rtio_iodev_sqe_err(sqe, error);
	}
}

static void lis2dh_stream_cancel_work(struct k_work *work)
{
	struct lis2dh_data *data =
		CONTAINER_OF(k_work_delayable_from_work(work), struct lis2dh_data, stream_work);
	const struct device *dev = data->dev;

	lis2dh_lock(dev);
	if (data->streaming_sqe != NULL) {
		if ((data->streaming_sqe->sqe.flags & RTIO_SQE_CANCELED) != 0U) {
			lis2dh_stream_fail(dev, -ECANCELED);
		} else {
			(void)k_work_reschedule(&data->stream_work, LIS2DH_CANCEL_INTERVAL);
		}
	}
	lis2dh_unlock(dev);
}

void lis2dh_stream_init(const struct device *dev)
{
	struct lis2dh_data *data = dev->data;

	data->dev = dev;
	k_work_init_delayable(&data->stream_work, lis2dh_stream_cancel_work);
}

static int lis2dh_stream_arm(const struct device *dev, const struct sensor_read_config *cfg)
{
	struct lis2dh_data *data = dev->data;
	uint8_t routes = 0U;
	int status;

	for (size_t i = 0U; i < cfg->count; i++) {
		uint8_t route = lis2dh_stream_route(cfg->triggers[i].trigger);

		if (cfg->triggers[i].opt != SENSOR_STREAM_DATA_NOP) {
			data->stream_nop_events &= ~route;
		}
		routes |= route;
	}
	routes &= ~data->stream_nop_events;
	status = data->hw_tf->update_reg(
		dev, LIS2DH_REG_CTRL3, LIS2DH_EN_FIFO_WTM_INT1 | LIS2DH_EN_FIFO_OVRN_INT1, routes);
	if (status < 0) {
		return status;
	}
	return lis2dh_trigger_fifo_int1_set(dev, routes != 0U);
}

void lis2dh_stream_submit(const struct device *dev, struct rtio_iodev_sqe *sqe)
{
	const struct sensor_read_config *cfg = sqe->sqe.iodev->data;
	struct lis2dh_data *data = dev->data;
	uint8_t routes = 0U;
	bool resumed;
	int status = 0;

	lis2dh_lock(dev);
	resumed = atomic_ptr_cas(&data->stream_pending, sqe, NULL);
	if (resumed && !data->stream_active) {
		status = -ECANCELED;
		goto reject;
	}
	if (data->streaming_sqe != NULL || atomic_ptr_get(&data->stream_pending) != NULL ||
	    (data->stream_active && (!resumed || data->stream_iodev != sqe->sqe.iodev))) {
		status = -EBUSY;
		goto reject;
	}
	if (cfg->count == 0U) {
		status = -EINVAL;
		goto reject;
	}
	for (size_t i = 0U; i < cfg->count; i++) {
		const struct sensor_stream_trigger *trig = &cfg->triggers[i];

		if (trig->trigger != SENSOR_TRIG_FIFO_WATERMARK &&
		    trig->trigger != SENSOR_TRIG_FIFO_FULL) {
			status = -ENOTSUP;
			goto reject;
		}
		if ((trig->opt != SENSOR_STREAM_DATA_INCLUDE &&
		     trig->opt != SENSOR_STREAM_DATA_DROP && trig->opt != SENSOR_STREAM_DATA_NOP) ||
		    (routes & lis2dh_stream_route(trig->trigger)) != 0U) {
			status = -EINVAL;
			goto reject;
		}
		routes |= lis2dh_stream_route(trig->trigger);
	}
	if ((sqe->sqe.flags & RTIO_SQE_CANCELED) != 0U) {
		status = -ECANCELED;
		goto reject;
	}
	if (!data->stream_active && lis2dh_fifo_is_busy(dev)) {
		status = -EBUSY;
		goto reject;
	}

	data->streaming_sqe = sqe;
	if (!data->stream_active) {
		data->stream_active = true;
		data->stream_iodev = sqe->sqe.iodev;
		data->stream_routes = routes;
		data->stream_nop_events = 0U;
		status = lis2dh_fifo_start(dev);
	} else {
		status = lis2dh_stream_arm(dev, cfg);
	}
	if (status < 0) {
		lis2dh_stream_fail(dev, status);
	} else {
		(void)k_work_reschedule(&data->stream_work, LIS2DH_CANCEL_INTERVAL);
	}
	lis2dh_unlock(dev);
	return;

reject:
	if (resumed && data->stream_active) {
		(void)lis2dh_fifo_stop(dev);
	}
	rtio_iodev_sqe_err(sqe, status);
	lis2dh_unlock(dev);
}

int lis2dh_stream_handle_irq(const struct device *dev)
{
	struct lis2dh_data *data = dev->data;
	struct rtio_iodev_sqe *sqe = data->streaming_sqe;
	const struct sensor_read_config *cfg;
	const struct sensor_stream_trigger *event = NULL;
	struct lis2dh_encoded_header *header;
	uint8_t *buffer;
	uint32_t buffer_len;
	uint32_t required;
	uint8_t src;
	uint8_t count;
	uint64_t timestamp;
	int status;

	/* Called with the lifecycle lock held. Keep GPIO masked during handoff. */
	if (sqe == NULL) {
		return 0;
	}
	if ((sqe->sqe.flags & RTIO_SQE_CANCELED) != 0U) {
		status = -ECANCELED;
		goto fail;
	}
	cfg = sqe->sqe.iodev->data;
	status = data->hw_tf->read_reg(dev, LIS2DH_REG_FIFO_SRC, &src);
	if (status < 0) {
		goto fail;
	}
	timestamp = lis2dh_timestamp_ns();
	for (size_t i = 0U; i < cfg->count; i++) {
		const struct sensor_stream_trigger *trig = &cfg->triggers[i];
		uint8_t flag =
			trig->trigger == SENSOR_TRIG_FIFO_FULL ? LIS2DH_FIFO_OVRN : LIS2DH_FIFO_WTM;

		if ((src & flag) != 0U &&
		    (data->stream_nop_events & lis2dh_stream_route(trig->trigger)) == 0U) {
			event = trig;
			if (trig->trigger == SENSOR_TRIG_FIFO_FULL) {
				break;
			}
		}
	}
	if (event == NULL) {
		status = lis2dh_stream_arm(dev, cfg);
		if (status < 0) {
			goto fail;
		}
		return 0;
	}
	count = (src & LIS2DH_FIFO_EMPTY) != 0U
			? 0U
			: ((src & LIS2DH_FIFO_OVRN) != 0U ? LIS2DH_FIFO_MAX_SAMPLES
							  : src & LIS2DH_FIFO_FSS_MASK);
	if (event->opt != SENSOR_STREAM_DATA_INCLUDE) {
		count = 0U;
	}
	required = sizeof(*header) + count * LIS2DH_ENCODED_SAMPLE_SIZE;
	status = rtio_sqe_rx_buf(sqe, required, required, &buffer, &buffer_len);
	if (status < 0) {
		goto fail;
	}
	if (count != 0U) {
		status = data->hw_tf->read_data(dev, LIS2DH_REG_ACCEL_X_LSB,
						buffer + sizeof(*header),
						count * LIS2DH_ENCODED_SAMPLE_SIZE);
		if (status < 0) {
			goto fail;
		}
		for (size_t axis = 0U; axis < 3U; axis++) {
			data->sample.xyz[axis] = (int16_t)sys_get_le16(
				buffer + sizeof(*header) +
				(count - 1U) * LIS2DH_ENCODED_SAMPLE_SIZE + axis * 2U);
		}
		data->sample.status = LIS2DH_STATUS_ZYX_DRDY;
		data->fifo_cache_valid = true;
	}
	if (event->opt == SENSOR_STREAM_DATA_DROP) {
		status = lis2dh_fifo_drop(dev);
		if (status < 0) {
			goto fail;
		}
		data->stream_nop_events = 0U;
	} else if (event->opt == SENSOR_STREAM_DATA_NOP) {
		/* A level event is reported once until data is included or dropped. */
		data->stream_nop_events |= lis2dh_stream_route(event->trigger);
	} else {
		data->stream_nop_events = 0U;
	}
	header = (struct lis2dh_encoded_header *)buffer;
	*header = (struct lis2dh_encoded_header){
		.timestamp_ns = timestamp,
		.period_ns = data->fifo_period_ns,
		.scale = data->scale,
		.sample_count = count,
		.trigger = event->trigger,
		.shift = lis2dh_encoded_shift(data->scale),
		.is_fifo = 1U,
	};
	if ((sqe->sqe.flags & RTIO_SQE_CANCELED) != 0U) {
		status = -ECANCELED;
		goto fail;
	}
	(void)k_work_cancel_delayable(&data->stream_work);
	data->streaming_sqe = NULL;
	atomic_ptr_set(&data->stream_handoff, sqe);
	rtio_iodev_sqe_ok(sqe, 0);
	/* Never access sqe after completion. The executor either resubmits it
	 * synchronously to submit(), or terminates it (including late cancel).
	 */
	if (atomic_ptr_get(&data->stream_handoff) != NULL) {
		atomic_ptr_set(&data->stream_handoff, NULL);
		(void)lis2dh_fifo_stop(dev);
	}
	return 0;

fail:
	lis2dh_stream_fail(dev, status);
	return status;
}
