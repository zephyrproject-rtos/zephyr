/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2s/fallback.h>

#define I2S_FALLBACK_THREAD_PRIORITY CONFIG_I2S_FALLBACK_THREAD_PRIORITY
#define I2S_FALLBACK_TIMEOUT K_MSEC(CONFIG_I2S_FALLBACK_TIMEOUT_MS)

static bool rx_enabled(struct i2s_fallback *fallback)
{
	return fallback->op == RTIO_OP_RX || fallback->op == RTIO_OP_TXRX;
}

static bool tx_enabled(struct i2s_fallback *fallback)
{
	return fallback->op == RTIO_OP_TX || fallback->op == RTIO_OP_TXRX;
}

static int write_tx(struct i2s_fallback *fallback,
		    struct rtio_iodev_sqe *iodev_sqe,
		    uint8_t *dst_mem)
{
	struct rtio_sqe *sqe;
	size_t buf_len;
	const uint8_t *src_mem;

	sqe = &iodev_sqe->sqe;
	buf_len = sqe->op == RTIO_OP_TX ? sqe->tx.buf_len : sqe->txrx.buf_len;
	if (buf_len != fallback->buf_len) {
		return -EINVAL;
	}

	src_mem = sqe->op == RTIO_OP_TX ? sqe->tx.buf : sqe->txrx.tx_buf;
	memcpy(dst_mem, src_mem, buf_len);
	return i2s_write(fallback->dev, dst_mem, buf_len);
}

static int consume_rx(struct i2s_fallback *fallback,
		      struct rtio_iodev_sqe *iodev_sqe,
		      uint8_t *src_mem)
{
	struct rtio_sqe *sqe;
	size_t buf_len;
	void *dst_mem;

	sqe = &iodev_sqe->sqe;
	buf_len = fallback->op == RTIO_OP_RX ? sqe->rx.buf_len : sqe->txrx.buf_len;
	if (buf_len != fallback->buf_len) {
		k_mem_slab_free(fallback->rx_slab, src_mem);
		return -EINVAL;
	}

	dst_mem = fallback->op == RTIO_OP_RX ? sqe->rx.buf : sqe->txrx.rx_buf;
	memcpy(dst_mem, src_mem, buf_len);
	k_mem_slab_free(fallback->rx_slab, src_mem);
	return 0;
}

static void submission_q_put(struct i2s_fallback *fallback,
			     struct rtio_iodev_sqe *iodev_sqe)
{
	sys_ringq_put(fallback->submission_q, &iodev_sqe);
}

static struct rtio_iodev_sqe *submission_q_get(struct i2s_fallback *fallback)
{
	struct rtio_iodev_sqe *iodev_sqe;

	sys_ringq_get(fallback->submission_q, &iodev_sqe);
	return iodev_sqe;
}

static size_t submission_q_size(struct i2s_fallback *fallback)
{
	return sys_ringq_size(fallback->submission_q);
}

static void reset_submission_q(struct i2s_fallback *fallback)
{
	sys_ringq_reset(fallback->submission_q);
}

static void reserve_tx_buf(struct i2s_fallback *fallback, uint8_t *tx_buf)
{
	uint8_t **ref;

	ARRAY_FOR_EACH(fallback->tx_buf_refs, i) {
		ref = &fallback->tx_buf_refs[i];

		if (*ref == NULL) {
			*ref = tx_buf;
			return;
		}
	}

	__ASSERT_NO_MSG(false);
}

static void free_reserved_tx_bufs(struct i2s_fallback *fallback)
{
	uint8_t **ref;

	ARRAY_FOR_EACH(fallback->tx_buf_refs, i) {
		ref = &fallback->tx_buf_refs[i];

		if (*ref != NULL) {
			k_mem_slab_free(fallback->tx_slab, *ref);
			*ref = NULL;
		}
	}
}

