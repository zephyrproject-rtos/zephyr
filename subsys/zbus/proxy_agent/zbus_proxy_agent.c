/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_proxy_agent, CONFIG_ZBUS_PROXY_AGENT_LOG_LEVEL);

/* Forward declaration */
static int zbus_proxy_agent_send(struct zbus_proxy_agent_config *config,
				 struct zbus_proxy_agent_msg *msg, uint8_t transmit_attempts);
static int zbus_proxy_agent_stop_tracking(struct zbus_proxy_agent_config *config, uint32_t msg_id);
static void zbus_proxy_agent_ack_timeout_handler(struct k_work *work);

static bool is_duplicate_message(struct zbus_proxy_agent_config *config, uint32_t msg_id)
{
	if (!config) {
		return -EINVAL;
	}

	for (int i = 0; i < config->duplicate_detection.detection_buffer_size; i++) {
		if (config->duplicate_detection.detection_buffer[i] == msg_id) {
			LOG_DBG("Detected duplicate message ID %d", msg_id);
			return true;
		}
	}
	return false;
}

static void add_to_duplicate_detection(struct zbus_proxy_agent_config *config, uint32_t msg_id)
{
	if (!config) {
		return;
	}

	config->duplicate_detection.detection_buffer[config->duplicate_detection.detection_head] =
		msg_id;
	config->duplicate_detection.detection_head =
		(config->duplicate_detection.detection_head + 1) %
		config->duplicate_detection.detection_buffer_size;

	LOG_DBG("Added message ID %d to duplicate detection buffer at position %d", msg_id,
		config->duplicate_detection.detection_head);
}

static uint32_t calculate_backoff_timeout(struct zbus_proxy_agent_config *config, uint8_t attempts)
{
	uint32_t multiplier;
	uint32_t timeout;

	multiplier = 1U << attempts;

	/* Check for overflow */
	if (attempts >= 32 || config->tracking.ack_timeout_start_ms > (UINT32_MAX >> attempts)) {
		return config->tracking.ack_timeout_max_ms;
	}

	timeout = config->tracking.ack_timeout_start_ms * multiplier;

	/* Cap to maximum timeout if defined */
	if (config->tracking.ack_timeout_max_ms > 0 &&
	    timeout > config->tracking.ack_timeout_max_ms) {
		return config->tracking.ack_timeout_max_ms;
	}
	return timeout;
}

static void ack_work_handler(struct k_work *work)
{
	struct zbus_proxy_agent_config *config =
		CONTAINER_OF(work, struct zbus_proxy_agent_config, response.response_work);
	struct zbus_proxy_agent_msg response_msg;
	int ret;
	const char *response_type_str;
	uint8_t raw_buffer[ZBUS_PROXY_AGENT_RESPONSE_BUFFER_SIZE];
	size_t serialized_size;

	if (!config || !config->backend.backend_api || !config->backend.backend_api->backend_send) {
		LOG_ERR("Invalid config or backend API for sending response");
		return;
	}

	if (config->response.pending_response_type == ZBUS_PROXY_AGENT_MSG_TYPE_ACK) {
		ret = zbus_create_proxy_agent_ack_msg(&response_msg,
						      config->response.pending_response_msg_id);
		response_type_str = "ACK";
	} else if (config->response.pending_response_type == ZBUS_PROXY_AGENT_MSG_TYPE_NACK) {
		ret = zbus_create_proxy_agent_nack_msg(&response_msg,
						       config->response.pending_response_msg_id);
		response_type_str = "NACK";
	} else {
		LOG_ERR("Invalid response type: %d", config->response.pending_response_type);
		return;
	}
	if (ret < 0) {
		LOG_ERR("Failed to create %s message: %d", response_type_str, ret);
		return;
	}

	serialized_size = serialize_proxy_agent_msg(&response_msg, raw_buffer, sizeof(raw_buffer));
	if (serialized_size <= 0) {
		LOG_ERR("Failed to serialize %s message", response_type_str);
		return;
	}

	ret = config->backend.backend_api->backend_send(config->backend.backend_config, raw_buffer,
							serialized_size);
	if (ret < 0) {
		LOG_ERR("Failed to send %s message: %d", response_type_str, ret);
		return;
	}

	LOG_DBG("Sent %s for message ID %d", response_type_str,
		config->response.pending_response_msg_id);
}

