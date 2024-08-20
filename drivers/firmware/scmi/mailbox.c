/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include "mailbox.h"

LOG_MODULE_REGISTER(scmi_mbox);

static void scmi_mbox_cb(const struct device *mbox,
			 mbox_channel_id_t channel_id,
			 void *user_data,
			 struct mbox_msg *data)
{
	struct scmi_channel *scmi_chan = user_data;

	if (scmi_chan->cb) {
		scmi_chan->cb(scmi_chan);
	}
}

static int scmi_mbox_send_message(const struct device *transport,
				  struct scmi_channel *chan,
				  struct scmi_message *msg)
{
	struct scmi_mbox_channel *mbox_chan;
	int ret;

	mbox_chan = chan->data;

	ret = scmi_shmem_write_message(mbox_chan->shmem, msg);
	if (ret < 0) {
		LOG_ERR("failed to write message to shmem: %d", ret);
		return ret;
	}

	ret = mbox_send_dt(&mbox_chan->tx, NULL);
	if (ret < 0) {
		LOG_ERR("failed to ring doorbell: %d", ret);
		return ret;
	}

	return 0;
}

static int scmi_mbox_read_message(const struct device *transport,
				  struct scmi_channel *chan,
				  struct scmi_message *msg)
{
	struct scmi_mbox_channel *mbox_chan;

	mbox_chan = chan->data;

	return scmi_shmem_read_message(mbox_chan->shmem, msg);
}

static bool scmi_mbox_channel_is_free(const struct device *transport,
				      struct scmi_channel *chan)
{
	struct scmi_mbox_channel *mbox_chan = chan->data;

	return scmi_shmem_channel_status(mbox_chan->shmem) &
		SCMI_SHMEM_CHAN_STATUS_BUSY_BIT;
}

static int scmi_mbox_setup_chan(const struct device *transport,
				struct scmi_channel *chan,
				bool tx)
{
	int ret;
	struct scmi_mbox_channel *mbox_chan;
	struct mbox_dt_spec *tx_reply;

	mbox_chan = chan->data;

	if (!tx) {
		return -ENOTSUP;
	}

	if (mbox_chan->tx_reply.dev) {
		tx_reply = &mbox_chan->tx_reply;
	} else {
		tx_reply = &mbox_chan->tx;
	}

	ret = mbox_register_callback_dt(tx_reply, scmi_mbox_cb, chan);
	if (ret < 0) {
		LOG_ERR("failed to register tx reply cb");
		return ret;
	}

	ret = mbox_set_enabled_dt(tx_reply, true);
	if (ret < 0) {
		LOG_ERR("failed to enable tx reply dbell");
	}

	/* enable interrupt-based communication */
	scmi_shmem_update_flags(mbox_chan->shmem,
				SCMI_SHMEM_CHAN_FLAG_IRQ_BIT,
				SCMI_SHMEM_CHAN_FLAG_IRQ_BIT);

	return 0;
}

static struct scmi_transport_api scmi_mbox_api = {
	.setup_chan = scmi_mbox_setup_chan,
	.send_message = scmi_mbox_send_message,
	.read_message = scmi_mbox_read_message,
	.channel_is_free = scmi_mbox_channel_is_free,
};

DT_INST_SCMI_MAILBOX_DEFINE(0, PRE_KERNEL_1,
			    CONFIG_ARM_SCMI_TRANSPORT_INIT_PRIORITY,
			    &scmi_mbox_api);
