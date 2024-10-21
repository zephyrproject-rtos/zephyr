/*
 * Copyright (c) 2023 Intel Corporation
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/rtio.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/mpsc_lockfree.h>
#include <zephyr/sys/__assert.h>

#define LOG_LEVEL CONFIG_I3C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3c_rtio);

const struct rtio_iodev_api i3c_iodev_api = {
	.submit = i3c_iodev_submit,
};

struct rtio_sqe *i3c_rtio_copy(struct rtio *r, struct rtio_iodev *iodev, const struct i3c_msg *msgs,
			       uint8_t num_msgs)
{
	__ASSERT(num_msgs > 0, "Expecting at least one message to copy");

	struct rtio_sqe *sqe = NULL;

	for (uint8_t i = 0; i < num_msgs; i++) {
		sqe = rtio_sqe_acquire(r);

		if (sqe == NULL) {
			rtio_sqe_drop_all(r);
			return NULL;
		}

		if (msgs[i].flags & I3C_MSG_READ) {
			rtio_sqe_prep_read(sqe, iodev, RTIO_PRIO_NORM, msgs[i].buf, msgs[i].len,
					   NULL);
		} else {
			rtio_sqe_prep_write(sqe, iodev, RTIO_PRIO_NORM, msgs[i].buf, msgs[i].len,
					    NULL);
		}
		sqe->flags |= RTIO_SQE_TRANSACTION;
		sqe->iodev_flags =
			((msgs[i].flags & I3C_MSG_STOP) ? RTIO_IODEV_I3C_STOP : 0) |
			((msgs[i].flags & I3C_MSG_RESTART) ? RTIO_IODEV_I3C_RESTART : 0) |
			((msgs[i].flags & I3C_MSG_HDR) ? RTIO_IODEV_I3C_HDR : 0) |
			((msgs[i].flags & I3C_MSG_NBCH) ? RTIO_IODEV_I3C_NBCH : 0) |
			RTIO_IODEV_I3C_HDR_MODE_SET(msgs[i].hdr_mode) |
			RTIO_IODEV_I3C_HDR_CMD_CODE_SET(msgs[i].hdr_cmd_code);
	}

	sqe->flags &= ~RTIO_SQE_TRANSACTION;

	return sqe;
}

void i3c_rtio_init(struct i3c_rtio *ctx)
{
	k_sem_init(&ctx->lock, 1, 1);
	mpsc_init(&ctx->io_q);
	ctx->txn_curr = NULL;
	ctx->txn_head = NULL;
	ctx->iodev.api = &i3c_iodev_api;
}

/**
 * @private
 * @brief Setup the next transaction (could be a single op) if needed
 *
 * @retval true New transaction to start with the hardware is setup
 * @retval false No new transaction to start
 */
static bool i3c_rtio_next(struct i3c_rtio *ctx, bool completion)
{
	k_spinlock_key_t key = k_spin_lock(&ctx->slock);

	/* Already working on something, bail early */
	if (!completion && ctx->txn_head != NULL) {
		k_spin_unlock(&ctx->slock, key);
		return false;
	}

	struct mpsc_node *next = mpsc_pop(&ctx->io_q);

	/* Nothing left to do */
	if (next == NULL) {
		ctx->txn_head = NULL;
		ctx->txn_curr = NULL;
		k_spin_unlock(&ctx->slock, key);
		return false;
	}

	ctx->txn_head = CONTAINER_OF(next, struct rtio_iodev_sqe, q);
	ctx->txn_curr = ctx->txn_head;

	k_spin_unlock(&ctx->slock, key);

	return true;
}

bool i3c_rtio_complete(struct i3c_rtio *ctx, int status)
{
	/* On error bail */
	if (status < 0) {
		rtio_iodev_sqe_err(ctx->txn_head, status);
		return i3c_rtio_next(ctx, true);
	}

	/* Try for next submission in the transaction */
	ctx->txn_curr = rtio_txn_next(ctx->txn_curr);
	if (ctx->txn_curr) {
		return true;
	}

	rtio_iodev_sqe_ok(ctx->txn_head, status);
	return i3c_rtio_next(ctx, true);
}
bool i3c_rtio_submit(struct i3c_rtio *ctx, struct rtio_iodev_sqe *iodev_sqe)
{
	mpsc_push(&ctx->io_q, &iodev_sqe->q);
	return i3c_rtio_next(ctx, false);
}

