/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/spi.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(spi_mngr, CONFIG_SPI_LOG_LEVEL);
#include "spi_context.h"

static void spi_callback(struct device *dev, int result, void *user_data);

static struct spi_transaction *get_current(struct spi_mngr *spi_mngr)
{
	sys_snode_t *node = sys_slist_peek_head(&spi_mngr->mngr.ops);
	struct qop_op *op = CONTAINER_OF(node, struct qop_op, node);

	return CONTAINER_OF(op, struct spi_transaction, op);
}

static bool post_single_transfer(struct device *dev, const struct spi_msg *msg)
{
	struct spi_common_data *common_data = dev->driver_data;

	if ((msg->flags & SPI_MSG_CS0_END_MASK) == SPI_MSG_CS0_END_CLR) {
		spi_context_cs_n_control(&common_data->ctx, 0, false);
	}

	return (msg->flags & SPI_MSG_PAUSE_AFTER);
}

static void pre_single_transfer(struct device *dev, const struct spi_msg *msg)
{
	struct spi_common_data *common_data = dev->driver_data;

	if ((msg->flags & SPI_MSG_CS0_START_MASK) == SPI_MSG_CS0_START_SET) {
		spi_context_cs_n_control(&common_data->ctx, 0, true);
	}
}

static int do_next_transfer(struct spi_mngr *spi_mngr)
{
	const struct spi_transaction *transaction = get_current(spi_mngr);
	const struct spi_msg *msg = &transaction->msgs[spi_mngr->current_idx];
	int err;

	pre_single_transfer(spi_mngr->dev, msg);

	err = spi_single_transfer(spi_mngr->dev, msg, spi_callback, NULL);
	if (err < 0) {
		(void)post_single_transfer(spi_mngr->dev, msg);
	}

	return err;
}

static int do_next_delayed_transfer(struct spi_mngr *spi_mngr)
{
	const struct spi_transaction *transaction = get_current(spi_mngr);
	const struct spi_msg *msg = &transaction->msgs[spi_mngr->current_idx];
	u8_t delay = (msg->flags & SPI_MSG_DELAY_MASK) >> SPI_MSG_DELAY_OFFSET;

	if (delay) {
		k_timer_start(&spi_mngr->timer, K_MSEC(delay), 0);
		return 0;
	}

	return do_next_transfer(spi_mngr);
}

/* returns true if there was next transaction pending */
static void on_op_completed(struct spi_mngr *spi_mngr, int result)
{
	LOG_DBG("Transaction completed");
	qop_op_done_notify(&spi_mngr->mngr, result);
}

static void timer_expired(struct k_timer *timer)
{
	struct spi_mngr *spi_mngr = k_timer_user_data_get(timer);
	int err;

	err = do_next_transfer(spi_mngr);
	if (err < 0) {
		on_op_completed(spi_mngr, err);
	}
}

static void spi_callback(struct device *dev, int result, void *user_data)
{
	int err;
	bool pause;
	struct spi_common_data *data = dev->driver_data;
	struct spi_mngr *spi_mngr = &data->mngr;
	struct spi_transaction *transaction = get_current(spi_mngr);
	const struct spi_msg *msg = &transaction->msgs[spi_mngr->current_idx];

	LOG_DBG("Transfer %d/%d completed (result:%d)",
		spi_mngr->current_idx + 1, transaction->num_msgs, result);

	pause = post_single_transfer(dev, msg);

	/* error handle */
	if (result != 0) {
		on_op_completed(spi_mngr, result);
		return;
	}

	spi_mngr->current_idx++;
	if (spi_mngr->current_idx == transaction->num_msgs) {
		on_op_completed(spi_mngr, result);
		return;
	}

	if (pause) {
		transaction->paused = true;
		transaction->paused_callback(dev, transaction,
						spi_mngr->current_idx);
	} else {
		err = do_next_delayed_transfer(spi_mngr);
		if (err < 0) {
			on_op_completed(spi_mngr, err);
		}
	}
}

static int transaction_schedule(struct qop_mngr *mngr)
{
	struct spi_mngr *spi_mngr = CONTAINER_OF(mngr, struct spi_mngr, mngr);
	const struct spi_transaction *transaction = get_current(spi_mngr);
	int err;

	spi_mngr->current_idx = 0;
	err = spi_configure(spi_mngr->dev, transaction->config);
	if (err < 0) {
		return err;
	}

	return do_next_delayed_transfer(spi_mngr);
}

int z_spi_mngr_init(struct device *dev)
{
	struct spi_common_data *data = dev->driver_data;
	struct spi_mngr *spi_mngr = &data->mngr;

	spi_mngr->dev = dev;

	k_timer_init(&spi_mngr->timer, timer_expired, NULL);
	k_timer_user_data_set(&spi_mngr->timer, spi_mngr);

	return qop_mngr_init(&spi_mngr->mngr, transaction_schedule, 0);
}

int spi_resume(struct device *dev)
{
	struct spi_common_data *data = dev->driver_data;
	struct spi_mngr *spi_mngr = &data->mngr;
	struct spi_transaction *transaction = get_current(spi_mngr);

	if (transaction && transaction->paused) {
		int err;

		transaction->paused = false;
		err = do_next_delayed_transfer(spi_mngr);
		if (err < 0) {
			on_op_completed(spi_mngr, err);
		}
		return 0;
	}

	return -EFAULT;
}

int spi_schedule(struct device *dev, const struct spi_transaction *transaction)
{
	struct spi_common_data *data = dev->driver_data;
	struct spi_mngr *spi_mngr = &data->mngr;

	LOG_DBG("Scheduling transaction (config:%p, msgs:%d)",
			transaction->config, transaction->num_msgs);

	return qop_op_schedule(&spi_mngr->mngr,
				(struct qop_op *)&transaction->op);
}
