/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2s/fallback.h>

#define I2S_FALLBACK_THREAD_PRIORITY CONFIG_I2S_FALLBACK_THREAD_PRIORITY
#define I2S_FALLBACK_TIMEOUT K_MSEC(CONFIG_I2S_FALLBACK_TIMEOUT_MS)

static bool rx_enabled(struct i2s_fallback_iodev_data *iodev_data)
{
	return iodev_data->op == RTIO_OP_RX || iodev_data->op == RTIO_OP_TXRX;
}

static bool tx_enabled(struct i2s_fallback_iodev_data *iodev_data)
{
	return iodev_data->op == RTIO_OP_TX || iodev_data->op == RTIO_OP_TXRX;
}

static int write_tx(struct i2s_fallback_iodev_data *iodev_data,
		    struct rtio_iodev_sqe *iodev_sqe,
		    uint8_t *dst_mem)
{
	struct rtio_sqe *sqe = &iodev_sqe->sqe;
	const uint8_t *src_mem;
	size_t buf_len;

	buf_len = sqe->op == RTIO_OP_TX ? sqe->tx.buf_len : sqe->txrx.buf_len;
	src_mem = sqe->op == RTIO_OP_TX ? sqe->tx.buf : sqe->txrx.tx_buf;
	memcpy(dst_mem, src_mem, buf_len);
	return i2s_write(iodev_data->dev, dst_mem, buf_len);
}

static void consume_rx(struct i2s_fallback_iodev_data *iodev_data,
		       struct rtio_iodev_sqe *iodev_sqe,
		       uint8_t *src_mem)
{
	struct rtio_sqe *sqe = &iodev_sqe->sqe;
	size_t buf_len;
	void *dst_mem;

	buf_len = iodev_data->op == RTIO_OP_RX ? sqe->rx.buf_len : sqe->txrx.buf_len;
	dst_mem = iodev_data->op == RTIO_OP_RX ? sqe->rx.buf : sqe->txrx.rx_buf;
	memcpy(dst_mem, src_mem, buf_len);
	k_mem_slab_free(iodev_data->rx_slab, src_mem);
}

static void submission_q_put(struct i2s_fallback_iodev_data *iodev_data,
			     struct rtio_iodev_sqe *iodev_sqe)
{
	sys_ringq_put(iodev_data->submission_q, &iodev_sqe);
}

static struct rtio_iodev_sqe *submission_q_get(struct i2s_fallback_iodev_data *iodev_data)
{
	struct rtio_iodev_sqe *iodev_sqe;

	sys_ringq_get(iodev_data->submission_q, &iodev_sqe);
	return iodev_sqe;
}

static size_t submission_q_size(struct i2s_fallback_iodev_data *iodev_data)
{
	return sys_ringq_size(iodev_data->submission_q);
}

static void reset_submission_q(struct i2s_fallback_iodev_data *iodev_data)
{
	return sys_ringq_reset(iodev_data->submission_q);
}

static void reserve_tx_buf(struct i2s_fallback_iodev_data *iodev_data, uint8_t *tx_buf)
{
	uint8_t **ref;

	ARRAY_FOR_EACH(iodev_data->tx_buf_refs, i) {
		ref = &iodev_data->tx_buf_refs[i];

		if (*ref == NULL) {
			*ref = tx_buf;
			return;
		}
	}

	__ASSERT_NO_MSG(false);
}

static void free_reserved_tx_bufs(struct i2s_fallback_iodev_data *iodev_data)
{
	uint8_t **ref;

	ARRAY_FOR_EACH(iodev_data->tx_buf_refs, i) {
		ref = &iodev_data->tx_buf_refs[i];

		if (*ref != NULL) {
			k_mem_slab_free(iodev_data->tx_slab, *ref);
			*ref = NULL;
		}
	}
}

