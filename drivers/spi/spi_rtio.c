/*
 * Copyright (c) 2023 Intel Corporation
 * Copyright (c) 2024 Croxel Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/rtio/work.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(spi_rtio, CONFIG_SPI_LOG_LEVEL);

const struct rtio_iodev_api spi_iodev_api = {
	.submit = spi_iodev_submit,
};

static void spi_rtio_iodev_default_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	struct spi_dt_spec *dt_spec = iodev_sqe->sqe.iodev->data;
	const struct device *dev = dt_spec->bus;
	int err = 0;

	LOG_DBG("Sync RTIO work item for: %p", (void *)dev);

	/** Take care of Multi-submissions transactions in the same context.
	 * This guarantees that linked items will be consumed in the expected
	 * order, regardless pending items in the workqueue.
	 */
	struct rtio_iodev_sqe *txn_head = iodev_sqe;
	struct rtio_iodev_sqe *txn_curr = iodev_sqe;

	do {
		struct rtio_sqe *sqe = &txn_curr->sqe;
		struct spi_buf tx_buf = {0};
		struct spi_buf_set tx_buf_set = {
			.buffers = &tx_buf,
		};

		struct spi_buf rx_buf = {0};
		struct spi_buf_set rx_buf_set = {
			.buffers = &rx_buf,
		};

		LOG_DBG("Preparing transfer: %p", txn_curr);

		switch (sqe->op) {
		case RTIO_OP_RX:
			rx_buf.buf = sqe->rx.buf;
			rx_buf.len = sqe->rx.buf_len;
			rx_buf_set.count = 1;
			break;
		case RTIO_OP_TX:
			tx_buf.buf = (uint8_t *)sqe->tx.buf;
			tx_buf.len = sqe->tx.buf_len;
			tx_buf_set.count = 1;
			break;
		case RTIO_OP_TINY_TX:
			tx_buf.buf = (uint8_t *)sqe->tiny_tx.buf;
			tx_buf.len = sqe->tiny_tx.buf_len;
			tx_buf_set.count = 1;
			break;
		case RTIO_OP_TXRX:
			rx_buf.buf = sqe->txrx.rx_buf;
			rx_buf.len = sqe->txrx.buf_len;
			tx_buf.buf = (uint8_t *)sqe->txrx.tx_buf;
			tx_buf.len = sqe->txrx.buf_len;
			rx_buf_set.count = 1;
			tx_buf_set.count = 1;
			break;
		default:
			LOG_ERR("Invalid op code %d for submission %p\n", sqe->op, (void *)sqe);
			err = -EIO;
			break;
		}

		if (!err) {
			struct spi_buf_set *tx_buf_ptr = tx_buf_set.count > 0 ? &tx_buf_set : NULL;
			struct spi_buf_set *rx_buf_ptr = rx_buf_set.count > 0 ? &rx_buf_set : NULL;

			err = spi_transceive_dt(dt_spec, tx_buf_ptr, rx_buf_ptr);

			/* NULL if this submission is not a transaction */
			txn_curr = rtio_txn_next(txn_curr);
		}
	} while (err >= 0 && txn_curr != NULL);

	if (err < 0) {
		LOG_ERR("Transfer failed: %d", err);
		rtio_iodev_sqe_err(txn_head, err);
	} else {
		LOG_DBG("Transfer OK: %d", err);
		rtio_iodev_sqe_ok(txn_head, err);
	}
}

void spi_rtio_iodev_default_submit(const struct device *dev,
				   struct rtio_iodev_sqe *iodev_sqe)
{
	LOG_DBG("Executing fallback for dev: %p, sqe: %p", (void *)dev, (void *)iodev_sqe);

	struct rtio_work_req *req = rtio_work_req_alloc();

	__ASSERT_NO_MSG(req);

	rtio_work_req_submit(req, iodev_sqe, spi_rtio_iodev_default_submit_sync);
}
