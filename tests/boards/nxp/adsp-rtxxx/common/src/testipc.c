/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>

#include "testipc.h"

static const struct mbox_dt_spec mbox_tx = MBOX_DT_SPEC_GET(DT_NODELABEL(mailboxes), tx);
static const struct mbox_dt_spec mbox_rx = MBOX_DT_SPEC_GET(DT_NODELABEL(mailboxes), rx);

static struct k_sem sem_rx;

static uint32_t rxmsg;

static size_t txed;
static size_t rxed;

static void mbox_rx_cb(const struct device *dev, mbox_channel_id_t channel,
			   void *user_data, struct mbox_msg *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(user_data);

	/*
	 * Only DATA words are used by this protocol. A NULL (data-less
	 * doorbell) callback is not expected; ignore it defensively.
	 */
	if (data == NULL) {
		return;
	}

	rxmsg = *(const uint32_t *)data->data;
	k_sem_give(&sem_rx);
}

int testipc_init(void)
{
	int ret;

	if (!device_is_ready(mbox_tx.dev)) {
		return -ENODEV;
	}

	if (!device_is_ready(mbox_rx.dev)) {
		return -ENODEV;
	}

	k_sem_init(&sem_rx, 0, 1);

	ret = mbox_register_callback_dt(&mbox_rx, mbox_rx_cb, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = mbox_set_enabled_dt(&mbox_rx, true);
	if (ret < 0) {
		return ret;
	}

	rxmsg = 0;

	txed = 0;
	rxed = 0;

	return 0;
}

int testipc_send(uint32_t msg)
{
		int ret;

	/* Local buffer: mbox_send_dt() copies synchronously before returning. */
	uint32_t tx = msg;
	struct mbox_msg m = {.data = &tx, .size = sizeof(tx)};

	ret = mbox_send_dt(&mbox_tx, &m);
	if (ret < 0) {
		return ret;
	}

	txed++;

	return 0;
}

int testipc_report_error(int retcode)
{
	return testipc_send(testipc_msg_make(AMP_OP_ERROR, retcode));
}

int testipc_recv(uint32_t *msg)
{
	return testipc_recv_timeout(msg, K_FOREVER);
}

int testipc_recv_timeout(uint32_t *msg, k_timeout_t timeout)
{
	int ret;

	ret = k_sem_take(&sem_rx, timeout);
	if (ret < 0) {
		return ret;
	}

	*msg = rxmsg;
	rxmsg = 0;

	rxed++;

	return 0;
}