static int schedule_ack(struct zbus_proxy_agent_config *config, uint32_t msg_id,
			uint8_t response_type)
{
	int ret;

	if (!config) {
		LOG_ERR("Invalid config for scheduling response");
		return -EINVAL;
	}

	if (response_type != ZBUS_PROXY_AGENT_MSG_TYPE_ACK &&
	    response_type != ZBUS_PROXY_AGENT_MSG_TYPE_NACK) {
		LOG_ERR("Invalid response type: %d", response_type);
		return -EINVAL;
	}

	config->response.pending_response_msg_id = msg_id;
	config->response.pending_response_type = response_type;

	ret = k_work_submit(&config->response.response_work);
	if (ret < 0) {
		LOG_ERR("Failed to schedule response work: %d", ret);
		return ret;
	}

	LOG_DBG("Scheduled %s for message ID %d",
		((response_type == ZBUS_PROXY_AGENT_MSG_TYPE_ACK) ? "ACK" : "NACK"), msg_id);
	return 0;
}

static int handle_recv_ack(struct zbus_proxy_agent_config *config, uint32_t msg_id)
{
	int ret;

	LOG_DBG("Received ACK for message ID %d", msg_id);
	ret = zbus_proxy_agent_stop_tracking(config, msg_id);
	if (ret < 0) {
		if (ret == -ENOENT) {
			LOG_DBG("Message ID %d was not found in tracking pool (already processed "
				"or never tracked)",
				msg_id);
		} else {
			LOG_ERR("Failed to process received ACK: %d", ret);
		}
	} else {
		LOG_DBG("Successfully processed ACK for message ID %d", msg_id);
	}
	return 0;
}

static int handle_recv_nack(struct zbus_proxy_agent_config *config, uint32_t msg_id)
{
	int ret;

	LOG_WRN("Received NACK for message ID %d, remote processing failed", msg_id);
	ret = zbus_proxy_agent_stop_tracking(config, msg_id);
	if (ret < 0 && ret != -ENOENT) {
		LOG_ERR("Failed to process received NACK: %d", ret);
	}
	return 0;
}

static int handle_recv_msg(struct zbus_proxy_agent_config *config, struct zbus_proxy_agent_msg *msg)
{
	int ret;
	int process_ret;

	LOG_DBG("Received data message ID %d for channel '%s'", msg->id, msg->channel_name);
	if (is_duplicate_message(config, msg->id)) {
		LOG_DBG("Duplicate message ID %d detected, sending ACK again", msg->id);
		/* Send ACK again for duplicate to avoid further retransmission */
		ret = schedule_ack(config, msg->id, ZBUS_PROXY_AGENT_MSG_TYPE_ACK);
		if (ret < 0) {
			LOG_ERR("Failed to schedule ACK for duplicate message %d: %d", msg->id,
				ret);
		}
		return 0;
	}
	add_to_duplicate_detection(config, msg->id);

	process_ret = k_msgq_put(&config->receive.receive_msgq, msg, K_NO_WAIT);
	if (process_ret < 0) {
		LOG_ERR("Failed to enqueue received message in proxy agent %s: %d",
			config->backend.name, process_ret);
		ret = schedule_ack(config, msg->id, ZBUS_PROXY_AGENT_MSG_TYPE_NACK);
		if (ret < 0) {
			LOG_ERR("Failed to schedule NACK for message %d: %d", msg->id, ret);
		}
		return process_ret;
	}

	return process_ret;
}

static int recv_callback(const uint8_t *data, size_t length, void *user_data)
{
	struct zbus_proxy_agent_config *config = (struct zbus_proxy_agent_config *)user_data;
	struct zbus_proxy_agent_msg msg;
	int ret;

	if (!config) {
		LOG_ERR("Invalid proxy agent configuration in receive callback");
		return -EINVAL;
	}

	ret = deserialize_proxy_agent_msg(data, length, &msg);
	if (ret < 0) {
		LOG_ERR("Failed to deserialize received raw message: %d", ret);
		return ret;
	}

	switch (msg.type) {
	case ZBUS_PROXY_AGENT_MSG_TYPE_ACK:
		/* ACK indicates successful receipt and processing, stop retransmissions */
		return handle_recv_ack(config, msg.id);
	case ZBUS_PROXY_AGENT_MSG_TYPE_NACK:
		/* NACK indicates processing failure, stop retransmissions and log error */
		return handle_recv_nack(config, msg.id);
	case ZBUS_PROXY_AGENT_MSG_TYPE_MSG:
		/* Data message, process and forward to receive queue */
		return handle_recv_msg(config, &msg);
	default:
		LOG_WRN("Unknown message type: %d", msg.type);
		return -EINVAL;
	}
}

