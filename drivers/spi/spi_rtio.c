/*
 * Copyright (c) 2023 Intel Corporation
 * Copyright (c) 2024 Croxel Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/rtio/work.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/sys/mpsc_lockfree.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_rtio, CONFIG_SPI_LOG_LEVEL);

const struct rtio_iodev_api spi_iodev_api = {
	.submit = spi_iodev_submit,
};

static void spi_rtio_iodev_default_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	struct spi_dt_spec *dt_spec = iodev_sqe->sqe.iodev->data;
	const struct device *dev = dt_spec->bus;
	uint8_t num_msgs = 0;
	int err = 0;

	LOG_DBG("Sync RTIO work item for: %p", (void *)dev);

	/** Take care of Multi-submissions transactions in the same context.
	 * This guarantees that linked items will be consumed in the expected
	 * order, regardless pending items in the workqueue.
	 */
	struct rtio_iodev_sqe *txn_head = iodev_sqe;
	struct rtio_iodev_sqe *txn_curr = iodev_sqe;

	/* We allocate the spi_buf's on the stack, to do so
	 * the count of messages needs to be determined to
	 * ensure we don't go over the statically sized array.
	 */
	do {
		switch (txn_curr->sqe.op) {
		case RTIO_OP_RX:
		case RTIO_OP_TX:
		case RTIO_OP_TINY_TX:
		case RTIO_OP_TXRX:
			num_msgs++;
			break;
		default:
			LOG_ERR("Invalid op code %d for submission %p", txn_curr->sqe.op,
				(void *)&txn_curr->sqe);
			err = -EIO;
			break;
		}
		txn_curr = rtio_txn_next(txn_curr);
	} while (err == 0 && txn_curr != NULL);

	if (err != 0) {
		rtio_iodev_sqe_err(txn_head, err);
		return;
	}

	/* Allocate msgs on the stack, MISRA doesn't like VLAs so we need a statically
	 * sized array here. It's pretty unlikely we have more than 4 spi messages
	 * in a transaction as we typically would only have 2, one to write a
	 * register address, and another to read/write the register into an array
	 */
	if (num_msgs > CONFIG_SPI_RTIO_FALLBACK_MSGS) {
		LOG_ERR("At most CONFIG_SPI_RTIO_FALLBACK_MSGS"
			" submissions in a transaction are"
			" allowed in the default handler");
		rtio_iodev_sqe_err(txn_head, -ENOMEM);
		return;
	}

	struct spi_buf tx_bufs[CONFIG_SPI_RTIO_FALLBACK_MSGS];
	struct spi_buf rx_bufs[CONFIG_SPI_RTIO_FALLBACK_MSGS];
	struct spi_buf_set tx_buf_set = {
		.buffers = tx_bufs,
		.count = num_msgs,
	};
	struct spi_buf_set rx_buf_set = {
		.buffers = rx_bufs,
		.count = num_msgs,
	};

	txn_curr = txn_head;

	for (size_t i = 0 ; i < num_msgs ; i++) {
		struct rtio_sqe *sqe = &txn_curr->sqe;

		switch (sqe->op) {
		case RTIO_OP_RX:
			rx_bufs[i].buf = sqe->rx.buf;
			rx_bufs[i].len = sqe->rx.buf_len;
			tx_bufs[i].buf = NULL;
			tx_bufs[i].len = sqe->rx.buf_len;
			break;
		case RTIO_OP_TX:
			rx_bufs[i].buf = NULL;
			rx_bufs[i].len = sqe->tx.buf_len;
			tx_bufs[i].buf = (uint8_t *)sqe->tx.buf;
			tx_bufs[i].len = sqe->tx.buf_len;
			break;
		case RTIO_OP_TINY_TX:
			rx_bufs[i].buf = NULL;
			rx_bufs[i].len = sqe->tiny_tx.buf_len;
			tx_bufs[i].buf = (uint8_t *)sqe->tiny_tx.buf;
			tx_bufs[i].len = sqe->tiny_tx.buf_len;
			break;
		case RTIO_OP_TXRX:
			rx_bufs[i].buf = sqe->txrx.rx_buf;
			rx_bufs[i].len = sqe->txrx.buf_len;
			tx_bufs[i].buf = (uint8_t *)sqe->txrx.tx_buf;
			tx_bufs[i].len = sqe->txrx.buf_len;
			break;
		default:
			err = -EIO;
			break;
		}

		txn_curr = rtio_txn_next(txn_curr);
	}

	if (err == 0) {
		__ASSERT_NO_MSG(num_msgs > 0);
		err = spi_transceive_dt(dt_spec, &tx_buf_set, &rx_buf_set);
	}

	if (err != 0) {
		rtio_iodev_sqe_err(txn_head, err);
	} else {
		rtio_iodev_sqe_ok(txn_head, 0);
	}
}