static int configure(struct i2s_fallback *fallback)
{
	struct i2s_rtio *ctx;
	struct rtio_iodev_sqe *iodev_sqe;
	struct rtio_sqe *sqe;
	size_t buf_len;
	const struct rtio_iodev *iodev;
	const struct i2s_iodev_data *iodev_data;
	const struct i2s_iodev_config *iodev_config;
	struct i2s_config config;
	int ret;

	ctx = fallback->ctx;
	iodev_sqe = ctx->curr;
	sqe = &iodev_sqe->sqe;

	switch (sqe->op) {
	case RTIO_OP_RX:
		buf_len = sqe->rx.buf_len;
		break;

	case RTIO_OP_TX:
		buf_len = sqe->tx.buf_len;
		break;

	case RTIO_OP_TXRX:
		buf_len = sqe->txrx.buf_len;
		break;

	default:
		return -ENOTSUP;
	}

	if (buf_len > CONFIG_I2S_FALLBACK_MAX_BUF_LEN) {
		return -ENOMEM;
	}

	iodev = sqe->iodev;
	iodev_data = iodev->data;
	iodev_config = &iodev_data->config;

	fallback->op = sqe->op;
	fallback->buf_len = buf_len;

	config.word_size = iodev_config->word_size;
	config.channels = iodev_config->channels;
	config.format = iodev_config->format;
	config.options = iodev_config->options;
	config.frame_clk_freq = iodev_config->frame_clk_freq;
	config.block_size = fallback->buf_len;
	config.timeout = CONFIG_I2S_FALLBACK_TIMEOUT_MS;

	if (sqe->op == RTIO_OP_RX || sqe->op == RTIO_OP_TXRX) {
		config.mem_slab = fallback->rx_slab;

		ret = i2s_configure(fallback->dev, I2S_DIR_RX, &config);
		if (ret) {
			return ret;
		}
	}

	if (sqe->op == RTIO_OP_TX || sqe->op == RTIO_OP_TXRX) {
		config.mem_slab = fallback->tx_slab;

		ret = i2s_configure(fallback->dev, I2S_DIR_TX, &config);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int i2s_fallback_trigger(struct i2s_fallback *fallback,
				enum i2s_trigger_cmd cmd)
{
	int ret;

	switch (fallback->op) {
	case RTIO_OP_RX:
		ret = i2s_trigger(fallback->dev, I2S_DIR_RX, cmd);
		break;

	case RTIO_OP_TX:
		ret = i2s_trigger(fallback->dev, I2S_DIR_TX, cmd);
		break;

	case RTIO_OP_TXRX:
		ret = i2s_trigger(fallback->dev, I2S_DIR_BOTH, cmd);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static void i2s_fallback_run(struct i2s_fallback *fallback)
{
	int ret;
	struct i2s_rtio *ctx;
	struct rtio_iodev_sqe *iodev_sqe;
	void *tx_mem;
	bool stream;
	void *rx_mem;
	size_t rx_mem_size;

	ret = configure(fallback);
	if (ret) {
		ctx = fallback->ctx;
		goto complete;
	}

	ctx = fallback->ctx;

	/*
	 * Preload the I2S device by adding I2S_FALLBACK_BUF_COUNT submissions to the submission
	 * queue, copying TX data from the submissions to the I2S device in case TX is enabled.
	 */
	for (size_t i = 0; i < I2S_FALLBACK_BUF_COUNT; i++) {
		iodev_sqe = ctx->curr;

		if (tx_enabled(fallback)) {
			ret = k_mem_slab_alloc(fallback->tx_slab, &tx_mem, K_NO_WAIT);
			if (ret) {
				goto drop_complete;
			}

			ret = write_tx(fallback, iodev_sqe, tx_mem);
			if (ret) {
				k_mem_slab_free(fallback->tx_slab, tx_mem);
				goto drop_complete;
			}
		}

		submission_q_put(fallback, iodev_sqe);

		if (i == I2S_FALLBACK_BUF_COUNT - 1) {
			break;
		}

		if (!i2s_rtio_continue(ctx)) {
			goto drop_complete;
		}
	}

	ret = i2s_fallback_trigger(fallback, I2S_TRIGGER_START);
	if (ret) {
		goto drop_complete;
	}

	stream = true;

	while (1) {
		if (rx_enabled(fallback)) {
			/* Wait for an RX buffer to be ready to track submission completion */
			ret = i2s_read(fallback->dev, &rx_mem, &rx_mem_size);
			if (ret) {
				goto drop_complete;
			}
		}

		if (tx_enabled(fallback)) {
			/* Wait for a TX buffer to be ready to track submission completion */
			ret = k_mem_slab_alloc(fallback->tx_slab, &tx_mem, I2S_FALLBACK_TIMEOUT);
			if (ret) {
				if (rx_enabled(fallback)) {
					k_mem_slab_free(fallback->rx_slab, rx_mem);
				}

				goto drop_complete;
			}
		}

		if (i2s_rtio_is_cancelled(ctx)) {
			ret = -ECANCELED;

			if (tx_enabled(fallback)) {
				k_mem_slab_free(fallback->tx_slab, tx_mem);
			}

			goto complete;
		}

		/*
		 * A block has been successfully transferred by the I2S device. Get the oldest
		 * submission which matches the newly completed block and finalize it.
		 */
		iodev_sqe = submission_q_get(fallback);

		if (rx_enabled(fallback)) {
			/*
			 * Copy the I2S device RX buffer content to the user provided buffer and
			 * free it.
			 */
			ret = consume_rx(fallback, iodev_sqe, rx_mem);
			if (ret) {
				if (tx_enabled(fallback)) {
					k_mem_slab_free(fallback->tx_slab, tx_mem);
				}

				goto drop_complete;
			}
		}

		if (submission_q_size(fallback) == 1 && !tx_enabled(fallback)) {
			/*
			 * Stop the I2S device after this submission. In case TX is enabled the I2S
			 * device will already be draining and will stop on its own after this
			 * submission.
			 */
			ret = i2s_fallback_trigger(fallback, I2S_TRIGGER_STOP);
			if (ret) {
				goto drop_complete;
			}
		}

		if (submission_q_size(fallback) == 0) {
			if (tx_enabled(fallback)) {
				k_mem_slab_free(fallback->tx_slab, tx_mem);
			}

			break;
		}

		/*
		 * Oldest submission is now complete. We will never complete the last submission
		 * here so it will always return false.
		 */
		(void)i2s_rtio_complete(ctx, 0);

		if (!stream) {
			/* Stream is draining */
			if (tx_enabled(fallback)) {
				/*
				 * We need to keep the tx slab fully allocated to wait for it to
				 * become available.
				 */
				reserve_tx_buf(fallback, tx_mem);
			}

			continue;
		}

		/* The I2S device is ready to queue up a new transfer */
		stream = i2s_rtio_continue(ctx);

		if (!stream) {
			/* Stream shall start draining */
			if (tx_enabled(fallback)) {
				/*
				 * We need to keep the tx slab fully allocated to wait for it to
				 * become available.
				 */
				reserve_tx_buf(fallback, tx_mem);

				/*
				 * Start draining to nicely stop the stream rather than
				 * perpousfully underrunning the I2S device buffer.
				 */
				ret = i2s_fallback_trigger(fallback, I2S_TRIGGER_DRAIN);
				if (ret) {
					goto drop_complete;
				}
			}

			continue;
		}

		/*
		 * Add the newest submission to the submission queue, copying TX data from the
		 * submissions to the I2S device in case TX is enabled.
		 */
		iodev_sqe = ctx->curr;

		if (tx_enabled(fallback)) {
			ret = write_tx(fallback, iodev_sqe, tx_mem);
			if (ret) {
				k_mem_slab_free(fallback->tx_slab, tx_mem);
				goto drop_complete;
			}
		}

		submission_q_put(fallback, iodev_sqe);
	}

drop_complete:
	if (i2s_fallback_trigger(fallback, I2S_TRIGGER_DROP)) {
		(void)i2s_fallback_trigger(fallback, I2S_TRIGGER_PREPARE);
	}

complete:
	/* Complete all but the newest submission in the IO queue */
	for (size_t i = 1; i < submission_q_size(fallback); i++) {
		i2s_rtio_complete(ctx, ret);
	}

	reset_submission_q(fallback);
	free_reserved_tx_bufs(fallback);

	/* Complete the newest submission */
	if (i2s_rtio_complete(ctx, ret)) {
		/* New stream ready to start */
		k_sem_give(&fallback->start_sem);
	}
}

static void routine(void *p1, void *p2, void *p3)
{
	struct i2s_fallback *fallback;

	ARGS_UNUSED(p2, p3);

	fallback = p1;

	while (1) {
		k_sem_take(&fallback->start_sem, K_FOREVER);
		i2s_fallback_run(fallback);
	}
}

void i2s_fallback_init(struct i2s_fallback *fallback, const struct device *dev)
{
	fallback->dev = dev;
	i2s_rtio_init(fallback->ctx);
	k_sem_init(&fallback->start_sem, 0, 1);
	k_thread_create(&fallback->thread,
			fallback->thread_stack,
			K_KERNEL_STACK_SIZEOF(fallback->thread_stack),
			routine,
			fallback,
			NULL,
			NULL,
			I2S_FALLBACK_THREAD_PRIORITY,
			0,
			K_NO_WAIT);
}

void i2s_fallback_iodev_submit(struct i2s_fallback *fallback, struct rtio_iodev_sqe *iodev_sqe)
{
	if (i2s_rtio_submit(fallback->ctx, iodev_sqe)) {
		k_sem_give(&fallback->start_sem);
	}
}

void i2s_fallback_iodev_cancel(struct i2s_fallback *fallback)
{
	if (i2s_rtio_cancel(fallback->ctx)) {
		i2s_fallback_trigger(fallback, I2S_TRIGGER_DROP);
	}
}
