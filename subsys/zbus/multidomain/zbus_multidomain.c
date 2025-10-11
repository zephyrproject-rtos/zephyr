/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/multidomain/zbus_multidomain.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_multidomain, CONFIG_ZBUS_MULTIDOMAIN_LOG_LEVEL);

/* Forward declaration of static functions */
static int zbus_proxy_agent_send(struct zbus_proxy_agent_config *config,
				 struct zbus_proxy_agent_msg *msg, uint8_t transmit_attempts);

static void zbus_proxy_agent_sent_msg_pool_init(struct zbus_proxy_agent_config *config)
{
	if (!config) {
		LOG_ERR("Invalid proxy agent configuration for message pool init");
		return;
	}

	if (!config->sent_msg_pool) {
		LOG_ERR("No send message pool defined for proxy agent %s", config->name);
		return;
	}

	sys_slist_init(&config->sent_msg_list);
}

static struct zbus_proxy_agent_tracked_msg *
zbus_proxy_agent_find_sent_msg_data(struct zbus_proxy_agent_config *config, uint32_t msg_id)
{
	struct net_buf *buf;

	SYS_SLIST_FOR_EACH_CONTAINER(&config->sent_msg_list, buf, node) {
		uint32_t *msg_id_ptr = net_buf_user_data(buf);

		if (*msg_id_ptr == msg_id) {
			return (struct zbus_proxy_agent_tracked_msg *)buf->data;
		}
	}
	return NULL;
}

static int zbus_proxy_agent_sent_ack_timeout_stop(struct zbus_proxy_agent_config *config,
						  uint32_t msg_id)
{
	if (!config) {
		LOG_ERR("Invalid proxy agent configuration for removing sent message buffer");
		return -EINVAL;
	}

	if (!config->sent_msg_pool) {
		LOG_ERR("No send message pool defined for proxy agent %s", config->name);
		return -ENOSYS;
	}

	/* Protect list traversal and modification */
	unsigned int key = irq_lock();

	struct net_buf *prev_buf = NULL;
	struct net_buf *buf;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&config->sent_msg_list, buf, prev_buf, node) {
		uint32_t *msg_id_ptr = net_buf_user_data(buf);

		if (*msg_id_ptr == msg_id) {
			struct zbus_proxy_agent_tracked_msg *data =
				(struct zbus_proxy_agent_tracked_msg *)buf->data;

			/* Check if we're in ISR context - cannot use sync work operations */
			if (k_is_in_isr()) {
				/* In ISR context: Mark config as NULL and remove from list
				 * Cannot cancel work synchronously, but work handler will see NULL
				 * config
				 */
				data->config = NULL;
				sys_slist_remove(&config->sent_msg_list,
						 prev_buf ? &prev_buf->node : NULL, &buf->node);
				net_buf_unref(buf);
				irq_unlock(key);
				LOG_DBG("ACK received in ISR context for message ID %d, removed "
					"from tracking",
					msg_id);
				return 0;
			} else if (k_current_get() != &k_sys_work_q.thread) {
				/* In thread context (not work queue): Safe to cancel work
				 * synchronously
				 */
				struct k_work_sync sync;

				k_work_cancel_delayable_sync(&data->work, &sync);
			} else {
				/* In work queue context: Mark as NULL to prevent retransmission */
				data->config = NULL;
			}

			sys_slist_remove(&config->sent_msg_list, prev_buf ? &prev_buf->node : NULL,
					 &buf->node);
			net_buf_unref(buf);
			irq_unlock(key);
			return 0;
		}
	}

	irq_unlock(key);
	LOG_WRN("Sent message ID %d not found in list of tracked messages", msg_id);
	return -ENOENT;
}

