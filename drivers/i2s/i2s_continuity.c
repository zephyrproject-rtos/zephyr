/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2s/continuity.h>
#include <zephyr/logging/log.h>

#include <string.h>

LOG_MODULE_REGISTER(i2s_continuity, CONFIG_I2S_LOG_LEVEL);

static void cqe_callback(struct rtio *r, void *user_data)
{
	struct i2s_continuity_iodev_data *data = user_data;

	ARG_UNUSED(r);

	k_work_submit(&data->work);
}

static void consume_completions(struct i2s_continuity_iodev_data *data)
{
	struct rtio_cqe *cqe;
	struct rtio_iodev_sqe *iodev_sqe;

	while (true) {
		cqe = rtio_cqe_consume(data->r);
		if (cqe == NULL) {
			break;
		}

		/* Stream is running as I2S device iodev is completing submissions */
		data->started = true;

		/*
		 * If this is a completion of a forwarded submission, the forwarded submission is
		 * stored in the user data passed to the completion.
		 */
		iodev_sqe = cqe->userdata;

		if (iodev_sqe != NULL) {
			/* Forward the completion of the forwarded submission */
			if (cqe->result) {
				rtio_iodev_sqe_err(iodev_sqe, cqe->result);
			} else {
				rtio_iodev_sqe_ok(iodev_sqe, 0);
			}
		}

		rtio_cqe_release(data->r, cqe);

		data->sq_count--;
	}
}

static void forward_submissions(struct i2s_continuity_iodev_data *data)
{
	struct mpsc_node *node;
	struct rtio_iodev_sqe *in_iodev_sqe;
	struct rtio_sqe *in_sqe;
	struct rtio_sqe *out_sqe;

	while (true) {
		node = mpsc_pop(&data->io_q);
		if (node == NULL) {
			break;
		}

		if (data->sq_count == 0) {
			/* Submitting a submission is an implicit start condition */
			data->stop = false;

			/*
			 * The stream is not started until the I2S device iodev returns the first
			 * completion.
			 */
			data->started = false;
		}

		in_iodev_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

		out_sqe = rtio_sqe_acquire(data->r);
		if (out_sqe == NULL) {
			rtio_iodev_sqe_err(in_iodev_sqe, -ENOMEM);
			continue;
		}

		in_sqe = &in_iodev_sqe->sqe;

		/* Copy the operation and priority for the default submissions */
		data->op = in_sqe->op;
		data->prio = in_sqe->prio;

		/*
		 * Copy and strip the in-submission to just the transfer, overwriting the iodev
		 * and storing the in-submission in the out-submissions user data to be completed
		 * later.
		 */
		memcpy(out_sqe, in_sqe, sizeof(struct rtio_sqe));
		out_sqe->flags = 0;
		out_sqe->iodev = data->iodev;
		out_sqe->userdata = in_iodev_sqe;

		data->sq_count++;

		rtio_submit(data->r, 0);
	}
}

static void insert_default_submissions(struct i2s_continuity_iodev_data *data)
{
	struct rtio_sqe *sqe;

	if (data->sq_count > I2S_CONTINUITY_STREAM_SUBMISSIONS) {
		return;
	}

	for (size_t i = 0; i < I2S_CONTINUITY_STREAM_SUBMISSIONS - data->sq_count; i++) {
		sqe = rtio_sqe_acquire(data->r);

		if (sqe == NULL) {
			LOG_ERR("failed to acquire sqe");
			continue;
		}

		memset(sqe, 0, sizeof(struct rtio_sqe));

		sqe->op = data->op;
		sqe->prio = data->prio;
		sqe->iodev = data->iodev;

		switch (sqe->op) {
		case RTIO_OP_RX:
			sqe->rx.buf_len = data->buf_len;
			sqe->rx.buf = data->rx_buf;
			break;

		case RTIO_OP_TX:
			sqe->tx.buf_len = data->buf_len;
			sqe->tx.buf = data->tx_buf;
			break;

		case RTIO_OP_TXRX:
			sqe->txrx.buf_len = data->buf_len;
			sqe->txrx.tx_buf = data->tx_buf;
			sqe->txrx.rx_buf = data->rx_buf;

			break;

		default:
			break;
		}

		data->sq_count++;

		rtio_submit(data->r, 0);
	}
}

static void work_handler(struct k_work *work)
{
	struct i2s_continuity_iodev_data *data =
		CONTAINER_OF(work, struct i2s_continuity_iodev_data, work);

	consume_completions(data);
	forward_submissions(data);

	if (data->stop) {
		return;
	}

	if (!data->started) {
		return;
	}

	insert_default_submissions(data);
}

static void i2s_continuity_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_sqe *sqe = &iodev_sqe->sqe;
	const struct rtio_iodev *iodev = sqe->iodev;
	struct i2s_continuity_iodev_data *data = iodev->data;

	mpsc_push(&data->io_q, &iodev_sqe->q);
	k_work_submit(&data->work);
}

const struct rtio_iodev_api i2s_continuity_iodev_api = {
	.submit = i2s_continuity_iodev_submit,
};

void i2s_continuity_iodev_init(const struct rtio_iodev *i2s_continuity_iodev)
{
	struct i2s_continuity_iodev_data *data = i2s_continuity_iodev->data;

	rtio_set_cqe_callback(data->r, cqe_callback, (void *)data);
	mpsc_init(&data->io_q);
	k_work_init(&data->work, work_handler);
	data->sq_count = 0;
}

bool i2s_continuity_iodev_is_ready(const struct rtio_iodev *i2s_continuity_iodev)
{
	struct i2s_continuity_iodev_data *data = i2s_continuity_iodev->data;

	return i2s_is_ready_iodev(data->iodev);
}

void i2s_continuity_iodev_stop(const struct rtio_iodev *i2s_continuity_iodev)
{
	struct i2s_continuity_iodev_data *data = i2s_continuity_iodev->data;

	data->stop = true;
}
