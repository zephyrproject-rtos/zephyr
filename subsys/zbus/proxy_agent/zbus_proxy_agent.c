/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>
#include <zephyr/sys/math_extras.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_proxy_agent, CONFIG_ZBUS_PROXY_AGENT_LOG_LEVEL);

/* Forward declaration */
static int zbus_proxy_agent_send(struct zbus_proxy_agent_config *config,
				 struct zbus_proxy_agent_msg *msg, uint8_t transmit_attempts);
static void zbus_proxy_agent_ack_timeout_handler(struct k_work *work);

static bool is_duplicate_message(struct zbus_proxy_agent_config *config, uint32_t msg_id)
{
	CHECKIF(config == NULL) {
		return false;
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
	CHECKIF(config == NULL) {
		return;
	}

	config->duplicate_detection.detection_buffer[config->duplicate_detection.detection_head] =
		msg_id;
	config->duplicate_detection.detection_head =
		(config->duplicate_detection.detection_head + 1) %
		config->duplicate_detection.detection_buffer_size;

	LOG_DBG("Added message ID %d to duplicate detection buffer", msg_id);
}

static uint32_t calculate_backoff_timeout(struct zbus_proxy_agent_config *config, uint8_t attempts)
{
	uint32_t multiplier;
	uint32_t timeout;
	uint32_t max_timeout = CONFIG_ZBUS_PROXY_AGENT_ACK_TIMEOUT_MAX_MS;

	multiplier = 1U << attempts;

	if (u32_mul_overflow(config->tracking.ack_timeout_initial_ms, multiplier, &timeout)) {
		if (max_timeout > 0) {
			return max_timeout;
		}
		return UINT32_MAX;
	}

	/* Cap to maximum timeout if set */
	if (max_timeout > 0 &&
	    timeout > max_timeout) {
		return max_timeout;
	}
	return timeout;
}

/**
 * @brief Work handler for sending ACK/NACK responses asynchronously.
 */
static void ack_work_handler(struct k_work *work)
{
	struct zbus_proxy_agent_config *config =
		CONTAINER_OF(work, struct zbus_proxy_agent_config, response.response_work);
	struct zbus_proxy_agent_msg response_msg;
	int ret;
	const char *response_type_str;
	uint8_t raw_buffer[ZBUS_PROXY_AGENT_RESPONSE_BUFFER_SIZE];
	size_t serialized_size;

	CHECKIF(config == NULL) {
		LOG_ERR("Invalid config for sending response");
		return;
	}
	CHECKIF(config->backend.backend_api == NULL ||
		config->backend.backend_api->backend_send == NULL) {
		LOG_ERR("Invalid backend API for sending response");
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

	CHECKIF(config == NULL) {
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
	return 0;
}

static bool zbus_proxy_agent_mark_ack_nack_received(struct zbus_proxy_agent_config *config,
						    uint32_t msg_id)
{
	struct net_buf *buf;
	uint32_t *msg_id_ptr;
	struct zbus_proxy_agent_tracked_msg *data;
	unsigned int key;
	bool found = false;

	/* Find and atomically mark the message as ACKed/NACKed */
	key = irq_lock();
	SYS_SLIST_FOR_EACH_CONTAINER(&config->tracking.tracking_msg_list, buf, node) {
		msg_id_ptr = net_buf_user_data(buf);
		if (*msg_id_ptr == msg_id) {
			data = (struct zbus_proxy_agent_tracked_msg *)buf->data;
			atomic_set(&data->ack_nack_received, 1);
			found = true;
			break;
		}
	}
	irq_unlock(key);

	return found;
}

static int handle_recv_ack(struct zbus_proxy_agent_config *config, uint32_t msg_id)
{
	int ret;

	LOG_DBG("Received ACK for message ID %d", msg_id);

	if (!zbus_proxy_agent_mark_ack_nack_received(config, msg_id)) {
		LOG_DBG("ACK for message ID %d not found in tracking list (already processed)",
			msg_id);
		return -ENOENT;
	}
	ret = k_msgq_put(&config->tracking.cleanup_msgq, &msg_id, K_NO_WAIT);
	if (ret < 0) {
		LOG_WRN("Cleanup queue full for message ID %d, dropping", msg_id);
	}
	return 0;
}

static int handle_recv_nack(struct zbus_proxy_agent_config *config, uint32_t msg_id)
{
	int ret;

	LOG_WRN("Received NACK for message ID %d, remote processing failed", msg_id);

	if (!zbus_proxy_agent_mark_ack_nack_received(config, msg_id)) {
		LOG_DBG("NACK for message ID %d not found in tracking list (already processed)",
			msg_id);
		return -ENOENT;
	}
	ret = k_msgq_put(&config->tracking.cleanup_msgq, &msg_id, K_NO_WAIT);
	if (ret < 0) {
		LOG_WRN("Cleanup queue full for message ID %d, dropping", msg_id);
	}
	return 0;
}

static int handle_recv_msg(struct zbus_proxy_agent_config *config, struct zbus_proxy_agent_msg *msg)
{
	int ret;

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

	ret = k_msgq_put(&config->receive.receive_msgq, msg, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to enqueue received message in proxy agent %s: %d",
			config->backend.name, ret);
	}
	return ret;
}

static int recv_callback(const uint8_t *data, size_t length, void *user_data)
{
	struct zbus_proxy_agent_config *config = (struct zbus_proxy_agent_config *)user_data;
	struct zbus_proxy_agent_msg msg;
	int ret;

	CHECKIF(config == NULL) {
		LOG_ERR("Invalid proxy agent configuration in receive callback");
		return -EINVAL;
	}

	ret = deserialize_proxy_agent_msg(data, length, &msg);
	if (ret < 0) {
		LOG_ERR("Failed to deserialize received message: %d", ret);
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
	CHECKIF(config == NULL) {
		LOG_ERR("Invalid proxy agent configuration for message pool init");
		return -EINVAL;
	}

	CHECKIF(config->tracking.tracking_msg_pool == NULL) {
		LOG_ERR("No send message pool defined for proxy agent %s", config->backend.name);
		return -ENOSYS;
	}

	sys_slist_init(&config->tracking.tracking_msg_list);
	return 0;
}

static int zbus_proxy_agent_stop_tracking(struct zbus_proxy_agent_config *config, uint32_t msg_id)
{
	struct net_buf *buf, *tmp;
	sys_snode_t *prev = NULL;
	uint32_t *msg_id_ptr;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&config->tracking.tracking_msg_list, buf, tmp, node) {
		msg_id_ptr = net_buf_user_data(buf);

		if (*msg_id_ptr == msg_id) {
			struct zbus_proxy_agent_tracked_msg *data =
				(struct zbus_proxy_agent_tracked_msg *)buf->data;
			struct k_work_sync sync;

			k_work_cancel_delayable_sync(&data->work, &sync);
			sys_slist_remove(&config->tracking.tracking_msg_list, prev, &buf->node);
			net_buf_unref(buf);
			return 0;
		}
		prev = &buf->node;
	}

	LOG_DBG("Message ID %d not found in tracking list", msg_id);
	return -ENOENT;
}

static int schedule_timeout_work(struct zbus_proxy_agent_tracked_msg *data, uint8_t attempts)
{
	uint32_t timeout_ms;
	int ret;

	CHECKIF(data == NULL) {
		LOG_ERR("Invalid data for scheduling timeout work");
		return -EINVAL;
	}
	CHECKIF(data->config == NULL) {
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

	CHECKIF(data == NULL) {
		LOG_ERR("Invalid data for retry message send");
		return -EINVAL;
	}
	CHECKIF(data->config == NULL) {
		LOG_ERR("Invalid config for retry message send");
		return -EINVAL;
	}
	CHECKIF(data->config->backend.backend_api == NULL ||
		data->config->backend.backend_api->backend_send == NULL) {
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

	CHECKIF(data == NULL) {
		LOG_ERR("Invalid data for handling message retry");
		return -EINVAL;
	}

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

	ret = k_msgq_put(&data->config->tracking.cleanup_msgq, &expected_msg_id, K_NO_WAIT);
	if (ret < 0) {
		LOG_WRN("Cleanup queue full for message ID %d, dropping", expected_msg_id);
	}
	return 0;
}

static void zbus_proxy_agent_ack_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork;
	struct zbus_proxy_agent_tracked_msg *data;
	uint32_t expected_msg_id;

	dwork = k_work_delayable_from_work(work);
	data = CONTAINER_OF(dwork, struct zbus_proxy_agent_tracked_msg, work);

	expected_msg_id = data->msg.id;

	if (atomic_get(&data->ack_nack_received)) {
		/* ACK/NACK already received, but cleanup not done yet */
		LOG_DBG("ACK/NACK received for message ID %d, skipping retransmission",
			expected_msg_id);
		return;
	}
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

	CHECKIF(config == NULL || msg == NULL) {
		LOG_ERR("Invalid parameters for adding sent message buffer");
		return -EINVAL;
	}
	CHECKIF(config->tracking.tracking_msg_pool == NULL) {
		LOG_ERR("No send message pool defined for proxy agent %s", config->backend.name);
		return -ENOSYS;
	}

	key = irq_lock();

	buf = net_buf_alloc(config->tracking.tracking_msg_pool, K_NO_WAIT);
	CHECKIF(buf == NULL) {
		irq_unlock(key);
		LOG_ERR("Sent message pool full, cannot track message ID %d for proxy agent %s",
			msg->id, config->backend.name);
		return -ENOMEM;
	}

	data = net_buf_add(buf, sizeof(struct zbus_proxy_agent_tracked_msg));
	data->config = config;
	data->transmit_attempts = transmit_attempts;
	atomic_set(&data->ack_nack_received, 0);
	memcpy(&data->msg, msg, sizeof(*msg));

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

	CHECKIF(config == NULL || config->backend.backend_config == NULL) {
		LOG_ERR("Invalid proxy agent configuration");
		return -EINVAL;
	}
	CHECKIF(config->backend.backend_api == NULL ||
		config->backend.backend_api->backend_init == NULL) {
		LOG_ERR("Backend API not available for proxy agent %s", config->backend.name);
		return -ENOSYS;
	}
	CHECKIF(config->tracking.cleanup_msgq_buffer == NULL) {
		LOG_ERR("Cleanup queue buffer not configured for proxy agent %s",
			config->backend.name);
		return -EINVAL;
	}

	k_work_init(&config->response.response_work, ack_work_handler);

	k_msgq_init(&config->receive.receive_msgq, config->receive.receive_msgq_buffer,
		    sizeof(struct zbus_proxy_agent_msg),
		    config->receive.receive_msgq_buffer_size / sizeof(struct zbus_proxy_agent_msg));

	memset(config->duplicate_detection.detection_buffer, 0,
	       config->duplicate_detection.detection_buffer_size * sizeof(uint32_t));
	config->duplicate_detection.detection_head = 0;

	k_msgq_init(&config->tracking.cleanup_msgq, config->tracking.cleanup_msgq_buffer,
		    sizeof(uint32_t), CONFIG_ZBUS_PROXY_AGENT_CLEANUP_QUEUE_SIZE);

	ret = config->backend.backend_api->backend_init(config->backend.backend_config);
	if (ret < 0) {
		LOG_ERR("Failed to initialize backend for proxy agent %s: %d", config->backend.name,
			ret);
		return ret;
	}

	LOG_DBG("Proxy agent \'%s\' initialized successfully", config->backend.name);
	return 0;
}

static int zbus_proxy_agent_send(struct zbus_proxy_agent_config *config,
				 struct zbus_proxy_agent_msg *msg, uint8_t transmit_attempts)
{
	int ret, cleanup_ret;
	size_t serialized_size;

	CHECKIF(config == NULL || msg == NULL) {
		LOG_ERR("Invalid parameters for sending message");
		return -EINVAL;
	}

	CHECKIF(config->backend.backend_api == NULL ||
		config->backend.backend_api->backend_send == NULL) {
		LOG_ERR("Backend API not available for proxy agent %s", config->backend.name);
		return -ENOSYS;
	}

	serialized_size = serialize_proxy_agent_msg(msg, config->serialization_buffer,
						    config->serialization_buffer_size);
	if (serialized_size <= 0) {
		LOG_ERR("Failed to serialize message for backend");
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
		LOG_ERR("Failed to send message via proxy agent %s: %d", config->backend.name, ret);

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
	LOG_DBG("Message sent successfully via proxy agent %s", config->backend.name);
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

	CHECKIF(config == NULL) {
		LOG_ERR("Invalid proxy agent configuration for handling subscriber message");
		return;
	}
	CHECKIF(subscriber == NULL) {
		LOG_ERR("Invalid subscriber for handling subscriber message");
		return;
	}
	buf = k_fifo_get(subscriber->message_fifo, K_NO_WAIT);
	CHECKIF(buf == NULL) {
		LOG_ERR("Failed to get message from subscriber FIFO");
		return;
	}
	chan = *(struct zbus_channel **)net_buf_user_data(buf);
	CHECKIF(chan == NULL) {
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
	if (ret < 0) {
		LOG_ERR("Failed to create proxy agent message for channel %s: %d", chan->name, ret);
		net_buf_unref(buf);
		return;
	}
	net_buf_unref(buf);
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

	CHECKIF(config == NULL) {
		LOG_ERR("Invalid proxy agent configuration for handling received message");
		return -EINVAL;
	}

	ret = k_msgq_get(&config->receive.receive_msgq, &msg, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to get message from receive MSGQ: %d", ret);
		return ret;
	}

	chan = zbus_chan_from_name(msg.channel_name);
	if (chan == NULL) {
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

static void process_cleanup_msgq(struct zbus_proxy_agent_config *config)
{
	uint32_t msg_id;
	int ret;

	/* Process all pending tracking cleanup */
	while (k_msgq_get(&config->tracking.cleanup_msgq, &msg_id, K_NO_WAIT) == 0) {
		ret = zbus_proxy_agent_stop_tracking(config, msg_id);
		if (ret < 0 && ret != -ENOENT) {
			LOG_ERR("Failed to stop tracking message ID %d: %d", msg_id, ret);
		}
	}
}

int zbus_proxy_agent_thread(struct zbus_proxy_agent_config *config,
			    const struct zbus_observer *subscriber)
{
	int ret;

	_ZBUS_ASSERT(config != NULL, "Invalid proxy agent configuration for thread");
	_ZBUS_ASSERT(subscriber != NULL, "Invalid subscriber for proxy agent thread");
	_ZBUS_ASSERT(config->backend.backend_api != NULL,
		     "Backend API not available for proxy agent %s", config->backend.name);
	_ZBUS_ASSERT(config->backend.backend_api->backend_set_recv_cb != NULL,
		     "Backend set receive callback API not available for proxy agent %s",
		     config->backend.name);

	LOG_DBG("Starting thread for proxy agent \'%s\'", config->backend.name);

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

	struct k_poll_event events[3] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						subscriber->message_fifo, 0),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&config->receive.receive_msgq, 0),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&config->tracking.cleanup_msgq, 0),
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
		if (events[2].state & K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
			process_cleanup_msgq(config);
		}

		events[0].state = K_POLL_STATE_NOT_READY;
		events[1].state = K_POLL_STATE_NOT_READY;
		events[2].state = K_POLL_STATE_NOT_READY;
	}
	return 0;
}