static int zbus_proxy_agent_tracking_pool_init(struct zbus_proxy_agent_config *config)
{
	if (!config) {
		LOG_ERR("Invalid proxy agent configuration for message pool init");
		return -EINVAL;
	}

	if (!config->tracking.tracking_msg_pool) {
		LOG_ERR("No send message pool defined for proxy agent %s", config->backend.name);
		return -ENOSYS;
	}

	sys_slist_init(&config->tracking.tracking_msg_list);
	return 0;
}

static struct zbus_proxy_agent_tracked_msg *
zbus_proxy_agent_find_sent_msg_data(struct zbus_proxy_agent_config *config, uint32_t msg_id)
{
	struct net_buf *buf;
	uint32_t *msg_id_ptr;

	SYS_SLIST_FOR_EACH_CONTAINER(&config->tracking.tracking_msg_list, buf, node) {
		msg_id_ptr = net_buf_user_data(buf);

		if (*msg_id_ptr == msg_id) {
			return (struct zbus_proxy_agent_tracked_msg *)buf->data;
		}
	}
	return NULL;
}

static int zbus_proxy_agent_stop_tracking(struct zbus_proxy_agent_config *config, uint32_t msg_id)
{
	if (!config) {
		LOG_ERR("Invalid proxy agent configuration for removing sent message buffer");
		return -EINVAL;
	}

	if (!config->tracking.tracking_msg_pool) {
		LOG_ERR("No send message pool defined for proxy agent %s", config->backend.name);
		return -ENOSYS;
	}

	/* Protect list traversal and modification */
	unsigned int key = irq_lock();

	struct net_buf *prev_buf = NULL;
	struct net_buf *buf;
	uint32_t *msg_id_ptr;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&config->tracking.tracking_msg_list, buf, prev_buf,
					  node) {
		msg_id_ptr = net_buf_user_data(buf);

		if (*msg_id_ptr == msg_id) {
			struct zbus_proxy_agent_tracked_msg *data =
				(struct zbus_proxy_agent_tracked_msg *)buf->data;

			if (k_is_in_isr() || (k_work_busy_get(&data->work.work) & K_WORK_RUNNING)) {
				/* ISR context OR work is running, cannot cancel safely, just
				 * remove reference to config so that timeout handler knows it has
				 * been ACKed and will not attempt to resend or reschedule itself
				 */
				data->config = NULL;
				sys_slist_remove(&config->tracking.tracking_msg_list,
						 prev_buf ? &prev_buf->node : NULL, &buf->node);
				net_buf_unref(buf);
				irq_unlock(key);
				return 0;
			}
			/* Safe to cancel and cleanup */
			struct k_work_sync sync;

			k_work_cancel_delayable_sync(&data->work, &sync);
			sys_slist_remove(&config->tracking.tracking_msg_list,
					 prev_buf ? &prev_buf->node : NULL, &buf->node);
			net_buf_unref(buf);
			irq_unlock(key);
			return 0;
		}
	}

	irq_unlock(key);
	LOG_DBG("Sent message ID %d not found in list of tracked messages", msg_id);
	return -ENOENT;
}

static int schedule_timeout_work(struct zbus_proxy_agent_tracked_msg *data, uint8_t attempts)
{
	uint32_t timeout_ms;
	int ret;

	if (!data) {
		LOG_ERR("Invalid data for scheduling timeout work");
		return -EINVAL;
	}

	if (!data->config) {
		LOG_DBG("Data config is NULL, likely already ACKed, not scheduling timeout");
		return 0;
	}

	timeout_ms = calculate_backoff_timeout(data->config, attempts);

	LOG_DBG("Scheduling ACK timeout for message ID %d in %d ms (attempts: %d)", data->msg.id,
		timeout_ms, attempts);

	k_work_init_delayable(&data->work, zbus_proxy_agent_ack_timeout_handler);
	ret = k_work_schedule_for_queue(&k_sys_work_q, &data->work, K_MSEC(timeout_ms));
	if (ret < 0) {
		LOG_ERR("Failed to schedule timeout work for message ID %d: %d", data->msg.id, ret);
		return ret;
	}

	return 0;
}

