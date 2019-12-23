/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/i2c.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(i2c_mngr, CONFIG_I2C_LOG_LEVEL);

static void i2c_callback(struct device *dev, int result, void *user_data);

static int do_next_transfer(struct i2c_mngr *i2c_mngr)
{
	sys_snode_t *node = sys_slist_peek_head(&i2c_mngr->mngr.ops);
	struct qop_op *op = CONTAINER_OF(node, struct qop_op, node);
	struct i2c_transaction *transaction=
			CONTAINER_OF(op, struct i2c_transaction, op);

	return i2c_single_transfer(i2c_mngr->dev,
				&transaction->msgs[i2c_mngr->current_idx],
				transaction->addr,
				i2c_callback,
				NULL);
}

static int do_next_delayed_transfer(struct i2c_mngr *i2c_mngr)
{
	sys_snode_t *node = sys_slist_peek_head(&i2c_mngr->mngr.ops);
	struct qop_op *op = CONTAINER_OF(node, struct qop_op, node);
	struct i2c_transaction *transaction=
			CONTAINER_OF(op, struct i2c_transaction, op);
	u8_t delay =
	   (transaction->msgs[i2c_mngr->current_idx].flags & I2C_MSG_DELAY_MASK)
	   >> I2C_MSG_DELAY_OFFSET;

	if (delay) {
		k_timer_start(&i2c_mngr->timer, K_MSEC(delay), 0);
		return 0;
	}

	return do_next_transfer(i2c_mngr);
}

/* returns true if there was next transaction pending */
static void on_op_completed(struct i2c_mngr *i2c_mngr, int result)
{
	I2C_DBG(i2c_mngr->dev, "Transaction completed");
	qop_op_done_notify(&i2c_mngr->mngr, result);
}

static void timer_expired(struct k_timer *timer)
{
	struct i2c_mngr *i2c_mngr = k_timer_user_data_get(timer);
	int err;

	err = do_next_transfer(i2c_mngr);
	if (err < 0) {
		on_op_completed(i2c_mngr, err);
	}
}

static void i2c_callback(struct device *dev, int result, void *user_data)
{
	int err;
	struct i2c_common_data *data = dev->driver_data;
	struct i2c_mngr *i2c_mngr = &data->mngr;
	sys_snode_t *node = sys_slist_peek_head(&i2c_mngr->mngr.ops);
	struct qop_op *op = CONTAINER_OF(node, struct qop_op, node);
	struct i2c_transaction *transaction=
			CONTAINER_OF(op, struct i2c_transaction, op);

	I2C_DBG(dev, "Transfer %d/%d completed (result:%d)",
		i2c_mngr->current_idx + 1, transaction->num_msgs, result);
	/* error handle */
	if (result != 0) {
		on_op_completed(i2c_mngr, result);
		return;
	}

	i2c_mngr->current_idx++;
	if (i2c_mngr->current_idx == transaction->num_msgs) {
		on_op_completed(i2c_mngr, result);
		return;
	}

	err = do_next_delayed_transfer(i2c_mngr);
	if (err < 0) {
		on_op_completed(i2c_mngr, err);
	}
}

static int transaction_schedule(struct qop_mngr *mngr)
{
	struct i2c_mngr *i2c_mngr = CONTAINER_OF(mngr, struct i2c_mngr, mngr);

	i2c_mngr->current_idx = 0;

	return do_next_delayed_transfer(i2c_mngr);
}

int z_i2c_mngr_init(struct device *dev)
{
	struct i2c_common_data *data = dev->driver_data;
	struct i2c_mngr *i2c_mngr = &data->mngr;

	i2c_mngr->dev = dev;

	k_timer_init(&i2c_mngr->timer, timer_expired, NULL);
	k_timer_user_data_set(&i2c_mngr->timer, i2c_mngr);

	return qop_mngr_init(&i2c_mngr->mngr, transaction_schedule, 0);
}

int i2c_schedule(struct device *dev, struct i2c_transaction *transaction)
{
	struct i2c_common_data *data = dev->driver_data;
	struct i2c_mngr *i2c_mngr = &data->mngr;

	I2C_DBG(dev, "Scheduling transaction (addr:%d, msgs:%d)",
			transaction->addr, transaction->num_msgs);

	return qop_op_schedule(&i2c_mngr->mngr, &transaction->op);
}
