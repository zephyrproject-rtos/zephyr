/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/protocol.h>
#include <zephyr/drivers/firmware/scmi/transport.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

LOG_MODULE_REGISTER(scmi_core);

#define SCMI_CHAN_LOCK_TIMEOUT_USEC 500
#define SCMI_CHAN_SEM_TIMEOUT_USEC 500

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

static void scmi_core_reply_cb(struct scmi_channel *chan)
{
	if (!k_is_pre_kernel()) {
		k_sem_give(&chan->sem);
	}
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

	/* no support for RX channels ATM */
	if (!tx) {
		return -ENOTSUP;
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

static int scmi_send_message_pre_kernel(struct scmi_protocol *proto,
					struct scmi_message *msg,
					struct scmi_message *reply)
{
	int ret;

	ret = scmi_transport_send_message(proto->transport, proto->tx, msg);
	if (ret < 0) {
		return ret;
	}

	/* no kernel primitives, we're forced to poll here.
	 *
	 * Cortex-M quirk: no interrupts at this point => no timer =>
	 * no timeout mechanism => this can block the whole system.
	 *
	 * TODO: is there a better way to handle this?
	 */
	while (!scmi_transport_channel_is_free(proto->transport, proto->tx)) {
	}

	ret = scmi_transport_read_message(proto->transport, proto->tx, reply);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int scmi_send_message_post_kernel(struct scmi_protocol *proto,
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
		      struct scmi_message *reply)
{
	if (!proto->tx) {
		return -ENODEV;
	}

	if (!proto->tx->ready) {
		return -EINVAL;
	}

	if (k_is_pre_kernel()) {
		return scmi_send_message_pre_kernel(proto, msg, reply);
	} else {
		return scmi_send_message_post_kernel(proto, msg, reply);
	}
}

static int scmi_core_protocol_setup(const struct device *transport)
{
	int ret;

	STRUCT_SECTION_FOREACH(scmi_protocol, it) {
		it->transport = transport;

#ifndef CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS
		/* no static channel allocation, attempt dynamic binding */
		it->tx = scmi_transport_request_channel(transport, it->id, true);
#endif /* CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS */

		if (!it->tx) {
			return -ENODEV;
		}

		ret = scmi_core_setup_chan(transport, it->tx, true);
		if (ret < 0) {
			return ret;
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