int i3c_rtio_transfer(struct i3c_rtio *ctx, struct i3c_msg *msgs, uint8_t num_msgs,
		      struct i3c_device_desc *desc)
{
	struct rtio_iodev *iodev = &ctx->iodev;
	struct rtio *const r = ctx->r;
	struct rtio_sqe *sqe = NULL;
	struct rtio_cqe *cqe = NULL;
	int res = 0;

	k_sem_take(&ctx->lock, K_FOREVER);

	ctx->i3c_desc = desc;

	sqe = i3c_rtio_copy(r, iodev, msgs, num_msgs);
	if (sqe == NULL) {
		LOG_ERR("Not enough submission queue entries");
		res = -ENOMEM;
		goto out;
	}

	rtio_submit(r, 1);

	cqe = rtio_cqe_consume(r);
	while (cqe != NULL) {
		res = cqe->result;
		rtio_cqe_release(r, cqe);
		cqe = rtio_cqe_consume(r);
	}

out:
	k_sem_give(&ctx->lock);
	return res;
}

int i3c_rtio_configure(struct i3c_rtio *ctx, enum i3c_config_type type, void *config)
{
	struct rtio_iodev *iodev = &ctx->iodev;
	struct rtio *const r = ctx->r;
	struct rtio_sqe *sqe = NULL;
	struct rtio_cqe *cqe = NULL;
	int res = 0;

	k_sem_take(&ctx->lock, K_FOREVER);

	sqe = rtio_sqe_acquire(r);
	if (sqe == NULL) {
		LOG_ERR("Not enough submission queue entries");
		res = -ENOMEM;
		goto out;
	}

	sqe->op = RTIO_OP_I3C_CONFIGURE;
	sqe->iodev = iodev;
	sqe->i3c_config.type = type;
	sqe->i3c_config.config = config;

	rtio_submit(r, 1);

	cqe = rtio_cqe_consume(r);
	res = cqe->result;
	rtio_cqe_release(r, cqe);

out:
	k_sem_give(&ctx->lock);
	return res;
}

int i3c_rtio_ccc(struct i3c_rtio *ctx, struct i3c_ccc_payload *payload)
{
	struct rtio_iodev *iodev = &ctx->iodev;
	struct rtio *const r = ctx->r;
	struct rtio_sqe *sqe = NULL;
	struct rtio_cqe *cqe = NULL;
	int res = 0;

	k_sem_take(&ctx->lock, K_FOREVER);

	sqe = rtio_sqe_acquire(r);
	if (sqe == NULL) {
		LOG_ERR("Not enough submission queue entries");
		res = -ENOMEM;
		goto out;
	}

	sqe->op = RTIO_OP_I3C_CCC;
	sqe->iodev = iodev;
	sqe->ccc_payload = payload;

	rtio_submit(r, 1);

	cqe = rtio_cqe_consume(r);
	res = cqe->result;
	rtio_cqe_release(r, cqe);

out:
	k_sem_give(&ctx->lock);
	return res;
}

int i3c_rtio_recover(struct i3c_rtio *ctx)
{
	struct rtio_iodev *iodev = &ctx->iodev;
	struct rtio *const r = ctx->r;
	struct rtio_sqe *sqe = NULL;
	struct rtio_cqe *cqe = NULL;
	int res = 0;

	k_sem_take(&ctx->lock, K_FOREVER);

	sqe = rtio_sqe_acquire(r);
	if (sqe == NULL) {
		LOG_ERR("Not enough submission queue entries");
		res = -ENOMEM;
		goto out;
	}

	sqe->op = RTIO_OP_I3C_RECOVER;
	sqe->iodev = iodev;

	rtio_submit(r, 1);

	cqe = rtio_cqe_consume(r);
	res = cqe->result;
	rtio_cqe_release(r, cqe);

out:
	k_sem_give(&ctx->lock);
	return res;
}