static int send_retry_message(struct zbus_proxy_agent_tracked_msg *data)
{
	size_t serialized_size;
	int ret;

	if (!data || !data->config) {
		LOG_ERR("Invalid data or config for retry message send");
		return -EINVAL;
	}

	if (!data->config->backend.backend_api ||
	    !data->config->backend.backend_api->backend_send) {
		LOG_ERR("Backend API not available for retry");
		return -ENOSYS;
	}

	serialized_size = serialize_proxy_agent_msg(&data->msg, data->config->serialization_buffer,
						    data->config->serialization_buffer_size);
	if (serialized_size <= 0) {
		LOG_ERR("Failed to serialize retry message ID %d", data->msg.id);
		return -EINVAL;
	}

	ret = data->config->backend.backend_api->backend_send(data->config->backend.backend_config,
							      data->config->serialization_buffer,
							      serialized_size);
	if (ret < 0) {
		LOG_ERR("Failed to resend message ID %d: %d", data->msg.id, ret);
		return ret;
	}

	LOG_DBG("Resent message ID %d (attempt %d)", data->msg.id, data->transmit_attempts);
	return 0;
}

static int handle_message_retry(struct zbus_proxy_agent_tracked_msg *data, uint32_t expected_msg_id)
{
	int ret;

	LOG_WRN("Sent message ID %d timed out waiting for acknowledgment", expected_msg_id);

	data->transmit_attempts++;
	if ((data->transmit_attempts < data->config->tracking.ack_attempt_limit) ||
	    (data->config->tracking.ack_attempt_limit == -1)) {

		LOG_WRN("Retrying to send message ID %d (attempt %d)", expected_msg_id,
			data->transmit_attempts);

		ret = send_retry_message(data);
		if (ret < 0) {
			return ret;
		}

		ret = schedule_timeout_work(data, data->transmit_attempts);
		if (ret < 0) {
			LOG_ERR("Failed to schedule fresh timeout work for message ID %d: %d",
				expected_msg_id, ret);
		}
		return ret;
	}
	LOG_ERR("Max transmit attempts (%d) reached for message ID %d, giving up",
		data->config->tracking.ack_attempt_limit, expected_msg_id);

	ret = zbus_proxy_agent_stop_tracking(data->config, expected_msg_id);
	if (ret < 0 && ret != -ENOENT) {
		LOG_ERR("Failed to remove sent message ID %d from tracking pool: %d",
			expected_msg_id, ret);
	}
	return ret;
}

static void zbus_proxy_agent_ack_timeout_handler(struct k_work *work)
{
	uint32_t key;
	uint32_t expected_msg_id;
	struct zbus_proxy_agent_tracked_msg *data;
	struct zbus_proxy_agent_tracked_msg *current_data;
	struct k_work_delayable *dwork;

	dwork = k_work_delayable_from_work(work);
	data = CONTAINER_OF(dwork, struct zbus_proxy_agent_tracked_msg, work);

	if (!data) {
		LOG_ERR("Invalid sent message data in timeout handler");
		return;
	}

	expected_msg_id = data->msg.id;

	if (!data->config) {
		LOG_DBG("Timeout handler called for message ID %d but config is NULL, likely "
			"already ACKed",
			expected_msg_id);
		return;
	}

	key = irq_lock();
	current_data = zbus_proxy_agent_find_sent_msg_data(data->config, expected_msg_id);

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

	handle_message_retry(data, expected_msg_id);
}