void spi_rtio_iodev_default_submit(const struct device *dev,
				   struct rtio_iodev_sqe *iodev_sqe)
{
	LOG_DBG("Executing fallback for dev: %p, sqe: %p", (void *)dev, (void *)iodev_sqe);

	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed. Consider to increase "
			"CONFIG_RTIO_WORKQ_POOL_ITEMS.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, spi_rtio_iodev_default_submit_sync);
}

/**
 * @brief Copy the tx_bufs and rx_bufs into a set of RTIO requests
 *
 * @param[in] r rtio context
 * @param[in] iodev iodev to transceive with
 * @param[in] tx_bufs transmit buffer set
 * @param[in] rx_bufs receive buffer set
 * @param[out] last_sqe last sqe submitted, NULL if not enough memory
 *
 * @retval Number of submission queue entries
 * @retval -ENOMEM out of memory
 */
int spi_rtio_copy(struct rtio *r,
		  struct rtio_iodev *iodev,
		  const struct spi_buf_set *tx_bufs,
		  const struct spi_buf_set *rx_bufs,
		  bool no_response,
		  struct rtio_sqe **last_sqe)
{
	int ret = 0;
	size_t tx_count = tx_bufs ? tx_bufs->count : 0;
	size_t rx_count = rx_bufs ? rx_bufs->count : 0;

	uint32_t tx = 0, tx_len = 0;
	uint32_t rx = 0, rx_len = 0;
	uint8_t *tx_buf, *rx_buf;

	struct rtio_sqe *sqe = NULL;

	if (tx < tx_count) {
		tx_buf = tx_bufs->buffers[tx].buf;
		tx_len = tx_bufs->buffers[tx].len;
	} else {
		tx_buf = NULL;
		tx_len = rx_bufs->buffers[rx].len;
	}

	if (rx < rx_count) {
		rx_buf = rx_bufs->buffers[rx].buf;
		rx_len = rx_bufs->buffers[rx].len;
	} else {
		rx_buf = NULL;
		rx_len = (tx_bufs != NULL && tx < tx_count) ? tx_bufs->buffers[tx].len : 0;
	}