static void zbus_proxy_agent_sent_ack_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct zbus_proxy_agent_tracked_msg *data =
		CONTAINER_OF(dwork, struct zbus_proxy_agent_tracked_msg, work);
	if (!data) {
		LOG_ERR("Invalid sent message data in timeout handler");
		return;
	}
	uint32_t expected_msg_id = data->msg.id;

	if (!data->config) {
		LOG_DBG("Timeout handler called for message ID %d but config is NULL, likely "
			"already ACKed",
			expected_msg_id);
		return;
	}

	unsigned int key = irq_lock();
	struct zbus_proxy_agent_tracked_msg *current_data =
		zbus_proxy_agent_find_sent_msg_data(data->config, expected_msg_id);

	if (current_data != data) {
		irq_unlock(key);
		LOG_DBG("Timeout handler called for message ID %d but message no longer in "
			"tracking list, likely already ACKed",
			expected_msg_id);
		return;
	}

	if (k_work_delayable_is_pending(&data->work) == false) {
		irq_unlock(key);
		LOG_DBG("Timeout work for message ID %d was cancelled while waiting for lock",
			expected_msg_id);
		return;
	}

	irq_unlock(key);

	LOG_WRN("Sent message ID %d timed out waiting for acknowledgment", expected_msg_id);

	data->transmit_attempts++;
	if (data->transmit_attempts < CONFIG_ZBUS_MULTIDOMAIN_MAX_TRANSMIT_ATTEMPTS) {
		LOG_WRN("Retrying to send message ID %d (attempt %d)", expected_msg_id,
			data->transmit_attempts);
		if (data->config) {
			int ret = zbus_proxy_agent_send(data->config, &data->msg,
							data->transmit_attempts);
			if (ret < 0) {
				LOG_ERR("Failed to resend message ID %d: %d", expected_msg_id, ret);
			} else {
				LOG_DBG("Resent message ID %d (attempt %d)", expected_msg_id,
					data->transmit_attempts);
			}
		}
	} else {
		LOG_ERR("Max transmit attempts (%d) reached for message ID %d, giving up",
			CONFIG_ZBUS_MULTIDOMAIN_MAX_TRANSMIT_ATTEMPTS, expected_msg_id);
		if (data->config) {
			int ret = zbus_proxy_agent_sent_ack_timeout_stop(data->config,
									 expected_msg_id);
			if (ret < 0 && ret != -ENOENT) {
				/* -ENOENT means ACK already arrived and removed the message, which
				 * is fine
				 */
				LOG_ERR("Failed to remove sent message ID %d from tracking pool: "
					"%d",
					expected_msg_id, ret);
			}
		}
	}
}

static int zbus_proxy_agent_sent_ack_timeout_start(struct zbus_proxy_agent_config *config,
						   struct zbus_proxy_agent_msg *msg,
						   uint8_t transmit_attempts)
{
	if (!config || !msg) {
		LOG_ERR("Invalid parameters for adding sent message buffer");
		return -EINVAL;
	}

	if (!config->sent_msg_pool) {
		LOG_ERR("No send message pool defined for proxy agent %s", config->name);
		return -ENOSYS;
	}

	unsigned int key = irq_lock();

	struct zbus_proxy_agent_tracked_msg *data =
		zbus_proxy_agent_find_sent_msg_data(config, msg->id);
	if (data) {
		/* Message is already being tracked, just reschedule the timeout */
		data->transmit_attempts = transmit_attempts;
		if (&data->msg != msg) {
			memcpy(&data->msg, msg, sizeof(*msg));
		}
		uint32_t timeout_ms = CONFIG_ZBUS_MULTIDOMAIN_SENT_MSG_ACK_TIMEOUT
				      << transmit_attempts;
		if (timeout_ms > CONFIG_ZBUS_MULTIDOMAIN_SENT_MSG_ACK_TIMEOUT_MAX) {
			timeout_ms = CONFIG_ZBUS_MULTIDOMAIN_SENT_MSG_ACK_TIMEOUT_MAX;
		}
		k_work_reschedule(&data->work, K_MSEC(timeout_ms));
		irq_unlock(key);
		LOG_DBG("Rescheduled ACK timeout for message ID %d (attempts: %d, timeout: %d ms)",
			msg->id, transmit_attempts, timeout_ms);
		return 0;
	}

	struct net_buf *buf = net_buf_alloc(config->sent_msg_pool, K_NO_WAIT);

	if (!buf) {
		irq_unlock(key);
		LOG_ERR("Sent message pool full, cannot track message ID %d for proxy agent %s",
			msg->id, config->name);
		return -ENOMEM;
	}

	data = net_buf_add(buf, sizeof(struct zbus_proxy_agent_tracked_msg));
	if (!data) {
		net_buf_unref(buf);
		irq_unlock(key);
		return -ENOMEM;
	}
	data->config = config;
	data->transmit_attempts = transmit_attempts;
	if (&data->msg != msg) {
		memcpy(&data->msg, msg, sizeof(*msg));
	}
	k_work_init_delayable(&data->work, zbus_proxy_agent_sent_ack_timeout_handler);

	uint32_t *msg_id_ptr = net_buf_user_data(buf);
	*msg_id_ptr = msg->id;
	sys_slist_append(&config->sent_msg_list, &buf->node);
	uint32_t timeout_ms = CONFIG_ZBUS_MULTIDOMAIN_SENT_MSG_ACK_TIMEOUT << transmit_attempts;

	LOG_DBG("Scheduling ACK timeout for message ID %d in %d ms (attempts: %d)", msg->id,
		timeout_ms, transmit_attempts);

	k_work_schedule_for_queue(&k_sys_work_q, &data->work, K_MSEC(timeout_ms));

	irq_unlock(key);
	return 0;
}