static int zbus_proxy_agent_start_tracking(struct zbus_proxy_agent_config *config,
					   struct zbus_proxy_agent_msg *msg,
					   uint8_t transmit_attempts)
{
	int ret;
	uint32_t key;
	uint32_t *msg_id_ptr;
	struct net_buf *buf;
	struct zbus_proxy_agent_tracked_msg *data;

	if (!config || !msg) {
		LOG_ERR("Invalid parameters for adding sent message buffer");
		return -EINVAL;
	}

	if (!config->tracking.tracking_msg_pool) {
		LOG_ERR("No send message pool defined for proxy agent %s", config->backend.name);
		return -ENOSYS;
	}

	key = irq_lock();

	buf = net_buf_alloc(config->tracking.tracking_msg_pool, K_NO_WAIT);

	if (!buf) {
		irq_unlock(key);
		LOG_ERR("Sent message pool full, cannot track message ID %d for proxy agent %s",
			msg->id, config->backend.name);
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

	msg_id_ptr = net_buf_user_data(buf);
	*msg_id_ptr = msg->id;
	sys_slist_append(&config->tracking.tracking_msg_list, &buf->node);

	ret = schedule_timeout_work(data, transmit_attempts);

	irq_unlock(key);
	return ret;
}

static int zbus_proxy_agent_init(struct zbus_proxy_agent_config *config)
{
	int ret;

	if (!config || !config->backend.backend_config) {
		LOG_ERR("Invalid proxy agent configuration");
		return -EINVAL;
	}

	if (!config->backend.backend_api || !config->backend.backend_api->backend_init) {
		LOG_ERR("Raw backend API not available for proxy agent %s", config->backend.name);
		return -ENOSYS;
	}

	k_work_init(&config->response.response_work, ack_work_handler);

	/* Initialize receive message queue */
	k_msgq_init(&config->receive.receive_msgq, config->receive.receive_msgq_buffer,
		    sizeof(struct zbus_proxy_agent_msg),
		    config->receive.receive_msgq_buffer_size / sizeof(struct zbus_proxy_agent_msg));

	memset(config->duplicate_detection.detection_buffer, 0,
	       config->duplicate_detection.detection_buffer_size * sizeof(uint32_t));
	config->duplicate_detection.detection_head = 0;

	ret = config->backend.backend_api->backend_init(config->backend.backend_config);
	if (ret < 0) {
		LOG_ERR("Failed to initialize raw backend for proxy agent %s: %d",
			config->backend.name, ret);
		return ret;
	}

	LOG_DBG("Proxy agent %s with raw backend initialized successfully", config->backend.name);
	return 0;
}

static int zbus_proxy_agent_send(struct zbus_proxy_agent_config *config,
				 struct zbus_proxy_agent_msg *msg, uint8_t transmit_attempts)
{
	int ret, cleanup_ret;
	size_t serialized_size;

	if (!config || !msg) {
		LOG_ERR("Invalid parameters for sending message");
		return -EINVAL;
	}

	if (!config->backend.backend_api || !config->backend.backend_api->backend_send) {
		LOG_ERR("Raw backend API not available for proxy agent %s", config->backend.name);
		return -ENOSYS;
	}

	serialized_size = serialize_proxy_agent_msg(msg, config->serialization_buffer,
						    config->serialization_buffer_size);
	if (serialized_size <= 0) {
		LOG_ERR("Failed to serialize message for raw backend");
		return -EINVAL;
	}

	/* Add message to tracking pool before sending to avoid race condition with ACKs */
	if (config->tracking.tracking_msg_pool) {
		ret = zbus_proxy_agent_start_tracking(config, msg, transmit_attempts);
		if (ret < 0) {
			LOG_ERR("Failed to track sent message ID %d for proxy agent %s: %d",
				msg->id, config->backend.name, ret);
			return ret;
		}
	}

	ret = config->backend.backend_api->backend_send(
		config->backend.backend_config, config->serialization_buffer, serialized_size);
	if (ret < 0) {
		LOG_ERR("Failed to send raw message via proxy agent %s: %d", config->backend.name,
			ret);

		/* Remove from tracking pool since send failed */
		if (config->tracking.tracking_msg_pool) {
			cleanup_ret = zbus_proxy_agent_stop_tracking(config, msg->id);
			if (cleanup_ret < 0) {
				LOG_ERR("Failed to cleanup tracked message ID %d after send "
					"failure: %d",
					msg->id, cleanup_ret);
			}
		}
		return ret;
	}
	LOG_DBG("Raw message sent successfully via proxy agent %s", config->backend.name);
	return 0;
}

void process_subscriber_message(struct zbus_proxy_agent_config *config,
				const struct zbus_observer *subscriber)
{
	const struct zbus_channel *chan;
	struct zbus_proxy_agent_msg msg;
	struct net_buf *buf;
	void *message_data;
	int ret;

	if (!config || !subscriber) {
		LOG_ERR("Invalid parameters for handling subscriber message");
		return;
	}

	buf = k_fifo_get(subscriber->message_fifo, K_NO_WAIT);
	if (!buf) {
		LOG_ERR("Failed to get message from subscriber FIFO");
		return;
	}

	chan = *(struct zbus_channel **)net_buf_user_data(buf);
	if (!chan) {
		LOG_ERR("Invalid channel in subscriber message");
		net_buf_unref(buf);
		return;
	}

	if (chan->message_size > CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE) {
		LOG_ERR("Message size %zu exceeds maximum %d for channel %s", chan->message_size,
			CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE, chan->name);
		net_buf_unref(buf);
		return;
	}

	message_data = net_buf_remove_mem(buf, zbus_chan_msg_size(chan));

	ret = zbus_create_proxy_agent_msg(&msg, message_data, chan->message_size, chan->name,
					  strlen(chan->name));
	net_buf_unref(buf);

	if (ret < 0) {
		LOG_ERR("Failed to create proxy agent message for channel %s: %d", chan->name, ret);
		return;
	}

	ret = zbus_proxy_agent_send(config, &msg, 0);
	if (ret < 0) {
		LOG_ERR("Failed to send message via proxy agent %s: %d", config->backend.name, ret);
	}
}

int process_received_message(struct zbus_proxy_agent_config *config)
{
	int ret;
	const struct zbus_channel *chan;
	struct zbus_proxy_agent_msg msg = {0};

	if (!config) {
		LOG_ERR("Invalid proxy agent configuration for handling received message");
		return -EINVAL;
	}

	ret = k_msgq_get(&config->receive.receive_msgq, &msg, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to get message from receive MSGQ: %d", ret);
		return ret;
	}

	chan = zbus_chan_from_name(msg.channel_name);
	if (!chan) {
		LOG_ERR("Unknown channel '%s', cannot publish message", msg.channel_name);
		schedule_ack(config, msg.id, ZBUS_PROXY_AGENT_MSG_TYPE_NACK);
		return -ENOENT;
	}

	/* Verify this is a shadow channel belonging to this proxy agent */
	if (chan->validator != config->shadow_validator) {
		LOG_ERR("Channel '%s' is not a shadow channel for this proxy agent, cannot publish "
			"message",
			msg.channel_name);
		schedule_ack(config, msg.id, ZBUS_PROXY_AGENT_MSG_TYPE_NACK);
		return -EPERM;
	}

	ret = zbus_chan_pub(chan, msg.message_data, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to publish received message on channel %s: %d", msg.channel_name,
			ret);
		/* Neither ACK nor NACK is sent here, as we want to have the sender retry */
		return ret;
	}

	LOG_DBG("Successfully published received message on channel %s", msg.channel_name);
	schedule_ack(config, msg.id, ZBUS_PROXY_AGENT_MSG_TYPE_ACK);
	return 0;
}

