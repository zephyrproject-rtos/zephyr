/*
 * Copyright (c) 2024 Google LLC
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/rtio.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/work.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(i3c_rtio, CONFIG_I3C_LOG_LEVEL);

static inline void i3c_msg_from_rx(const struct rtio_iodev_sqe *iodev_sqe, struct i3c_msg *msg)
{
	__ASSERT_NO_MSG(iodev_sqe->sqe.op == RTIO_OP_RX);

	msg->buf = iodev_sqe->sqe.rx.buf;
	msg->len = iodev_sqe->sqe.rx.buf_len;
	msg->flags =
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I3C_STOP) ? I3C_MSG_STOP : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I3C_RESTART) ? I3C_MSG_RESTART : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I3C_HDR) ? I3C_MSG_HDR : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I3C_NBCH) ? I3C_MSG_NBCH : 0) |
		I3C_MSG_READ;
}

static inline void i3c_msg_from_tx(const struct rtio_iodev_sqe *iodev_sqe, struct i3c_msg *msg)
{
	__ASSERT_NO_MSG(iodev_sqe->sqe.op == RTIO_OP_TX);

	msg->buf = (uint8_t *)iodev_sqe->sqe.tx.buf;
	msg->len = iodev_sqe->sqe.tx.buf_len;
	msg->flags =
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I3C_STOP) ? I3C_MSG_STOP : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I3C_RESTART) ? I3C_MSG_RESTART : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I3C_HDR) ? I3C_MSG_HDR : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I3C_NBCH) ? I3C_MSG_NBCH : 0) |
		I3C_MSG_WRITE;
}

static inline void i3c_msg_from_tiny_tx(const struct rtio_iodev_sqe *iodev_sqe, struct i3c_msg *msg)
{
	__ASSERT_NO_MSG(iodev_sqe->sqe.op == RTIO_OP_TINY_TX);

	msg->buf = (uint8_t *)iodev_sqe->sqe.tiny_tx.buf;
	msg->len = iodev_sqe->sqe.tiny_tx.buf_len;
	msg->flags =
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I3C_STOP) ? I3C_MSG_STOP : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I3C_RESTART) ? I3C_MSG_RESTART : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I3C_HDR) ? I3C_MSG_HDR : 0) |
		((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I3C_NBCH) ? I3C_MSG_NBCH : 0) |
		I3C_MSG_WRITE;
}

void i3c_iodev_submit_work_handler(struct rtio_iodev_sqe *txn_first)
{
	const struct i3c_iodev_data *data =
		(const struct i3c_iodev_data *)txn_first->sqe.iodev->data;
	struct i3c_device_desc *desc;

	LOG_DBG("Sync RTIO work item for: %p", (void *)txn_first);
	uint32_t num_msgs = 0;
	int rc = 0;
	struct rtio_iodev_sqe *txn_last = txn_first;

	/* TODO: there really needs to be a compile time way to get the i3c_device_desc */
	desc = i3c_device_find(data->bus, &data->dev_id);
	if (!desc) {
		LOG_ERR("Cannot find I3C device descriptor");
		rc = -ENODEV;
		rtio_iodev_sqe_err(txn_first, rc);
		return;
	}

	/* Allocate the i3c_msg's on the stack, to do so
	 * the count of messages needs to be determined.
	 */
	do {
		switch (txn_last->sqe.op) {
		case RTIO_OP_RX:
		case RTIO_OP_TX:
		case RTIO_OP_TINY_TX:
			num_msgs++;
			break;
		default:
			LOG_ERR("Invalid op code %d for submission %p", txn_last->sqe.op,
				(void *)&txn_last->sqe);
			rc = -EIO;
			break;
		}
		txn_last = rtio_txn_next(txn_last);
	} while (rc == 0 && txn_last != NULL);

	if (rc != 0) {
		rtio_iodev_sqe_err(txn_first, rc);
		return;
	}

	/* Allocate msgs on the stack, MISRA doesn't like VLAs so we need a statically
	 * sized array here. It's pretty unlikely we have more than 4 i3c messages
	 * in a transaction as we typically would only have 2, one to write a
	 * register address, and another to read/write the register into an array
	 */
	if (num_msgs > CONFIG_I3C_RTIO_FALLBACK_MSGS) {
		LOG_ERR("At most CONFIG_I3C_RTIO_FALLBACK_MSGS"
			" submissions in a transaction are"
			" allowed in the default handler");
		rtio_iodev_sqe_err(txn_first, -ENOMEM);
		return;
	}
	struct i3c_msg msgs[CONFIG_I3C_RTIO_FALLBACK_MSGS];

	rc = 0;
	txn_last = txn_first;

	/* Copy the transaction into the stack allocated msgs */
	for (int i = 0; i < num_msgs; i++) {
		switch (txn_last->sqe.op) {
		case RTIO_OP_RX:
			i3c_msg_from_rx(txn_last, &msgs[i]);
			break;
		case RTIO_OP_TX:
			i3c_msg_from_tx(txn_last, &msgs[i]);
			break;
		case RTIO_OP_TINY_TX:
			i3c_msg_from_tiny_tx(txn_last, &msgs[i]);
			break;
		default:
			rc = -EIO;
			break;
		}

		txn_last = rtio_txn_next(txn_last);
	}

	if (rc == 0) {
		__ASSERT_NO_MSG(num_msgs > 0);

		rc = i3c_transfer(desc, msgs, num_msgs);
	}

	if (rc != 0) {
		rtio_iodev_sqe_err(txn_first, rc);
	} else {
		rtio_iodev_sqe_ok(txn_first, 0);
	}
}

void i3c_iodev_submit_fallback(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	LOG_DBG("Executing fallback for dev: %p, sqe: %p", (void *)dev, (void *)iodev_sqe);

	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, i3c_iodev_submit_work_handler);
}