static int zbus_proxy_agent_set_recv_cb(struct zbus_proxy_agent_config *config,
					int (*recv_cb)(const struct zbus_proxy_agent_msg *msg))
{
	int ret;

	if (!config || !config->api || !config->backend_config) {
		LOG_ERR("Invalid proxy agent configuration");
		return -EINVAL;
	}

	if (!config->api->backend_set_recv_cb) {
		LOG_ERR("Backend set receive callback function is not defined");
		return -ENOSYS;
	}

	ret = config->api->backend_set_recv_cb(config->backend_config, recv_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set receive callback for proxy agent %s: %d", config->name, ret);
		return ret;
	}

	LOG_DBG("Receive callback set successfully for proxy agent %s", config->name);
	return 0;
}

static int zbus_proxy_agent_set_ack_cb(struct zbus_proxy_agent_config *config,
				       int (*ack_cb)(uint32_t msg_id, void *user_data))
{
	if (!config || !config->api || !config->backend_config) {
		LOG_ERR("Invalid proxy agent configuration");
		return -EINVAL;
	}

	if (!config->api->backend_set_ack_cb) {
		LOG_ERR("Backend set ACK callback function is not defined");
		return -ENOSYS;
	}

	int ret = config->api->backend_set_ack_cb(config->backend_config, ack_cb, config);

	if (ret < 0) {
		LOG_ERR("Failed to set ACK callback for proxy agent %s: %d", config->name, ret);
		return ret;
	}

	LOG_DBG("ACK callback set successfully for proxy agent %s", config->name);
	return 0;
}

static int zbus_proxy_agent_init(struct zbus_proxy_agent_config *config)
{
	int ret;

	if (!config || !config->api || !config->backend_config) {
		LOG_ERR("Invalid proxy agent configuration");
		return -EINVAL;
	}

	if (!config->api->backend_init) {
		LOG_ERR("Backend init function is not defined");
		return -ENOSYS;
	}

	ret = config->api->backend_init(config->backend_config);
	if (ret < 0) {
		LOG_ERR("Failed to initialize backend for proxy agent %s: %d", config->name, ret);
		return ret;
	}

	LOG_DBG("Proxy agent %s of type %d initialized successfully", config->name, config->type);

	return 0;
}

static int zbus_proxy_agent_send(struct zbus_proxy_agent_config *config,
				 struct zbus_proxy_agent_msg *msg, uint8_t transmit_attempts)
{
	int ret;

	if (!config || !config->api || !msg) {
		LOG_ERR("Invalid parameters for sending message");
		return -EINVAL;
	}

	if (!config->api->backend_send) {
		LOG_ERR("Backend send function is not defined");
		return -ENOSYS;
	}

	/* Add message to tracking pool before sending to avoid race condition with ACKs */
	if (config->sent_msg_pool) {
		ret = zbus_proxy_agent_sent_ack_timeout_start(config, msg, transmit_attempts);
		if (ret < 0) {
			LOG_ERR("Failed to track sent message ID %d for proxy agent %s: %d",
				msg->id, config->name, ret);
			return ret;
		}
	}

	ret = config->api->backend_send((void *)config->backend_config, msg);
	if (ret < 0) {
		LOG_ERR("Failed to send message via proxy agent %s: %d", config->name, ret);

		/* Remove from tracking pool since send failed */
		if (config->sent_msg_pool) {
			ret = zbus_proxy_agent_sent_ack_timeout_stop(config, msg->id);
			if (ret < 0) {
				LOG_ERR("Failed to cleanup tracked message ID %d after send "
					"failure: %d",
					msg->id, ret);
			}
		}
		return ret;
	}

