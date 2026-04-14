/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_proxy_agent, CONFIG_ZBUS_PROXY_AGENT_LOG_LEVEL);

static struct zbus_proxy_agent_work_ctx *zbus_proxy_agent_work_ctx_from_work(struct k_work *work)
{
	return (struct zbus_proxy_agent_work_ctx *)
		((char *)work - offsetof(struct zbus_proxy_agent_work_ctx, work));
}

void zbus_proxy_agent_log_shadow_pub_denied(const struct zbus_proxy_agent *agent)
{
	LOG_ERR("Shadow channel can only be published by proxy agent '%s'", agent->name);
}

static bool is_shadow_channel_for_proxy(const struct zbus_channel *chan,
					const struct zbus_proxy_agent *agent)
{
	STRUCT_SECTION_FOREACH(zbus_shadow_channel, shadow_chan) {
		if (shadow_chan->chan == chan && shadow_chan->proxy_agent == agent) {
			return true;
		}
	}

	return false;
}

static void zbus_proxy_agent_submit_work(struct zbus_proxy_agent_work_ctx *work_ctx,
					 const char *work_name)
{
	int ret = k_work_submit_to_queue(work_ctx->agent->work_q, &work_ctx->work);

	if (ret < 0) {
		LOG_ERR("Failed to submit %s work for proxy agent '%s': %d", work_name,
			work_ctx->agent->name, ret);
	}
}

static void zbus_proxy_agent_rx_work_handler(struct k_work *work)
{
	struct zbus_proxy_agent_work_ctx *work_ctx = zbus_proxy_agent_work_ctx_from_work(work);
	const struct zbus_proxy_agent *agent = work_ctx->agent;
	struct zbus_proxy_agent_rx_msg msg;
	int ret;

	while (k_msgq_get(agent->rx_msgq, &msg, K_NO_WAIT) == 0) {
		atomic_set(agent->shadow_pub_active, 1);
		ret = zbus_chan_pub(msg.chan, msg.message, K_NO_WAIT);
		atomic_clear(agent->shadow_pub_active);
		if (ret < 0) {
			LOG_ERR("Failed to publish message on channel '%s': %d",
				_ZBUS_CHAN_NAME(msg.chan), ret);
		}
	}
}

static void zbus_proxy_agent_tx_work_handler(struct k_work *work)
{
	struct zbus_proxy_agent_work_ctx *work_ctx = zbus_proxy_agent_work_ctx_from_work(work);
	const struct zbus_proxy_agent *agent = work_ctx->agent;
	struct zbus_proxy_msg msg = {0};
	int ret;

	while (k_msgq_get(agent->tx_msgq, &msg, K_NO_WAIT) == 0) {
		ret = agent->backend_api->backend_send(agent, &msg);
		if (ret < 0) {
			LOG_ERR("Failed to send message via proxy agent '%s' backend: %d",
				agent->name, ret);
		}
	}
}

void zbus_proxy_agent_listener_cb(const struct zbus_channel *chan,
				  const struct zbus_proxy_agent *agent)
{
	const void *msg;
	const char *chan_name;
	size_t chan_name_len;
	struct zbus_proxy_msg tx_msg = {0};
	int ret;

	if (chan->message_size > CONFIG_ZBUS_PROXY_AGENT_MAX_MESSAGE_SIZE) {
		LOG_ERR("Message size %zu exceeds maximum %d in proxy agent listener callback",
			chan->message_size, CONFIG_ZBUS_PROXY_AGENT_MAX_MESSAGE_SIZE);
		return;
	}

	LOG_DBG("Received message on channel '%s' for proxy agent '%s'", chan->name, agent->name);

	msg = zbus_chan_const_msg(chan);
	chan_name = _ZBUS_CHAN_NAME(chan);
	chan_name_len = strlen(chan_name) + 1U;

	if (chan_name_len > sizeof(tx_msg.channel_name)) {
		LOG_ERR("Channel name '%s' too long for proxy transport (%zu > %zu)", chan_name,
			chan_name_len, sizeof(tx_msg.channel_name));
		return;
	}

	tx_msg.message_size = chan->message_size;
	memcpy(tx_msg.message, msg, chan->message_size);
	tx_msg.channel_name_len = chan_name_len;
	memcpy(tx_msg.channel_name, chan_name, chan_name_len);

	ret = k_msgq_put(agent->tx_msgq, &tx_msg, K_NO_WAIT);
	if (ret < 0) {
		LOG_WRN("Transmit queue full for proxy agent %s, dropping message", agent->name);
		return;
	}

	zbus_proxy_agent_submit_work(agent->tx_work, "transmit");
}

