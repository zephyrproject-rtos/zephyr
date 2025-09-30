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

int iim4623x_bus_prep_write(const struct device *dev, const uint8_t *buf, size_t len,
			    struct rtio_sqe **cb_sqe)
{
	struct iim4623x_data *data = dev->data;
	struct rtio_sqe *wr_sqe = rtio_sqe_acquire(data->rtio.ctx);

	if (!wr_sqe) {
		return -ENOMEM;
	}

	rtio_sqe_prep_write(wr_sqe, data->rtio.iodev, RTIO_PRIO_HIGH, buf, len, NULL);

	if (!cb_sqe) {
		return 1;
	}

	wr_sqe->flags |= RTIO_SQE_CHAINED;

	*cb_sqe = rtio_sqe_acquire(data->rtio.ctx);
	if (!(*cb_sqe)) {
		rtio_sqe_drop_all(data->rtio.ctx);
		return -ENOMEM;
	}

	return 2;
}

int iim4623x_bus_prep_read(const struct device *dev, uint8_t *buf, size_t len,
			   struct rtio_sqe **cb_sqe)
{
	struct iim4623x_data *data = dev->data;
	struct rtio_sqe *re_sqe = rtio_sqe_acquire(data->rtio.ctx);

	if (!re_sqe) {
		return -ENOMEM;
	}

	rtio_sqe_prep_read(re_sqe, data->rtio.iodev, RTIO_PRIO_HIGH, buf, len, NULL);

	if (!cb_sqe) {
		return 1;
	}

	re_sqe->flags |= RTIO_SQE_CHAINED;

	*cb_sqe = rtio_sqe_acquire(data->rtio.ctx);
	if (!(*cb_sqe)) {
		rtio_sqe_drop_all(data->rtio.ctx);
		return -ENOMEM;
	}

	return 2;
}

int iim4623x_bus_prep_write_read(const struct device *dev, const uint8_t *wbuf, size_t wlen,
				 uint8_t *rbuf, size_t rlen, struct rtio_sqe **cb_sqe)
{
	struct iim4623x_data *data = dev->data;
	struct rtio_iodev *iodev = data->rtio.iodev;
	struct rtio *ctx = data->rtio.ctx;

	/* Acquisition order matters */
	struct rtio_sqe *wr_sqe = rtio_sqe_acquire(ctx);

	data->await_sqe = rtio_sqe_acquire(ctx);

	struct rtio_sqe *re_sqe = rtio_sqe_acquire(ctx);

	if (!wr_sqe || !re_sqe || !data->await_sqe) {
		rtio_sqe_drop_all(ctx);
		data->await_sqe = NULL;
		return -ENOMEM;
	}

	/* Write */
	rtio_sqe_prep_write(wr_sqe, iodev, RTIO_PRIO_HIGH, wbuf, wlen, NULL);
	wr_sqe->flags |= RTIO_SQE_CHAINED;

	/* Await data-ready interrupt */
	rtio_sqe_prep_await(data->await_sqe, NULL, RTIO_PRIO_HIGH, data);
	data->await_sqe->flags |= RTIO_SQE_CHAINED;

	/* Read */
	rtio_sqe_prep_read(re_sqe, iodev, RTIO_PRIO_HIGH, rbuf, rlen, NULL);

	if (!cb_sqe) {
		return 3;
	}

	re_sqe->flags |= RTIO_SQE_CHAINED;

	*cb_sqe = rtio_sqe_acquire(ctx);
	if (!(*cb_sqe)) {
		rtio_sqe_drop_all(ctx);
		return -ENOMEM;
	}

	return 4;
}

int iim4623x_bus_write_then_read(const struct device *dev, const uint8_t *wbuf, size_t wlen,
				 uint8_t *rbuf, size_t rlen)
{
	struct iim4623x_data *data = dev->data;
	int n_sqe;
	int ret;

	__ASSERT(data->await_sqe == NULL, "Already awaiting completion");

	n_sqe = iim4623x_bus_prep_write_read(dev, wbuf, wlen, rbuf, rlen, NULL);
	if (n_sqe < 0) {
		return n_sqe;
	}

	if (!atomic_cas(&data->busy, 0, 1)) {
		return -EBUSY;
	}

	ret = iim4623x_rtio_submit_flush(data->rtio.ctx, n_sqe);

	atomic_cas(&data->busy, 1, 0);

	return ret;
}