	while ((tx < tx_count || rx < rx_count) && (tx_len > 0 || rx_len > 0)) {
		sqe = rtio_sqe_acquire(r);

		if (sqe == NULL) {
			ret = -ENOMEM;
			rtio_sqe_drop_all(r);
			goto out;
		}

		ret++;

		/* If tx/rx len are same, we can do a simple transceive */
		if (tx_len == rx_len) {
			if (tx_buf == NULL) {
				rtio_sqe_prep_read(sqe, iodev, RTIO_PRIO_NORM,
						   rx_buf, rx_len, NULL);
			} else if (rx_buf == NULL) {
				rtio_sqe_prep_write(sqe, iodev, RTIO_PRIO_NORM,
						    tx_buf, tx_len, NULL);
			} else {
				rtio_sqe_prep_transceive(sqe, iodev, RTIO_PRIO_NORM,
							 tx_buf, rx_buf, rx_len, NULL);
			}
			tx++;
			rx++;
			if (rx < rx_count) {
				rx_buf = rx_bufs->buffers[rx].buf;
				rx_len = rx_bufs->buffers[rx].len;
			} else {
				rx_buf = NULL;
				rx_len = 0;
			}
			if (tx < tx_count) {
				tx_buf = tx_bufs->buffers[tx].buf;
				tx_len = tx_bufs->buffers[tx].len;
			} else {
				tx_buf = NULL;
				tx_len = 0;
			}
		} else if (tx_len == 0) {
			rtio_sqe_prep_read(sqe, iodev, RTIO_PRIO_NORM,
					   (uint8_t *)rx_buf,
					   (uint32_t)rx_len,
					   NULL);
			rx++;
			if (rx < rx_count) {
				rx_buf = rx_bufs->buffers[rx].buf;
				rx_len = rx_bufs->buffers[rx].len;
			} else {
				rx_buf = NULL;
				rx_len = 0;
			}
		} else if (rx_len == 0) {
			rtio_sqe_prep_write(sqe, iodev, RTIO_PRIO_NORM,
					    (uint8_t *)tx_buf,
					    (uint32_t)tx_len,
					    NULL);
			tx++;
			if (tx < tx_count) {
				tx_buf = tx_bufs->buffers[tx].buf;
				tx_len = tx_bufs->buffers[tx].len;
			} else {
				tx_buf = NULL;
				tx_len = 0;
			}
		} else if (tx_len > rx_len) {
			if (rx_buf) {
				rtio_sqe_prep_transceive(sqe, iodev, RTIO_PRIO_NORM,
							 (uint8_t *)tx_buf,
							 (uint8_t *)rx_buf,
							 (uint32_t)rx_len,
							 NULL);
			} else {
				rtio_sqe_prep_write(sqe, iodev, RTIO_PRIO_NORM,
						    (uint8_t *)tx_buf,
						    (uint32_t)rx_len,
						    NULL);
			}

			tx_len -= rx_len;
			tx_buf += rx_len;
			rx++;
			if (rx < rx_count) {
				rx_buf = rx_bufs->buffers[rx].buf;
				rx_len = rx_bufs->buffers[rx].len;
			} else {
				rx_buf = NULL;
				rx_len = tx_len;
			}
		} else if (rx_len > tx_len) {
			if (tx_buf) {
				rtio_sqe_prep_transceive(sqe, iodev, RTIO_PRIO_NORM,
							 (uint8_t *)tx_buf,
							 (uint8_t *)rx_buf,
							 (uint32_t)tx_len,
							 NULL);
			} else {
				rtio_sqe_prep_read(sqe, iodev, RTIO_PRIO_NORM,
						   (uint8_t *)rx_buf,
						   (uint32_t)tx_len,
						   NULL);
			}

			rx_len -= tx_len;
			rx_buf += tx_len;
			tx++;
			if (tx < tx_count) {
				tx_buf = tx_bufs->buffers[tx].buf;
				tx_len = tx_bufs->buffers[tx].len;
			} else {
				tx_buf = NULL;
				tx_len = rx_len;
			}
		} else {
			__ASSERT(false, "Invalid %s state", __func__);
		}

		sqe->flags = RTIO_SQE_TRANSACTION;

		if (no_response) {
			sqe->flags |= RTIO_SQE_NO_RESPONSE;
		}
	}

	if (sqe != NULL) {
		sqe->flags = no_response ? RTIO_SQE_NO_RESPONSE : 0;
		*last_sqe = sqe;
	}

out:
	return ret;
}

/**
 * @brief Lock the SPI RTIO spinlock
 *
 * This is used internally for controlling the SPI RTIO context, and is
 * exposed to the user as it's required for safely implementing
 * iodev_start API, specific to each driver.
 *
 * @param ctx SPI RTIO context
 *
 * @retval Spinlock key
 */
static inline k_spinlock_key_t spi_spin_lock(struct spi_rtio *ctx)
{
	return k_spin_lock(&ctx->lock);
}

/**
 * @brief Unlock the previously obtained SPI RTIO spinlock
 *
 * @param ctx SPI RTIO context
 * @param key Spinlock key
 */
static inline void spi_spin_unlock(struct spi_rtio *ctx, k_spinlock_key_t key)
{
	k_spin_unlock(&ctx->lock, key);
}

/** Lock the shared rtio context used for fallback implementations */
static inline void spi_r_lock(struct spi_rtio *ctx)
{
	(void)k_sem_take(&ctx->r_lock, K_FOREVER);
}

/** Unlock the shared rtio context used for fallback implementations */
static inline void spi_r_unlock(struct spi_rtio *ctx)
{
	k_sem_give(&ctx->r_lock);
}

void spi_rtio_init(struct spi_rtio *ctx,
		   const struct device *dev)
{
	mpsc_init(&ctx->io_q);
	ctx->txn_head = NULL;
	ctx->txn_curr = NULL;
	ctx->dt_spec.bus = dev;
	ctx->iodev.data = &ctx->dt_spec;
	ctx->iodev.api = &spi_iodev_api;
	k_sem_init(&ctx->r_lock, 1, 1);
}

/**
 * @private
 * @brief Setup the next transaction (could be a single op) if needed
 *
 * @retval true New transaction to start with the hardware is setup
 * @retval false No new transaction to start
 */
