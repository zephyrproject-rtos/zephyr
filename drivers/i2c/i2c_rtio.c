/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/rtio_mpsc.h>
#include <zephyr/sys/__assert.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_rtio);

const struct rtio_iodev_api i2c_iodev_api = {
	.submit = i2c_iodev_submit,
};

struct rtio_sqe *i2c_rtio_copy(struct rtio *r, struct rtio_iodev *iodev, const struct i2c_msg *msgs,
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

		if (msgs[i].flags & I2C_MSG_READ) {
			rtio_sqe_prep_read(sqe, iodev, RTIO_PRIO_NORM, msgs[i].buf, msgs[i].len,
					   NULL);
		} else {
			rtio_sqe_prep_write(sqe, iodev, RTIO_PRIO_NORM, msgs[i].buf, msgs[i].len,
					    NULL);
		}
		sqe->flags |= RTIO_SQE_TRANSACTION;
		sqe->iodev_flags =
			((msgs[i].flags & I2C_MSG_STOP) ? RTIO_IODEV_I2C_STOP : 0) |
			((msgs[i].flags & I2C_MSG_RESTART) ? RTIO_IODEV_I2C_RESTART : 0) |
			((msgs[i].flags & I2C_MSG_ADDR_10_BITS) ? RTIO_IODEV_I2C_10_BITS : 0);
	}

	sqe->flags &= ~RTIO_SQE_TRANSACTION;

	return sqe;
}

void i2c_rtio_init(struct i2c_rtio *ctx, const struct device *dev)
{
	k_sem_init(&ctx->lock, 1, 1);
	rtio_mpsc_init(&ctx->io_q);
	ctx->txn_curr = NULL;
	ctx->txn_head = NULL;
	ctx->dt_spec.bus = dev;
	ctx->iodev.data = &ctx->dt_spec;
	ctx->iodev.api = &i2c_iodev_api;
	/* TODO drop the builtin submission queue? */
	rtio_mpsc_init(&ctx->iodev.iodev_sq);
}

/**
 * @private
 * @brief Setup the next transaction (could be a single op) if needed
 *
 * @retval true New transaction to start with the hardware is setup
 * @retval false No new transaction to start
 */
static bool i2c_rtio_next(struct i2c_rtio *ctx, bool completion)
{
	k_spinlock_key_t key = k_spin_lock(&ctx->slock);

	/* Already working on something, bail early */
	if (!completion && ctx->txn_head != NULL) {
		k_spin_unlock(&ctx->slock, key);
		return false;
	}

	struct rtio_mpsc_node *next = rtio_mpsc_pop(&ctx->io_q);

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

bool i2c_rtio_complete(struct i2c_rtio *ctx, int status)
{
	/* On error bail */
	if (status < 0) {
		rtio_iodev_sqe_err(ctx->txn_head, status);
		return i2c_rtio_next(ctx, true);
	}

	/* Try for next submission in the transaction */
	ctx->txn_curr = rtio_txn_next(ctx->txn_curr);
	if (ctx->txn_curr) {
		return true;
	}

	rtio_iodev_sqe_ok(ctx->txn_head, status);
	return i2c_rtio_next(ctx, true);
}
bool i2c_rtio_submit(struct i2c_rtio *ctx, struct rtio_iodev_sqe *iodev_sqe)
{
	rtio_mpsc_push(&ctx->io_q, &iodev_sqe->q);
	return i2c_rtio_next(ctx, false);
}

int i2c_rtio_transfer(struct i2c_rtio *ctx, struct i2c_msg *msgs, uint8_t num_msgs, uint16_t addr)
{
	struct rtio_iodev *iodev = &ctx->iodev;
	struct rtio *const r = ctx->r;
	struct rtio_sqe *sqe = NULL;
	struct rtio_cqe *cqe = NULL;
	int res = 0;

	k_sem_take(&ctx->lock, K_FOREVER);

	ctx->dt_spec.addr = addr;

	sqe = i2c_rtio_copy(r, iodev, msgs, num_msgs);
	if (sqe == NULL) {
		LOG_ERR("Not enough submission queue entries");
		res = -ENOMEM;
		goto out;
	}

	sqe->flags &= ~RTIO_SQE_TRANSACTION;

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

int i2c_rtio_configure(struct i2c_rtio *ctx, uint32_t i2c_config)
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

	sqe->op = RTIO_OP_I2C_CONFIGURE;
	sqe->iodev = iodev;
	sqe->i2c_config = i2c_config;

	rtio_submit(r, 1);

	cqe = rtio_cqe_consume(r);
	res = cqe->result;
	rtio_cqe_release(r, cqe);

out:
	k_sem_give(&ctx->lock);
	return res;
}

int i2c_rtio_recover(struct i2c_rtio *ctx)
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

	sqe->op = RTIO_OP_I2C_RECOVER;
	sqe->iodev = iodev;

	rtio_submit(r, 1);

	cqe = rtio_cqe_consume(r);
	res = cqe->result;
	rtio_cqe_release(r, cqe);

out:
	k_sem_give(&ctx->lock);
	return res;
}
