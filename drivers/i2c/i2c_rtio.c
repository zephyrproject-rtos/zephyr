/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/mpsc_lockfree.h>
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

struct rtio_sqe *i2c_rtio_copy_reg_write_byte(struct rtio *r, struct rtio_iodev *iodev,
					      uint8_t reg_addr, uint8_t data)
{
	uint8_t msg[2];

	struct rtio_sqe *sqe = rtio_sqe_acquire(r);

	if (sqe == NULL) {
		rtio_sqe_drop_all(r);
		return NULL;
	}
	msg[0] = reg_addr;
	msg[1] = data;
	rtio_sqe_prep_tiny_write(sqe, iodev, RTIO_PRIO_NORM, msg, sizeof(msg), NULL);
	sqe->iodev_flags = RTIO_IODEV_I2C_STOP;
	return sqe;
}

struct rtio_sqe *i2c_rtio_copy_reg_burst_read(struct rtio *r, struct rtio_iodev *iodev,
					      uint8_t start_addr, void *buf, size_t num_bytes)
{
	struct rtio_sqe *sqe = rtio_sqe_acquire(r);

	if (sqe == NULL) {
		rtio_sqe_drop_all(r);
		return NULL;
	}
	rtio_sqe_prep_tiny_write(sqe, iodev, RTIO_PRIO_NORM, &start_addr, 1, NULL);
	sqe->flags |= RTIO_SQE_TRANSACTION;

	sqe = rtio_sqe_acquire(r);
	if (sqe == NULL) {
		rtio_sqe_drop_all(r);
		return NULL;
	}
	rtio_sqe_prep_read(sqe, iodev, RTIO_PRIO_NORM, buf, num_bytes, NULL);
	sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	return sqe;
}

void i2c_rtio_init(struct i2c_rtio *ctx, const struct device *dev)
{
	k_sem_init(&ctx->lock, 1, 1);
	mpsc_init(&ctx->io_q);
	ctx->txn_curr = NULL;
	ctx->txn_head = NULL;
	ctx->dt_spec.bus = dev;
	ctx->iodev.data = &ctx->dt_spec;
	ctx->iodev.api = &i2c_iodev_api;
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
	mpsc_push(&ctx->io_q, &iodev_sqe->q);
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
	sqe->flags = 0;
	sqe->iodev = iodev;
	sqe->i2c_config = i2c_config;

	rtio_submit(r, 1);

	cqe = rtio_cqe_consume(r);
	if (unlikely(cqe != NULL)) {
		res = cqe->result;
		rtio_cqe_release(r, cqe);
	} else {
		res = -EIO;
	}

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
	sqe->flags = 0;
	sqe->iodev = iodev;

	rtio_submit(r, 1);

	cqe = rtio_cqe_consume(r);
	if (unlikely(cqe != NULL)) {
		res = cqe->result;
		rtio_cqe_release(r, cqe);
	} else {
		res = -EIO;
	}

out:
	k_sem_give(&ctx->lock);
	return res;
}

#ifdef CONFIG_I2C_CALLBACK
/*
 * Allocate a callback closure slot. Lock-free via atomic_test_and_set_bit;
 * safe from any context. Returns NULL when the pool is exhausted.
 */
static struct i2c_rtio_cb_slot *i2c_rtio_cb_slot_alloc(struct i2c_rtio *ctx)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(ctx->cb_slots); i++) {
		if (!atomic_test_and_set_bit(ctx->cb_slot_used, i)) {
			struct i2c_rtio_cb_slot *slot = &ctx->cb_slots[i];

			slot->ctx = ctx;
			return slot;
		}
	}

	return NULL;
}

static void i2c_rtio_cb_slot_free(struct i2c_rtio_cb_slot *slot)
{
	struct i2c_rtio *ctx = slot->ctx;
	uint32_t i = slot - &ctx->cb_slots[0];

	atomic_clear_bit(ctx->cb_slot_used, i);
}

/*
 * Trampoline invoked by RTIO_OP_CALLBACK after the chained transaction
 * completes. last_result carries the transfer status. Releases ctx->lock
 * (which the helper held to keep ctx->dt_spec.addr stable for the driver)
 * before invoking the user callback.
 */
static void i2c_rtio_cb_trampoline(struct rtio *r, const struct rtio_sqe *sqe, int last_result,
				   void *arg0)
{
	struct i2c_rtio_cb_slot *slot = arg0;
	struct i2c_rtio *ctx = slot->ctx;
	i2c_callback_t cb = slot->cb;
	void *userdata = slot->userdata;
	const struct device *dev = slot->dev;

	ARG_UNUSED(r);
	ARG_UNUSED(sqe);

	i2c_rtio_cb_slot_free(slot);
	k_sem_give(&ctx->lock);

	cb(dev, last_result, userdata);
}

int i2c_rtio_transfer_cb(struct i2c_rtio *ctx, const struct device *dev, struct i2c_msg *msgs,
			 uint8_t num_msgs, uint16_t addr, i2c_callback_t cb, void *userdata)
{
	struct rtio_iodev *iodev = &ctx->iodev;
	struct rtio *const r = ctx->r;
	struct rtio_sqe *last_msg_sqe;
	struct rtio_sqe *cb_sqe;
	struct i2c_rtio_cb_slot *slot;

	__ASSERT_NO_MSG(num_msgs > 0);
	__ASSERT_NO_MSG(cb != NULL);

	slot = i2c_rtio_cb_slot_alloc(ctx);
	if (slot == NULL) {
		return -ENOMEM;
	}

	k_sem_take(&ctx->lock, K_FOREVER);

	ctx->dt_spec.addr = addr;

	last_msg_sqe = i2c_rtio_copy(r, iodev, msgs, num_msgs);
	if (last_msg_sqe == NULL) {
		k_sem_give(&ctx->lock);
		i2c_rtio_cb_slot_free(slot);
		return -ENOMEM;
	}

	cb_sqe = rtio_sqe_acquire(r);
	if (cb_sqe == NULL) {
		rtio_sqe_drop_all(r);
		k_sem_give(&ctx->lock);
		i2c_rtio_cb_slot_free(slot);
		return -ENOMEM;
	}

	slot->cb = cb;
	slot->userdata = userdata;
	slot->dev = dev;

	rtio_sqe_prep_callback(cb_sqe, i2c_rtio_cb_trampoline, slot, NULL);
	cb_sqe->flags |= RTIO_SQE_NO_RESPONSE;

	last_msg_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_submit(r, 0);

	return 0;
}
#endif /* CONFIG_I2C_CALLBACK */
