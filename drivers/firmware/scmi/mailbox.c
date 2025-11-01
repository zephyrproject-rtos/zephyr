/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include "mailbox.h"
#include <zephyr/drivers/firmware/scmi/protocol.h>

LOG_MODULE_REGISTER(scmi_mbox);

static int scmi_mbox_read_msg_hdr(struct scmi_channel *chan,
				struct scmi_message *msg)
{
	int ret;
	struct scmi_mbox_channel *mbox_chan = chan->data;

	/* Initialize message structure for header-only read */
	msg->hdr = 0x0;
	msg->len = sizeof(uint32_t);
	msg->content = NULL;

	ret = scmi_shmem_read_hdr(mbox_chan->shmem, msg);
	if (ret < 0) {
		LOG_ERR("failed to read message to shmem: %d", ret);
		return ret;
	}

	return 0;
}

static void scmi_mbox_tx_reply_cb(const struct device *mbox,
				mbox_channel_id_t channel_id,
				void *user_data,
				struct mbox_msg *data)
{
	struct scmi_channel *scmi_chan = user_data;
	struct scmi_message msg;

	scmi_mbox_read_msg_hdr(scmi_chan, &msg);

	if (scmi_chan->cb) {
		scmi_chan->cb(scmi_chan, msg.hdr);
	}
}

static void scmi_mbox_rx_notify_cb(const struct device *mbox,
				mbox_channel_id_t channel_id,
				void *user_data,
				struct mbox_msg *data)
{
	struct scmi_channel *scmi_chan = user_data;
	struct scmi_mbox_channel *mbox_chan = scmi_chan->data;
	const struct device *shmem = mbox_chan->shmem;
	struct scmi_message msg;

	scmi_mbox_read_msg_hdr(scmi_chan, &msg);

	if (scmi_chan->cb) {
		scmi_chan->cb(scmi_chan, msg.hdr);
		scmi_shmem_clear_channel_status(shmem);
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
		SCMI_SHMEM_CHAN_STATUS_FREE_BIT;
}

static int scmi_mbox_setup_chan(const struct device *transport,
				struct scmi_channel *chan,
				bool tx)
{
	int ret;
	struct scmi_mbox_channel *mbox_chan;
	struct mbox_dt_spec *mbox_spec;

	mbox_chan = chan->data;

	if (tx) {
		mbox_spec = mbox_chan->tx_reply.dev ? &mbox_chan->tx_reply : &mbox_chan->tx;
		ret = mbox_register_callback_dt(mbox_spec, scmi_mbox_tx_reply_cb, chan);
		if (ret < 0) {
			LOG_ERR("failed to register reply cb on %s",
					mbox_chan->tx_reply.dev ? "tx_reply" : "tx");
			return ret;
		}
	} else {
		if (!mbox_chan->rx.dev) {
			LOG_ERR("RX channel not defined");
			return -ENOTSUP;
		}
		mbox_spec = &mbox_chan->rx;
		ret = mbox_register_callback_dt(&mbox_chan->rx, scmi_mbox_rx_notify_cb, chan);
		if (ret < 0) {
			LOG_ERR("failed to register notify cb on rx");
			return ret;
		}
	}

	ret = mbox_set_enabled_dt(mbox_spec, true);
	if (ret < 0) {
		LOG_ERR("failed to enable %s dbell", tx ? "tx" : "rx");
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