	LOG_DBG("Message sent successfully via proxy agent %s", config->name);
	return 0;
}

static int zbus_proxy_agent_msg_recv_cb(const struct zbus_proxy_agent_msg *msg)
{
	int ret;

	/* Find corresponding channel by name.
	 * TODO: Would create less overhead if using ID, but that would require ID to be enabled and
	 * stricter control of ID assignment.
	 */
	const struct zbus_channel *chan;

	if (!msg) {
		LOG_ERR("Received NULL message in callback");
		return -EINVAL;
	}

	chan = zbus_chan_from_name(msg->channel_name);

	if (!chan) {
		LOG_ERR("No channel found for message with name %s", msg->channel_name);
		return -ENOENT;
	}
	if (!chan->is_shadow_channel) {
		LOG_ERR("Channel %s is not a shadow channel, cannot process message", chan->name);
		return -EPERM;
	}

	ret = zbus_chan_pub_shadow(chan, msg->message_data, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to publish shadow message on channel %s: %d", chan->name, ret);
		return ret;
	}

	LOG_DBG("Published message on shadow channel %s", chan->name);

	return 0;
}

static int zbus_proxy_agent_msg_ack_cb(uint32_t msg_id, void *user_data)
{
	if (!user_data) {
		LOG_ERR("Received NULL user data in ACK callback");
		return -EINVAL;
	}

	struct zbus_proxy_agent_config *config = (struct zbus_proxy_agent_config *)user_data;

	LOG_DBG("Received ACK for message ID %d via proxy agent %s", msg_id, config->name);

	int ret = zbus_proxy_agent_sent_ack_timeout_stop(config, msg_id);

	if (ret < 0) {
		if (ret == -ENOENT) {
			LOG_DBG("Message ID %d was not found in tracking pool (already processed "
				"or never tracked)",
				msg_id);
			return -ENOENT;
		}
		LOG_WRN("Failed to remove sent message ID %d from tracking pool: %d", msg_id, ret);
		return ret;
	}

	LOG_DBG("Successfully processed ACK for message ID %d", msg_id);
	return 0;
}

int zbus_proxy_agent_thread(struct zbus_proxy_agent_config *config,
			    const struct zbus_observer *subscriber)
{
	int ret;
	uint8_t message_data[CONFIG_ZBUS_MULTIDOMAIN_MESSAGE_SIZE] = {0};
	const struct zbus_channel *chan;

	if (!config) {
		LOG_ERR("Invalid proxy agent configuration for thread");
		return -EINVAL;
	}

	LOG_DBG("Starting thread for proxy agent %s", config->name);

	ret = zbus_proxy_agent_set_recv_cb(config, zbus_proxy_agent_msg_recv_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set receive callback for proxy agent %s: %d", config->name, ret);
		return ret;
	}

	ret = zbus_proxy_agent_set_ack_cb(config, zbus_proxy_agent_msg_ack_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set ACK callback for proxy agent %s: %d", config->name, ret);
		return ret;
	}

	zbus_proxy_agent_sent_msg_pool_init(config);

	ret = zbus_proxy_agent_init(config);
	if (ret < 0) {
		LOG_ERR("Failed to initialize proxy agent %s: %d", config->name, ret);
		return ret;
	}

	while (!zbus_sub_wait_msg(subscriber, &chan, &message_data, K_FOREVER)) {
		struct zbus_proxy_agent_msg msg;

		if (ZBUS_CHANNEL_IS_SHADOW(chan)) {
			LOG_ERR("Forwarding of shadow channel %s, not supported by proxy agent",
				chan->name);
			continue;
		}

		ret = zbus_create_proxy_agent_msg(&msg, message_data, chan->message_size,
						  chan->name, strlen(chan->name));
		if (ret < 0) {
			LOG_ERR("Failed to create proxy agent message for channel %s: %d",
				chan->name, ret);
			continue;
		}

		ret = zbus_proxy_agent_send(config, &msg, 0);
		if (ret < 0) {
			LOG_ERR("Failed to send message via proxy agent %s: %d", config->name, ret);
		}
	}
	return 0;
}
