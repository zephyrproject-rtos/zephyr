/*
 * Copyright (c) 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/work.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(i2c_rtio, CONFIG_I2C_LOG_LEVEL);

static int i2c_iodev_submit_rx(struct rtio_iodev_sqe *iodev_sqe, struct i2c_msg msgs[2],
			       uint8_t *num_msgs)
{
	__ASSERT_NO_MSG(iodev_sqe->sqe.op == RTIO_OP_RX);

	msgs[0].buf = iodev_sqe->sqe.rx.buf;
	msgs[0].len = iodev_sqe->sqe.rx.buf_len;
	msgs[0].flags =
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_STOP) ? I2C_MSG_STOP : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_RESTART) ? I2C_MSG_RESTART : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_10_BITS) ? I2C_MSG_ADDR_10_BITS : 0) |
		I2C_MSG_READ;
	*num_msgs = 1;
	return 0;
}

static int i2c_iodev_submit_tx(struct rtio_iodev_sqe *iodev_sqe, struct i2c_msg msgs[2],
			       uint8_t *num_msgs)
{
	__ASSERT_NO_MSG(iodev_sqe->sqe.op == RTIO_OP_TX);

	msgs[0].buf = (uint8_t *)iodev_sqe->sqe.tx.buf;
	msgs[0].len = iodev_sqe->sqe.tx.buf_len;
	msgs[0].flags =
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_STOP) ? I2C_MSG_STOP : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_RESTART) ? I2C_MSG_RESTART : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_10_BITS) ? I2C_MSG_ADDR_10_BITS : 0) |
		I2C_MSG_WRITE;
	*num_msgs = 1;
	return 0;
}

static int i2c_iodev_submit_tiny_tx(struct rtio_iodev_sqe *iodev_sqe, struct i2c_msg msgs[2],
				    uint8_t *num_msgs)
{
	__ASSERT_NO_MSG(iodev_sqe->sqe.op == RTIO_OP_TINY_TX);

	msgs[0].buf = (uint8_t *)iodev_sqe->sqe.tiny_tx.buf;
	msgs[0].len = iodev_sqe->sqe.tiny_tx.buf_len;
	msgs[0].flags =
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_STOP) ? I2C_MSG_STOP : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_RESTART) ? I2C_MSG_RESTART : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_10_BITS) ? I2C_MSG_ADDR_10_BITS : 0) |
		I2C_MSG_WRITE;
	*num_msgs = 1;
	return 0;
}

static int i2c_iodev_submit_txrx(struct rtio_iodev_sqe *iodev_sqe, struct i2c_msg msgs[2],
				 uint8_t *num_msgs)
{
	__ASSERT_NO_MSG(iodev_sqe->sqe.op == RTIO_OP_TXRX);

	msgs[0].buf = (uint8_t *)iodev_sqe->sqe.txrx.tx_buf;
	msgs[0].len = iodev_sqe->sqe.txrx.buf_len;
	msgs[0].flags =
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_10_BITS) ? I2C_MSG_ADDR_10_BITS : 0) |
		I2C_MSG_WRITE;
	msgs[1].buf = iodev_sqe->sqe.txrx.rx_buf;
	msgs[1].len = iodev_sqe->sqe.txrx.buf_len;
	msgs[1].flags =
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_STOP) ? I2C_MSG_STOP : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_RESTART) ? I2C_MSG_RESTART : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_10_BITS) ? I2C_MSG_ADDR_10_BITS : 0) |
		I2C_MSG_READ;
	*num_msgs = 2;
	return 0;
}

void i2c_iodev_submit_work_handler(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct i2c_dt_spec *dt_spec = (const struct i2c_dt_spec *)iodev_sqe->sqe.iodev->data;
	const struct device *dev = dt_spec->bus;

	LOG_DBG("Sync RTIO work item for: %p", (void *)iodev_sqe);

	struct rtio_iodev_sqe *transaction_current = iodev_sqe;
	struct i2c_msg msgs[2];
	uint8_t num_msgs;
	int rc = 0;

	do {
		/* Convert the iodev_sqe back to an i2c_msg */
		switch (transaction_current->sqe.op) {
		case RTIO_OP_RX:
			rc = i2c_iodev_submit_rx(transaction_current, msgs, &num_msgs);
			break;
		case RTIO_OP_TX:
			rc = i2c_iodev_submit_tx(transaction_current, msgs, &num_msgs);
			break;
		case RTIO_OP_TINY_TX:
			rc = i2c_iodev_submit_tiny_tx(transaction_current, msgs, &num_msgs);
			break;
		case RTIO_OP_TXRX:
			rc = i2c_iodev_submit_txrx(transaction_current, msgs, &num_msgs);
			break;
		default:
			LOG_ERR("Invalid op code %d for submission %p", transaction_current->sqe.op,
				(void *)&transaction_current->sqe);
			rc = -EIO;
			break;
		}

		if (rc == 0) {
			__ASSERT_NO_MSG(num_msgs > 0);

			rc = i2c_transfer(dev, msgs, num_msgs, dt_spec->addr);
			transaction_current = rtio_txn_next(transaction_current);
		}
	} while (rc == 0 && transaction_current != NULL);

	if (rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

void i2c_iodev_submit_fallback(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	LOG_DBG("Executing fallback for dev: %p, sqe: %p", (void *)dev, (void *)iodev_sqe);

	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, i2c_iodev_submit_work_handler);
}