int zbus_proxy_agent_thread(struct zbus_proxy_agent_config *config,
			    const struct zbus_observer *subscriber)
{
	int ret;

	if (!config) {
		LOG_ERR("Invalid proxy agent configuration for thread");
		return -EINVAL;
	}

	LOG_DBG("Starting thread for proxy agent %s", config->backend.name);

	if (!config->backend.backend_api || !config->backend.backend_api->backend_set_recv_cb) {
		LOG_ERR("Raw backend API not available for proxy agent %s", config->backend.name);
		return -ENOSYS;
	}

	ret = config->backend.backend_api->backend_set_recv_cb(config->backend.backend_config,
							       recv_callback, config);
	if (ret < 0) {
		LOG_ERR("Failed to set receive callback for proxy agent %s: %d",
			config->backend.name, ret);
		return ret;
	}

	ret = zbus_proxy_agent_tracking_pool_init(config);
	if (ret < 0) {
		LOG_ERR("Failed to initialize sent message pool for proxy agent %s: %d",
			config->backend.name, ret);
		return ret;
	}

	ret = zbus_proxy_agent_init(config);
	if (ret < 0) {
		LOG_ERR("Failed to initialize proxy agent %s: %d", config->backend.name, ret);
		return ret;
	}

	struct k_poll_event events[2] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY, subscriber->message_fifo,
						0),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&config->receive.receive_msgq, 0),
	};

	while (1) {
		ret = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
		if (ret < 0) {
			LOG_ERR("k_poll failed: %d", ret);
			continue;
		}
		if (events[0].state & K_POLL_STATE_FIFO_DATA_AVAILABLE) {
			process_subscriber_message(config, subscriber);
		}
		if (events[1].state & K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
			process_received_message(config);
		}

		events[0].state = K_POLL_STATE_NOT_READY;
		events[1].state = K_POLL_STATE_NOT_READY;
	}
	return 0;
}
