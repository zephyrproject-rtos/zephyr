/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/protocol.h>
#include <zephyr/drivers/firmware/scmi/transport.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include "mailbox.h"

LOG_MODULE_REGISTER(scmi_core);

#define SCMI_CHAN_LOCK_TIMEOUT_USEC 500
#define SCMI_CHAN_SEM_TIMEOUT_USEC 500

static sys_slist_t scmi_event_list;

int scmi_status_to_errno(int scmi_status)
{
	switch (scmi_status) {
	case SCMI_SUCCESS:
		return 0;
	case SCMI_NOT_SUPPORTED:
		return -EOPNOTSUPP;
	case SCMI_INVALID_PARAMETERS:
		return -EINVAL;
	case SCMI_DENIED:
		return -EACCES;
	case SCMI_NOT_FOUND:
		return -ENOENT;
	case SCMI_OUT_OF_RANGE:
		return -ERANGE;
	case SCMI_IN_USE:
	case SCMI_BUSY:
		return -EBUSY;
	case SCMI_PROTOCOL_ERROR:
		return -EPROTO;
	case SCMI_COMMS_ERROR:
	case SCMI_GENERIC_ERROR:
	case SCMI_HARDWARE_ERROR:
	default:
		return -EIO;
	}
}

static void scmi_core_reply_cb(struct scmi_channel *chan, struct scmi_message msg)
{
	uint32_t protocol_id, msg_id;
	struct scmi_protocol_event *event;

	if (!k_is_pre_kernel()) {
		k_sem_give(&chan->sem);
	}

	protocol_id = SCMI_MESSAGE_HDR_EX_PROTOCOL(msg.hdr);
	msg_id = SCMI_MESSAGE_HDR_EX_MSGID(msg.hdr);

	SYS_SLIST_FOR_EACH_CONTAINER(&scmi_event_list, event, node) {
		if (protocol_id == event->id) {
			for (uint32_t num = 0; num < event->num_events; num++) {
				if (msg_id == event->evts[num]) {
					event->cb(chan, msg);
				}
			}
		}
	}

	LOG_WRN("Unhandled SCMI event: proto=%d evt=%d", protocol_id, msg_id);
}

static int scmi_core_setup_chan(const struct device *transport,
				struct scmi_channel *chan, bool tx)
{
	int ret;

	if (!chan) {
		return -EINVAL;
	}

	if (chan->ready) {
		return 0;
	}

	k_mutex_init(&chan->lock);
	k_sem_init(&chan->sem, 0, 1);

	chan->cb = scmi_core_reply_cb;

	/* setup transport-related channel data */
	ret = scmi_transport_setup_chan(transport, chan, tx);
	if (ret < 0) {
		LOG_ERR("failed to setup channel");
		return ret;
	}

	/* protocols might share a channel. In such cases, this
	 * will stop them from being initialized again.
	 */
	chan->ready = true;

	return 0;
}

static int scmi_interrupt_enable(struct scmi_channel *chan, bool enable)
{
	struct scmi_mbox_channel *mbox_chan;
	struct mbox_dt_spec *tx_reply;
	bool comp_int;

	mbox_chan = chan->data;
	comp_int = enable ? SCMI_SHMEM_CHAN_FLAG_IRQ_BIT : 0;

	if (mbox_chan->tx_reply.dev) {
		tx_reply = &mbox_chan->tx_reply;
	} else {
		tx_reply = &mbox_chan->tx;
	}

	/* re-set completion interrupt */
	scmi_shmem_update_flags(mbox_chan->shmem, SCMI_SHMEM_CHAN_FLAG_IRQ_BIT, comp_int);

	return mbox_set_enabled_dt(tx_reply, enable);
}

static int scmi_send_message_polling(struct scmi_protocol *proto,
					struct scmi_message *msg,
					struct scmi_message *reply)
{
	int ret;
	int status;

	/*
	 * SCMI communication interrupt is enabled by default during setup_chan
	 * to support interrupt-driven communication. When using polling mode
	 * it must be disabled to avoid unnecessary interrupts and
	 * ensure proper polling behavior.
	 */
	status = scmi_interrupt_enable(proto->tx, false);

	ret = scmi_transport_send_message(proto->transport, proto->tx, msg);
	if (ret < 0) {
		goto cleanup;
	}