static int configure(struct i2s_fallback_iodev_data *iodev_data)
{
	struct i2s_rtio *ctx = iodev_data->ctx;
	struct rtio_iodev_sqe *iodev_sqe = ctx->curr;
	struct rtio_sqe *sqe = &iodev_sqe->sqe;
	const struct i2s_iodev_config *iodev_config = &iodev_data->config;
	int ret;
	struct i2s_config config;

	switch (sqe->op) {
	case RTIO_OP_RX:
	case RTIO_OP_TX:
	case RTIO_OP_TXRX:
		ret = 0;
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	if (ret) {
		return ret;
	}

	config.word_size = iodev_config->word_size;
	config.channels = iodev_config->channels;
	config.format = iodev_config->format;
	config.options = iodev_config->options;
	config.frame_clk_freq = iodev_config->frame_clk_freq;
	config.block_size = iodev_data->max_buf_len;
	config.timeout = CONFIG_I2S_FALLBACK_TIMEOUT_MS;

	if (sqe->op == RTIO_OP_RX || sqe->op == RTIO_OP_TXRX) {
		config.mem_slab = iodev_data->rx_slab;

		ret = i2s_configure(iodev_data->dev, I2S_DIR_RX, &config);
		if (ret) {
			return ret;
		}
	}

	if (sqe->op == RTIO_OP_TX || sqe->op == RTIO_OP_TXRX) {
		config.mem_slab = iodev_data->tx_slab;

		ret = i2s_configure(iodev_data->dev, I2S_DIR_TX, &config);
		if (ret) {
			return ret;
		}
	}

	iodev_data->op = sqe->op;
	return 0;
}

static int i2s_fallback_iodev_trigger(struct i2s_fallback_iodev_data *iodev_data,
				      enum i2s_trigger_cmd cmd)
{
	int ret;

	switch (iodev_data->op) {
	case RTIO_OP_RX:
		ret = i2s_trigger(iodev_data->dev, I2S_DIR_RX, cmd);
		break;

	case RTIO_OP_TX:
		ret = i2s_trigger(iodev_data->dev, I2S_DIR_TX, cmd);
		break;

	case RTIO_OP_TXRX:
		ret = i2s_trigger(iodev_data->dev, I2S_DIR_BOTH, cmd);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static void i2s_fallback_iodev_run(struct i2s_fallback_iodev_data *iodev_data)
{
	struct i2s_rtio *ctx = iodev_data->ctx;
	int ret;
	void *tx_mem;
	void *rx_mem;
	size_t rx_mem_size;
	struct rtio_iodev_sqe *iodev_sqe;
	bool stream;

	ret = configure(iodev_data);
	if (ret) {
		goto complete;
	}

	/*
	 * Preload the I2S device by adding I2S_FALLBACK_BUF_COUNT submissions to the submission
	 * queue, copying TX data from the submissions to the I2S device in case TX is enabled.
	 */
	for (size_t i = 0; i < I2S_FALLBACK_BUF_COUNT; i++) {
		iodev_sqe = ctx->curr;

		if (tx_enabled(iodev_data)) {
			ret = k_mem_slab_alloc(iodev_data->tx_slab, &tx_mem, K_NO_WAIT);
			if (ret) {
				goto drop_complete;
			}

			ret = write_tx(iodev_data, iodev_sqe, tx_mem);
			if (ret) {
				k_mem_slab_free(iodev_data->tx_slab, tx_mem);
				goto drop_complete;
			}
		}

		submission_q_put(iodev_data, iodev_sqe);

		if (i == I2S_FALLBACK_BUF_COUNT - 1) {
			break;
		}

		if (!i2s_rtio_continue(ctx)) {
			goto drop_complete;
		}
	}

	ret = i2s_fallback_iodev_trigger(iodev_data, I2S_TRIGGER_START);
	if (ret) {
		goto drop_complete;
	}

	stream = true;

	while (1) {
		if (rx_enabled(iodev_data)) {
			/* Wait for an RX buffer to be ready to track submission completion */
			ret = i2s_read(iodev_data->dev, &rx_mem, &rx_mem_size);
			if (ret) {
				goto drop_complete;
			}
		}

		if (tx_enabled(iodev_data)) {
			/* Wait for a TX buffer to be ready to track submission completion */
			ret = k_mem_slab_alloc(iodev_data->tx_slab, &tx_mem, I2S_FALLBACK_TIMEOUT);
			if (ret) {
				if (rx_enabled(iodev_data)) {
					k_mem_slab_free(iodev_data->rx_slab, rx_mem);
				}

				goto drop_complete;
			}
		}

		/*
		 * A block has been successfully transferred by the I2S device. Get the oldest
		 * submission which matches the newly completed block and finalize it.
		 */
		iodev_sqe = submission_q_get(iodev_data);

		if (rx_enabled(iodev_data)) {
			/*
			 * Copy the I2S device RX buffer content to the user provided buffer and
			 * free it.
			 */
			consume_rx(iodev_data, iodev_sqe, rx_mem);
		}

		if (submission_q_size(iodev_data) == 1 && !tx_enabled(iodev_data)) {
			/*
			 * Stop the I2S device after this submission. In case TX is enabled the I2S
			 * device will already be draining and will stop on its own after this
			 * submission.
			 */
			ret = i2s_fallback_iodev_trigger(iodev_data, I2S_TRIGGER_STOP);
			if (ret) {
				goto drop_complete;
			}
		}

		if (submission_q_size(iodev_data) == 0) {
			break;
		}

		/*
		 * Oldest submission is now complete. We will never complete the last submission
		 * here so it will always return false.
		 */
		(void)i2s_rtio_complete(iodev_data->ctx, 0);

		if (!stream) {
			/* Stream is draining */
			if (tx_enabled(iodev_data)) {
				/*
				 * We need to keep the tx slab fully allocated to wait for it to
				 * become available.
				 */
				reserve_tx_buf(iodev_data, tx_mem);
			}

			continue;
		}

		/* The I2S device is ready to queue up a new transfer */
		stream = i2s_rtio_continue(iodev_data->ctx);

		if (!stream) {
			/* Stream shall start draining */
			if (tx_enabled(iodev_data)) {
				/*
				 * We need to keep the tx slab fully allocated to wait for it to
				 * become available.
				 */
				reserve_tx_buf(iodev_data, tx_mem);

				/*
				 * Start draining to nicely stop the stream rather perpousfully
				 * underrunning the I2S device buffer.
				 */
				ret = i2s_fallback_iodev_trigger(iodev_data, I2S_TRIGGER_DRAIN);
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

		if (tx_enabled(iodev_data)) {
			ret = write_tx(iodev_data, iodev_sqe, tx_mem);
			if (ret) {
				k_mem_slab_free(iodev_data->tx_slab, tx_mem);
				goto drop_complete;
			}
		}

		submission_q_put(iodev_data, iodev_sqe);
	}

drop_complete:
	ret = i2s_fallback_iodev_trigger(iodev_data, I2S_TRIGGER_DROP);
	if (ret) {
		(void)i2s_fallback_iodev_trigger(iodev_data, I2S_TRIGGER_PREPARE);
	}

complete:
	/* Complete all but the newest submission in the IO queue */
	for (size_t i = 1; i < submission_q_size(iodev_data); i++) {
		i2s_rtio_complete(ctx, ret);
	}

	reset_submission_q(iodev_data);
	free_reserved_tx_bufs(iodev_data);

	/* Complete the newest submission */
	if (i2s_rtio_complete(ctx, ret)) {
		/* New stream ready to start */
		k_sem_give(&iodev_data->start_sem);
	}
}

static void routine(void *p1, void *p2, void *p3)
{
	struct i2s_fallback_iodev_data *iodev_data = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		k_sem_take(&iodev_data->start_sem, K_FOREVER);
		i2s_fallback_iodev_run(iodev_data);
	}
}

static void i2s_fallback_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	struct i2s_fallback_iodev_data *iodev_data = iodev_sqe->sqe.iodev->data;

	if (i2s_rtio_submit(iodev_data->ctx, iodev_sqe)) {
		k_sem_give(&iodev_data->start_sem);
	}
}

const struct rtio_iodev_api i2s_fallback_iodev_api = {
	.submit = i2s_fallback_iodev_submit,
};

void i2s_fallback_iodev_init(struct rtio_iodev *iodev)
{
	struct i2s_fallback_iodev_data *iodev_data = iodev->data;

	i2s_rtio_init(iodev_data->ctx);
	k_sem_init(&iodev_data->start_sem, 0, 1);
	k_thread_create(&iodev_data->thread,
			iodev_data->thread_stack,
			K_KERNEL_STACK_SIZEOF(iodev_data->thread_stack),
			routine,
			iodev_data,
			NULL,
			NULL,
			I2S_FALLBACK_THREAD_PRIORITY,
			0,
			K_NO_WAIT);
}

bool i2s_fallback_iodev_is_ready(const struct rtio_iodev *iodev)
{
	return i2s_is_ready_iodev(iodev);
}