static int zbus_proxy_agent_receive_cb(const struct zbus_proxy_agent *agent,
				       const struct zbus_proxy_msg *msg)
{
	const struct zbus_channel *chan;
	struct zbus_proxy_agent_rx_msg queued_msg = {0};
	int ret;

	if (msg->message_size == 0 || msg->message_size > sizeof(msg->message)) {
		LOG_ERR("Invalid message size %u in proxy agent receive callback",
			msg->message_size);
		return -EMSGSIZE;
	}

	if (msg->channel_name_len == 0 ||
	    msg->channel_name_len > CONFIG_ZBUS_PROXY_AGENT_MAX_CHANNEL_NAME_SIZE ||
	    msg->channel_name[msg->channel_name_len - 1] != '\0') {
		LOG_ERR("Invalid channel name length or missing NUL terminator in received proxy "
			"message");
		return -EINVAL;
	}

	chan = zbus_chan_from_name(msg->channel_name);
	if (chan == NULL) {
		LOG_ERR("Channel '%s' not found for received message", msg->channel_name);
		return -ENOENT;
	}

	if (!is_shadow_channel_for_proxy(chan, agent)) {
		LOG_ERR("Channel '%s' is not a shadow channel for proxy agent '%s'",
			msg->channel_name, agent->name);
		return -EINVAL;
	}

	if (msg->message_size != chan->message_size) {
		LOG_ERR("Size mismatch for channel '%s': got %u expected %zu", msg->channel_name,
			msg->message_size, chan->message_size);
		return -EMSGSIZE;
	}

	queued_msg.chan = chan;
	memcpy(queued_msg.message, msg->message, msg->message_size);

	ret = k_msgq_put(agent->rx_msgq, &queued_msg, K_NO_WAIT);
	if (ret < 0) {
		LOG_WRN("Receive queue full for proxy agent %s, dropping message", agent->name);
		return ret;
	}

	zbus_proxy_agent_submit_work(agent->rx_work, "receive");

	return 0;
}

int zbus_init_proxy_agent(const struct zbus_proxy_agent *agent)
{
	int ret;

	_ZBUS_ASSERT(agent != NULL, "Proxy agent configuration is NULL in init");
	_ZBUS_ASSERT(agent->backend_api != NULL, "Backend API is NULL in proxy agent init");
	_ZBUS_ASSERT(agent->rx_work != NULL, "RX work item is NULL in proxy agent init");
	_ZBUS_ASSERT(agent->tx_work != NULL, "TX work item is NULL in proxy agent init");
	_ZBUS_ASSERT(agent->rx_msgq != NULL, "RX message queue is NULL in proxy agent init");
	_ZBUS_ASSERT(agent->tx_msgq != NULL, "TX message queue is NULL in proxy agent init");
	_ZBUS_ASSERT(agent->work_q != NULL, "Workqueue is NULL in proxy agent init");
	_ZBUS_ASSERT(agent->shadow_pub_active != NULL,
		     "Shadow publish flag is NULL in proxy agent init");

	k_work_init(&agent->rx_work->work, zbus_proxy_agent_rx_work_handler);
	agent->rx_work->agent = agent;
	k_work_init(&agent->tx_work->work, zbus_proxy_agent_tx_work_handler);
	agent->tx_work->agent = agent;
	atomic_clear(agent->shadow_pub_active);

	ret = agent->backend_api->backend_set_recv_cb(agent, zbus_proxy_agent_receive_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set receive callback for proxy agent %s: %d", agent->name, ret);
		return ret;
	}

	ret = agent->backend_api->backend_init(agent);
	if (ret < 0) {
		LOG_ERR("Failed to initialize backend for proxy agent %s: %d", agent->name, ret);
		return ret;
	}

	LOG_DBG("Proxy agent %s initialized successfully", agent->name);

	return 0;
}