	/* no kernel primitives, we're forced to poll here.
	 *
	 * Cortex-M quirk: no interrupts at this point => no timer =>
	 * no timeout mechanism => this can block the whole system.
	 *
	 * Polling mode repeatedly checks the chan_status field in share memory
	 * to detect whether the remote side have completed message processing
	 *
	 * TODO: is there a better way to handle this?
	 */
	while (!scmi_transport_channel_is_free(proto->transport, proto->tx)) {
	}

	ret = scmi_transport_read_message(proto->transport, proto->tx, reply);
	if (ret < 0) {
		return ret;
	}

cleanup:
	/* restore scmi interrupt enable status when disable it pass */
	if (status >= 0) {
		scmi_interrupt_enable(proto->tx, true);
	}

	return ret;
}

static int scmi_send_message_interrupt(struct scmi_protocol *proto,
					 struct scmi_message *msg,
					 struct scmi_message *reply)
{
	int ret = 0;

	if (!proto->tx) {
		return -ENODEV;
	}

	/* wait for channel to be free */
	ret = k_mutex_lock(&proto->tx->lock, K_USEC(SCMI_CHAN_LOCK_TIMEOUT_USEC));
	if (ret < 0) {
		LOG_ERR("failed to acquire chan lock");
		return ret;
	}

	ret = scmi_transport_send_message(proto->transport, proto->tx, msg);
	if (ret < 0) {
		LOG_ERR("failed to send message");
		goto out_release_mutex;
	}

	/* only one protocol instance can wait for a message reply at a time */
	ret = k_sem_take(&proto->tx->sem, K_USEC(SCMI_CHAN_SEM_TIMEOUT_USEC));
	if (ret < 0) {
		LOG_ERR("failed to wait for msg reply");
		goto out_release_mutex;
	}

	ret = scmi_transport_read_message(proto->transport, proto->tx, reply);
	if (ret < 0) {
		LOG_ERR("failed to read reply");
		goto out_release_mutex;
	}

out_release_mutex:
	k_mutex_unlock(&proto->tx->lock);

	return ret;
}

int scmi_send_message(struct scmi_protocol *proto, struct scmi_message *msg,
		      struct scmi_message *reply, bool use_polling)
{
	if (!proto->tx) {
		return -ENODEV;
	}

	if (!proto->tx->ready) {
		return -EINVAL;
	}

	if (use_polling) {
		return scmi_send_message_polling(proto, msg, reply);
	} else {
		return scmi_send_message_interrupt(proto, msg, reply);
	}
}

int scmi_read_message(struct scmi_protocol *proto, struct scmi_message *msg)
{
	int ret = 0;

	if (!proto->rx) {
		return -ENODEV;
	}

	if (!proto->rx->ready) {
		return -EINVAL;
	}

	/* read message from platform, such as notification event
	 *
	 * Unlike scmi_send_message, reading messages with scmi_read_message is not currently
	 * required in the PRE_KERNEL stage. The interrupt-based logic is used here.
	 */
	ret = scmi_transport_read_message(proto->transport, proto->rx, msg);

	return ret;
}

int scmi_register_protocol_event_handler(struct scmi_protocol_event *event)
{
	if (!event || !event->cb || !event->evts || event->num_events == 0U) {
		return -EINVAL;
	}

	/* Append the protocol event node to the global SCMI event handler list */
	sys_slist_append(&scmi_event_list, &event->node);

	return 0;
}

static int scmi_core_protocol_setup(const struct device *transport)
{
	int ret;

	STRUCT_SECTION_FOREACH(scmi_protocol, it) {
		it->transport = transport;

#ifndef CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS
		/* no static channel allocation, attempt dynamic binding */
		it->tx = scmi_transport_request_channel(transport, it->id, true);
		it->rx = scmi_transport_request_channel(transport, it->id, false);
#endif /* CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS */

		if (!it->tx) {
			return -ENODEV;
		}

		ret = scmi_core_setup_chan(transport, it->tx, true);
		if (ret < 0) {
			return ret;
		}

		/* notification/delayed reply channel is optional */
		if (it->rx) {
			ret = scmi_core_setup_chan(transport, it->rx, false);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return 0;
}

int scmi_core_transport_init(const struct device *transport)
{
	int ret;

	ret = scmi_transport_init(transport);
	if (ret < 0) {
		return ret;
	}

	return scmi_core_protocol_setup(transport);
}
