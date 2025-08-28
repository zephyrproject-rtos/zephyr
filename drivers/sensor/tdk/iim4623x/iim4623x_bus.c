/*
 * Copyright (c) 2025 Sentry Technologies ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "iim4623x.h"
#include "iim4623x_bus.h"
#include "zephyr/sys/atomic.h"

#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>

static int iim4623x_rtio_submit_flush(struct rtio *ctx, int n_sqe)
{
	int ret;

	ret = rtio_submit(ctx, n_sqe);
	if (ret) {
		return ret;
	}

	return rtio_flush_completion_queue(ctx);
}

int iim4623x_bus_write(const struct device *dev, const uint8_t *buf, size_t len)
{
	struct iim4623x_data *data = dev->data;
	struct rtio *ctx = data->rtio.ctx;
	struct rtio_iodev *iodev = data->rtio.iodev;
	int ret;

	if (!atomic_cas(&data->busy, 0, 1)) {
		return -EBUSY;
	}

	struct rtio_sqe *wr_sqe = rtio_sqe_acquire(ctx);

	if (!wr_sqe) {
		ret = -ENOMEM;
		goto out;
	}

	rtio_sqe_prep_write(wr_sqe, iodev, RTIO_PRIO_HIGH, buf, len, NULL);

	ret = iim4623x_rtio_submit_flush(ctx, 1);

out:
	atomic_cas(&data->busy, 1, 0);
	return ret;
}

int iim4623x_bus_read(const struct device *dev, uint8_t *buf, size_t len)
{
	struct iim4623x_data *data = dev->data;
	struct rtio *ctx = data->rtio.ctx;
	struct rtio_iodev *iodev = data->rtio.iodev;
	int ret;

	if (!atomic_cas(&data->busy, 0, 1)) {
		return -EBUSY;
	}

	struct rtio_sqe *re_sqe = rtio_sqe_acquire(ctx);

	if (!re_sqe) {
		rtio_sqe_drop_all(ctx);
		ret = -ENOMEM;
		goto out;
	}

	rtio_sqe_prep_read(re_sqe, iodev, RTIO_PRIO_HIGH, buf, len, NULL);

	ret = iim4623x_rtio_submit_flush(ctx, 1);

out:
	atomic_cas(&data->busy, 1, 0);
	return ret;
}

int iim4623x_bus_write_then_read(const struct device *dev, const uint8_t *wbuf, size_t wlen,
				 uint8_t *rbuf, size_t rlen)
{
	struct iim4623x_data *data = dev->data;
	struct rtio *ctx = data->rtio.ctx;
	struct rtio_iodev *iodev = data->rtio.iodev;
	int ret;

	if (!atomic_cas(&data->busy, 0, 1)) {
		return -EBUSY;
	}

	__ASSERT(data->await_sqe == NULL, "Already awaiting completion");

	/* Acquisition order matters */
	struct rtio_sqe *wr_sqe = rtio_sqe_acquire(ctx);

	data->await_sqe = rtio_sqe_acquire(ctx);

	struct rtio_sqe *re_sqe = rtio_sqe_acquire(ctx);

	struct rtio_sqe *de_sqe = rtio_sqe_acquire(ctx);

	if (!wr_sqe || !re_sqe || !de_sqe || !data->await_sqe) {
		rtio_sqe_drop_all(ctx);
		data->await_sqe = NULL;
		ret = -ENOMEM;
		goto out;
	}

	/* Write */
	rtio_sqe_prep_write(wr_sqe, iodev, RTIO_PRIO_HIGH, wbuf, wlen, NULL);
	wr_sqe->flags |= RTIO_SQE_CHAINED;

	/* Await data-ready interrupt */
	rtio_sqe_prep_await(data->await_sqe, NULL, RTIO_PRIO_HIGH, data);
	data->await_sqe->flags |= RTIO_SQE_CHAINED;

	/* Read */
	rtio_sqe_prep_read(re_sqe, iodev, RTIO_PRIO_HIGH, rbuf, rlen, NULL);
	re_sqe->flags |= RTIO_SQE_CHAINED;

	/**
	 * Allow iim46234 to be ready for a new command
	 * Refer to datasheet 5.3.1.4 which states 0.3ms after DRDY deasserts. It seems that it
	 * takes ~3.1us from CS deassert until DRDY deasserts, so just use a single delay of >300us
	 */
	/**
	 * TODO: it would be great if the delay could be scheduled to block the rtio context from
	 * executing SQEs without also having to block the current thread
	 */
	rtio_sqe_prep_delay(de_sqe, K_USEC(400), NULL);

	ret = iim4623x_rtio_submit_flush(ctx, 4);

out:
	atomic_cas(&data->busy, 1, 0);
	return ret;
}