static bool spi_rtio_next(struct spi_rtio *ctx, bool completion)
{
	k_spinlock_key_t key = spi_spin_lock(ctx);

	if (!completion && ctx->txn_curr != NULL) {
		spi_spin_unlock(ctx, key);
		return false;
	}

	struct mpsc_node *next = mpsc_pop(&ctx->io_q);

	if (next != NULL) {
		struct rtio_iodev_sqe *next_sqe = CONTAINER_OF(next, struct rtio_iodev_sqe, q);

		ctx->txn_head = next_sqe;
		ctx->txn_curr = next_sqe;
	} else {
		ctx->txn_head = NULL;
		ctx->txn_curr = NULL;
	}

	spi_spin_unlock(ctx, key);

	return (ctx->txn_curr != NULL);
}

bool spi_rtio_complete(struct spi_rtio *ctx, int status)
{
	struct rtio_iodev_sqe *txn_head = ctx->txn_head;
	bool result;

	result = spi_rtio_next(ctx, true);

	if (status < 0) {
		rtio_iodev_sqe_err(txn_head, status);
	} else {
		rtio_iodev_sqe_ok(txn_head, status);
	}

	return result;
}

bool spi_rtio_submit(struct spi_rtio *ctx,
		     struct rtio_iodev_sqe *iodev_sqe)
{
	/** Done */
	mpsc_push(&ctx->io_q, &iodev_sqe->q);
	return spi_rtio_next(ctx, false);
}

int spi_rtio_transceive(struct spi_rtio *ctx,
			const struct spi_config *config,
			const struct spi_buf_set *tx_bufs,
			const struct spi_buf_set *rx_bufs)
{
	struct spi_dt_spec *dt_spec = &ctx->dt_spec;
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	int err = 0;
	int ret;

	if (tx_bufs == NULL && rx_bufs == NULL) {
		return -EINVAL;
	}

	spi_r_lock(ctx);

	dt_spec->config = *config;

	ret = spi_rtio_copy(ctx->r, &ctx->iodev, tx_bufs, rx_bufs, false, &sqe);
	if (ret < 0) {
		spi_r_unlock(ctx);
		return ret;
	}

	/** Submit request and wait */
	rtio_submit(ctx->r, ret);

	while (ret > 0) {
		cqe = rtio_cqe_consume(ctx->r);
		if (cqe == NULL) {
			err = -EIO;
			break;
		}

		if (cqe->result < 0) {
			err = cqe->result;
		}

		rtio_cqe_release(ctx->r, cqe);
		ret--;
	}

	spi_r_unlock(ctx);
	return err;
}

#if CONFIG_SPI_ASYNC
static void transceive_async_callback(struct rtio *r,
				      const struct rtio_sqe *sqe,
				      int res,
				      void *arg0)
{
	struct spi_rtio *ctx = arg0;
	spi_callback_t cb = ctx->async_cb;
	struct spi_dt_spec *dt_spec = &ctx->dt_spec;
	const struct device *dev = dt_spec->bus;
	void *userdata = sqe->userdata;

	/*
	 * We have stored the context data needed for the callback so
	 * we can unlock the context here
	 */
	spi_r_unlock(ctx);

	if (cb == NULL) {
		return;
	}

	cb(dev, res, userdata);
}

int spi_rtio_transceive_async(struct spi_rtio *ctx,
			      const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs,
			      spi_callback_t cb,
			      void *userdata)
{
	struct spi_dt_spec *dt_spec = &ctx->dt_spec;
	struct rtio_sqe *sqe;
	int ret = 0;

	if (tx_bufs == NULL && rx_bufs == NULL) {
		return -EINVAL;
	}

	spi_r_lock(ctx);

	dt_spec->config = *config;

	ret = spi_rtio_copy(ctx->r, &ctx->iodev, tx_bufs, rx_bufs, true, &sqe);
	if (ret < 0) {
		spi_r_unlock(ctx);
		return ret;
	}

	sqe->flags |= RTIO_SQE_CHAINED;

	sqe = rtio_sqe_acquire(ctx->r);
	if (sqe == NULL) {
		rtio_sqe_drop_all(ctx->r);
		spi_r_unlock(ctx);
		return ret;
	}

	ctx->async_cb = cb;

	rtio_sqe_prep_callback_no_cqe(sqe,
				      transceive_async_callback,
				      ctx,
				      userdata);

	rtio_submit(ctx->r, 0);

	return 0;
}
#endif /* CONFIG_SPI_ASYNC */
