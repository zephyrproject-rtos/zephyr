/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Polar Electro Oy
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/spinlock.h>
#include <zephyr/ztest.h>

/*
 * A fake streaming sensor. It owns its stream submission until the test
 * completes a batch. On a batch it gives the submission back to RTIO, and RTIO
 * submits it again unless the stream was canceled.
 */
struct fake_sensor_data {
	struct k_spinlock lock;
	struct rtio_iodev_sqe *stream;
	int submit_calls;
	int cancel_calls;
	rtio_sqe_handle_t cancel_handle;
};

static void fake_sensor_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct fake_sensor_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->submit_calls++;
	data->stream = iodev_sqe;
	k_spin_unlock(&data->lock, key);
}

static void fake_sensor_cancel(const struct device *dev, struct rtio *r, rtio_sqe_handle_t handle)
{
	struct fake_sensor_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	struct rtio_iodev_sqe *iodev_sqe = rtio_iodev_sqe_from_handle(r, handle);
	bool const owned = iodev_sqe != NULL && iodev_sqe == data->stream;

	data->cancel_calls++;
	data->cancel_handle = handle;
	if (owned) {
		data->stream = NULL;
	}
	k_spin_unlock(&data->lock, key);

	if (owned) {
		rtio_iodev_sqe_err(iodev_sqe, -ECANCELED);
	}
}

static int fake_sensor_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static DEVICE_API(sensor, fake_sensor_api) = {
	.submit = fake_sensor_submit,
	.cancel = fake_sensor_cancel,
};

/* The same sensor without a cancel operation. */
static DEVICE_API(sensor, fake_sensor_no_cancel_api) = {
	.submit = fake_sensor_submit,
};

static struct fake_sensor_data fake_data;
static struct fake_sensor_data fake_no_cancel_data;

DEVICE_DEFINE(fake_sensor, "fake_sensor", fake_sensor_init, NULL, &fake_data, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fake_sensor_api);
DEVICE_DEFINE(fake_sensor_no_cancel, "fake_sensor_no_cancel", fake_sensor_init, NULL,
	      &fake_no_cancel_data, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      &fake_sensor_no_cancel_api);

static struct sensor_stream_trigger stream_trigger = {
	.trigger = SENSOR_TRIG_DATA_READY,
	.opt = SENSOR_STREAM_DATA_INCLUDE,
};

static struct sensor_read_config stream_config = {
	.sensor = DEVICE_GET(fake_sensor),
	.is_streaming = true,
	.triggers = &stream_trigger,
	.count = 1,
	.max = 1,
};

static struct sensor_read_config no_cancel_config = {
	.sensor = DEVICE_GET(fake_sensor_no_cancel),
	.is_streaming = true,
	.triggers = &stream_trigger,
	.count = 1,
	.max = 1,
};

RTIO_IODEV_DEFINE(stream_iodev, &__sensor_iodev_api, &stream_config);
RTIO_IODEV_DEFINE(no_cancel_iodev, &__sensor_iodev_api, &no_cancel_config);
RTIO_DEFINE_WITH_MEMPOOL(test_rtio, 2, 4, 4, 16, 4);

/* Read one sample into the stream buffer and give the submission back to RTIO. */
static void complete_batch(struct fake_sensor_data *data)
{
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	struct rtio_iodev_sqe *iodev_sqe = data->stream;
	uint8_t *buf = NULL;
	uint32_t buf_len = 0;

	data->stream = NULL;
	k_spin_unlock(&data->lock, key);

	zassert_not_null(iodev_sqe, "the driver has no stream submission");
	zassert_ok(rtio_sqe_rx_buf(iodev_sqe, 4, 4, &buf, &buf_len));
	memset(buf, 0x5a, buf_len);
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

/* Consume the next completion and return its result. Releases its buffer. */
static int next_result(void)
{
	struct rtio_cqe *cqe = rtio_cqe_consume(&test_rtio);
	uint8_t *buf = NULL;
	uint32_t buf_len = 0;
	int result;

	zassert_not_null(cqe, "no completion");
	result = cqe->result;
	if (rtio_cqe_get_mempool_buffer(&test_rtio, cqe, &buf, &buf_len) == 0) {
		rtio_release_buffer(&test_rtio, buf, buf_len);
	}
	rtio_cqe_release(&test_rtio, cqe);
	return result;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	memset(&fake_data, 0, sizeof(fake_data));
	memset(&fake_no_cancel_data, 0, sizeof(fake_no_cancel_data));
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_is_null(rtio_cqe_consume(&test_rtio), "unexpected completion");
	zassert_equal(rtio_sqe_acquirable(&test_rtio), 2, "submission leaked");
}

ZTEST_SUITE(sensor_rtio_cancel, NULL, NULL, before, after, NULL);

/* A cancel reaches the driver with the handle of the stream. The driver ends
 * the stream that waits for its trigger at once.
 */
ZTEST(sensor_rtio_cancel, test_cancel_reaches_the_driver)
{
	rtio_sqe_handle_t handle;

	zassert_ok(sensor_stream(&stream_iodev, &test_rtio, NULL, &handle));
	zassert_equal(fake_data.submit_calls, 1);

	zassert_ok(rtio_sqe_cancel(&test_rtio, handle));
	zassert_equal(fake_data.cancel_calls, 1);
	zassert_equal(fake_data.cancel_handle, handle);
	zassert_equal(next_result(), -ECANCELED);
}

/* A running stream delivers batches and still ends once with -ECANCELED. */
ZTEST(sensor_rtio_cancel, test_graceful_cancel_of_a_running_stream)
{
	rtio_sqe_handle_t handle;

	zassert_ok(sensor_stream(&stream_iodev, &test_rtio, NULL, &handle));
	for (int i = 0; i < 3; i++) {
		complete_batch(&fake_data);
		zassert_equal(next_result(), 0, "batch %d failed", i);
	}
	zassert_equal(fake_data.submit_calls, 4, "the stream was not submitted again");

	zassert_ok(rtio_sqe_cancel(&test_rtio, handle));
	zassert_equal(next_result(), -ECANCELED);

	/* The handle of the ended stream does not reach the driver again. */
	zassert_ok(rtio_sqe_cancel(&test_rtio, handle));
	zassert_equal(fake_data.cancel_calls, 1);
}

/* Without a cancel operation, the stream ends when the driver completes its
 * next batch. RTIO then reports -ECANCELED and does not submit it again.
 */
ZTEST(sensor_rtio_cancel, test_cancel_without_a_driver_cancel_operation)
{
	rtio_sqe_handle_t handle;

	zassert_ok(sensor_stream(&no_cancel_iodev, &test_rtio, NULL, &handle));
	zassert_ok(rtio_sqe_cancel(&test_rtio, handle));
	zassert_is_null(rtio_cqe_consume(&test_rtio), "the stream ended before its batch");

	complete_batch(&fake_no_cancel_data);
	zassert_equal(next_result(), -ECANCELED);
	zassert_equal(fake_no_cancel_data.submit_calls, 1, "a canceled stream was submitted again");
}
