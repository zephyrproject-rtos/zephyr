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

static int scmi_core_wait_reply(struct scmi_protocol *proto, bool use_polling)
{
	if (!use_polling) {
		return k_sem_take(&proto->tx->sem,
				  K_USEC(CONFIG_ARM_SCMI_CHAN_SEM_TIMEOUT_USEC));
	}

	while (!scmi_transport_channel_is_free(proto->transport, proto->tx)) {
	}

	return 0;
}

int scmi_send_message(struct scmi_protocol *proto, struct scmi_message *msg,
		      struct scmi_message *reply, bool use_polling)
{
	int ret;

	if (!proto->tx) {
		return -ENODEV;
	}

	if (!proto->tx->ready) {
		return -EINVAL;
	}

	/*
	 * while in PRE_KERNEL, interrupt-based messaging is forbidden
	 * because of the lack of kernel primitives (i.e. semaphores)
	 * (and potentially even interrupts, based on the architecture).
	 * Because of this, we ignore the caller's request and forcibly
	 * enable polling.
	 */
	use_polling = k_is_pre_kernel() ? true : use_polling;

	if (!k_is_pre_kernel()) {
		ret = k_mutex_lock(&proto->tx->lock, K_USEC(SCMI_CHAN_LOCK_TIMEOUT_USEC));
		if (ret < 0) {
			LOG_ERR("failed to acquire TX channel lock: %d", ret);
			return ret;
		}
	}

	ret = scmi_transport_send_message(proto->transport, proto->tx, msg, use_polling);
	if (ret < 0) {
		LOG_ERR("failed to send message at transport layer: %d", ret);
		goto out_release_mutex;
	}

	ret = scmi_core_wait_reply(proto, use_polling);
	if (ret < 0) {
		LOG_ERR("failed to wait for message reply: %d", ret);
		goto out_release_mutex;
	}

	ret = scmi_transport_read_message(proto->transport, proto->tx, reply);
	if (ret < 0) {
		LOG_ERR("failed to read message reply: %d", ret);
		goto out_release_mutex;
	}

out_release_mutex:
	if (!k_is_pre_kernel()) {
		k_mutex_unlock(&proto->tx->lock);
	}

	return ret;
}

static int scmi_core_protocol_negotiate(struct scmi_protocol *proto)

{
	uint32_t agent_version, platform_version;
	int ret;

	if (!proto) {
		return -EINVAL;
	}

	agent_version = proto->version;

	if (!agent_version) {
		LOG_ERR("Protocol 0x%X: Agent version not specified", proto->id);
		return -EINVAL;
	}

	ret = scmi_protocol_get_version(proto, &platform_version);
	if (ret < 0) {
		LOG_ERR("Protocol 0x%X: Failed to get platform version: %d",
			proto->id, ret);
		return ret;
	}

	if (platform_version > agent_version) {
		ret = scmi_protocol_version_negotiate(proto, agent_version);
		if (ret < 0) {
			LOG_WRN("Protocol 0x%X: Negotiation failed (%d). "
				"Platform v0x%08x does not support downgrade to agent v0x%08x",
				proto->id, ret, platform_version, agent_version);
		}
	}

	LOG_INF("Using protocol 0x%X: agent version 0x%08x, platform version 0x%08x",
			proto->id, agent_version, platform_version);

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
#endif /* CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS */

		if (!it->tx) {
			return -ENODEV;
		}

		ret = scmi_core_setup_chan(transport, it->tx, true);
		if (ret < 0) {
			return ret;
		}

		ret = scmi_core_protocol_negotiate(it);
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
