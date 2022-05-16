/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt.c
 *
 * @brief MQTT Client API Implementation.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mqtt, CONFIG_MQTT_LOG_LEVEL);

#include <zephyr/net/mqtt.h>

#include "mqtt_transport.h"
#include "mqtt_internal.h"
#include "mqtt_os.h"

static void client_reset(struct mqtt_client *client)
{
	MQTT_STATE_INIT(client);

	client->internal.last_activity = 0U;
	client->internal.rx_buf_datalen = 0U;
	client->internal.remaining_payload = 0U;
}

/** @brief Initialize tx buffer. */
static void tx_buf_init(struct mqtt_client *client, struct buf_ctx *buf)
{
	memset(client->tx_buf, 0, client->tx_buf_size);
	buf->cur = client->tx_buf;
	buf->end = client->tx_buf + client->tx_buf_size;
}

void event_notify(struct mqtt_client *client, const struct mqtt_evt *evt)
{
	if (client->evt_cb != NULL) {
		mqtt_mutex_unlock(client);

		client->evt_cb(client, evt);

		mqtt_mutex_lock(client);
	}
}

static void client_disconnect(struct mqtt_client *client, int result,
			      bool notify)
{
	int err_code;

	err_code = mqtt_transport_disconnect(client);
	if (err_code < 0) {
		NET_ERR("Failed to disconnect transport!");
	}

	/* Reset internal state. */
	client_reset(client);

	if (notify) {
		struct mqtt_evt evt = {
			.type = MQTT_EVT_DISCONNECT,
			.result = result,
		};

		/* Notify application. */
		event_notify(client, &evt);
	}
}

static int client_connect(struct mqtt_client *client)
{
	int err_code;
	struct buf_ctx packet;

	err_code = mqtt_transport_connect(client);
	if (err_code < 0) {
		return err_code;
	}

	tx_buf_init(client, &packet);
	MQTT_SET_STATE(client, MQTT_STATE_TCP_CONNECTED);

	err_code = connect_request_encode(client, &packet);
	if (err_code < 0) {
		goto error;
	}

	/* Send MQTT identification message to broker. */
	err_code = mqtt_transport_write(client, packet.cur,
					packet.end - packet.cur);
	if (err_code < 0) {
		goto error;
	}

	client->internal.last_activity = mqtt_sys_tick_in_ms_get();

	/* Reset the unanswered ping count for a new connection */
	client->unacked_ping = 0;

	NET_INFO("Connect completed");

	return 0;

error:
	client_disconnect(client, err_code, false);
	return err_code;
}

static int client_read(struct mqtt_client *client)
{
	int err_code;

	if (client->internal.remaining_payload > 0) {
		return -EBUSY;
	}

	err_code = mqtt_handle_rx(client);
	if (err_code < 0) {
		client_disconnect(client, err_code, true);
	}

	return err_code;
}

static int client_write(struct mqtt_client *client, const uint8_t *data,
			uint32_t datalen)
{
	int err_code;

	NET_DBG("[%p]: Transport writing %d bytes.", client, datalen);

	err_code = mqtt_transport_write(client, data, datalen);
	if (err_code < 0) {
		NET_ERR("Transport write failed, err_code = %d, "
			 "closing connection", err_code);
		client_disconnect(client, err_code, true);
		return err_code;
	}

	NET_DBG("[%p]: Transport write complete.", client);
	client->internal.last_activity = mqtt_sys_tick_in_ms_get();

	return 0;
}

static int client_write_msg(struct mqtt_client *client,
			    const struct msghdr *message)
{
	int err_code;

	NET_DBG("[%p]: Transport writing message.", client);

	err_code = mqtt_transport_write_msg(client, message);
	if (err_code < 0) {
		NET_ERR("Transport write failed, err_code = %d, "
			 "closing connection", err_code);
		client_disconnect(client, err_code, true);
		return err_code;
	}

	NET_DBG("[%p]: Transport write complete.", client);
	client->internal.last_activity = mqtt_sys_tick_in_ms_get();

	return 0;
}

void mqtt_client_init(struct mqtt_client *client)
{
	NULL_PARAM_CHECK_VOID(client);

	memset(client, 0, sizeof(*client));

	MQTT_STATE_INIT(client);
	mqtt_mutex_init(client);

	client->protocol_version = MQTT_VERSION_3_1_1;
	client->clean_session = MQTT_CLEAN_SESSION;
	client->keepalive = MQTT_KEEPALIVE;
}

#if defined(CONFIG_SOCKS)
int mqtt_client_set_proxy(struct mqtt_client *client,
			  struct sockaddr *proxy_addr,
			  socklen_t addrlen)
{
	if (IS_ENABLED(CONFIG_SOCKS)) {
		if (!client || !proxy_addr) {
			return -EINVAL;
		}

		client->transport.proxy.addrlen = addrlen;
		memcpy(&client->transport.proxy.addr, proxy_addr, addrlen);

		return 0;
	}

	return -ENOTSUP;
}
#endif

int mqtt_connect(struct mqtt_client *client)
{
	int err_code;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(client->client_id.utf8);

	mqtt_mutex_lock(client);

	if ((client->tx_buf == NULL) || (client->rx_buf == NULL)) {
		err_code = -ENOMEM;
		goto error;
	}

	err_code = client_connect(client);

error:
	if (err_code < 0) {
		client_reset(client);
	}

	mqtt_mutex_unlock(client);

	return err_code;
}

static int verify_tx_state(const struct mqtt_client *client)
{
	if (!MQTT_HAS_STATE(client, MQTT_STATE_CONNECTED)) {
		return -ENOTCONN;
	}

	return 0;
}

int mqtt_publish(struct mqtt_client *client,
		 const struct mqtt_publish_param *param)
{
	int err_code;
	struct buf_ctx packet;
	struct iovec io_vector[2];
	struct msghdr msg;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	NET_DBG("[CID %p]:[State 0x%02x]: >> Topic size 0x%08x, "
		 "Data size 0x%08x", client, client->internal.state,
		 param->message.topic.topic.size,
		 param->message.payload.len);

	mqtt_mutex_lock(client);

	tx_buf_init(client, &packet);

	err_code = verify_tx_state(client);
	if (err_code < 0) {
		goto error;
	}

	err_code = publish_encode(param, &packet);
	if (err_code < 0) {
		goto error;
	}

	io_vector[0].iov_base = packet.cur;
	io_vector[0].iov_len = packet.end - packet.cur;
	io_vector[1].iov_base = param->message.payload.data;
	io_vector[1].iov_len = param->message.payload.len;

	memset(&msg, 0, sizeof(msg));

	msg.msg_iov = io_vector;
	msg.msg_iovlen = ARRAY_SIZE(io_vector);

	err_code = client_write_msg(client, &msg);

error:
	NET_DBG("[CID %p]:[State 0x%02x]: << result 0x%08x",
			 client, client->internal.state, err_code);

	mqtt_mutex_unlock(client);

	return err_code;
}

int mqtt_publish_qos1_ack(struct mqtt_client *client,
			  const struct mqtt_puback_param *param)
{
	int err_code;
	struct buf_ctx packet;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	NET_DBG("[CID %p]:[State 0x%02x]: >> Message id 0x%04x",
		 client, client->internal.state, param->message_id);

	mqtt_mutex_lock(client);

	tx_buf_init(client, &packet);

	err_code = verify_tx_state(client);
	if (err_code < 0) {
		goto error;
	}

	err_code = publish_ack_encode(param, &packet);
	if (err_code < 0) {
		goto error;
	}

	err_code = client_write(client, packet.cur, packet.end - packet.cur);

error:
	NET_DBG("[CID %p]:[State 0x%02x]: << result 0x%08x",
		 client, client->internal.state, err_code);

	mqtt_mutex_unlock(client);

	return err_code;
}

int mqtt_publish_qos2_receive(struct mqtt_client *client,
			      const struct mqtt_pubrec_param *param)
{
	int err_code;
	struct buf_ctx packet;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	NET_DBG("[CID %p]:[State 0x%02x]: >> Message id 0x%04x",
		 client, client->internal.state, param->message_id);

	mqtt_mutex_lock(client);

	tx_buf_init(client, &packet);

	err_code = verify_tx_state(client);
	if (err_code < 0) {
		goto error;
	}

	err_code = publish_receive_encode(param, &packet);
	if (err_code < 0) {
		goto error;
	}

	err_code = client_write(client, packet.cur, packet.end - packet.cur);

error:
	NET_DBG("[CID %p]:[State 0x%02x]: << result 0x%08x",
		 client, client->internal.state, err_code);

	mqtt_mutex_unlock(client);

	return err_code;
}

int mqtt_publish_qos2_release(struct mqtt_client *client,
			      const struct mqtt_pubrel_param *param)
{
	int err_code;
	struct buf_ctx packet;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	NET_DBG("[CID %p]:[State 0x%02x]: >> Message id 0x%04x",
		 client, client->internal.state, param->message_id);

	mqtt_mutex_lock(client);

	tx_buf_init(client, &packet);

	err_code = verify_tx_state(client);
	if (err_code < 0) {
		goto error;
	}

	err_code = publish_release_encode(param, &packet);
	if (err_code < 0) {
		goto error;
	}

	err_code = client_write(client, packet.cur, packet.end - packet.cur);

error:
	NET_DBG("[CID %p]:[State 0x%02x]: << result 0x%08x",
		 client, client->internal.state, err_code);

	mqtt_mutex_unlock(client);

	return err_code;
}

int mqtt_publish_qos2_complete(struct mqtt_client *client,
			       const struct mqtt_pubcomp_param *param)
{
	int err_code;
	struct buf_ctx packet;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	NET_DBG("[CID %p]:[State 0x%02x]: >> Message id 0x%04x",
		 client, client->internal.state, param->message_id);

	mqtt_mutex_lock(client);

	tx_buf_init(client, &packet);

	err_code = verify_tx_state(client);
	if (err_code < 0) {
		goto error;
	}

	err_code = publish_complete_encode(param, &packet);
	if (err_code < 0) {
		goto error;
	}

	err_code = client_write(client, packet.cur, packet.end - packet.cur);
	if (err_code < 0) {
		goto error;
	}

error:
	NET_DBG("[CID %p]:[State 0x%02x]: << result 0x%08x",
		 client, client->internal.state, err_code);

	mqtt_mutex_unlock(client);

	return err_code;
}

int mqtt_disconnect(struct mqtt_client *client)
{
	int err_code;
	struct buf_ctx packet;

	NULL_PARAM_CHECK(client);

	mqtt_mutex_lock(client);

	tx_buf_init(client, &packet);

	err_code = verify_tx_state(client);
	if (err_code < 0) {
		goto error;
	}

	err_code = disconnect_encode(&packet);
	if (err_code < 0) {
		goto error;
	}

	err_code = client_write(client, packet.cur, packet.end - packet.cur);
	if (err_code < 0) {
		goto error;
	}

	client_disconnect(client, 0, true);

error:
	mqtt_mutex_unlock(client);

	return err_code;
}

int mqtt_subscribe(struct mqtt_client *client,
		   const struct mqtt_subscription_list *param)
{
	int err_code;
	struct buf_ctx packet;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	NET_DBG("[CID %p]:[State 0x%02x]: >> message id 0x%04x "
		 "topic count 0x%04x", client, client->internal.state,
		 param->message_id, param->list_count);

	mqtt_mutex_lock(client);

	tx_buf_init(client, &packet);

	err_code = verify_tx_state(client);
	if (err_code < 0) {
		goto error;
	}

	err_code = subscribe_encode(param, &packet);
	if (err_code < 0) {
		goto error;
	}

	err_code = client_write(client, packet.cur, packet.end - packet.cur);

error:
	NET_DBG("[CID %p]:[State 0x%02x]: << result 0x%08x",
		 client, client->internal.state, err_code);

	mqtt_mutex_unlock(client);

	return err_code;
}

int mqtt_unsubscribe(struct mqtt_client *client,
		     const struct mqtt_subscription_list *param)
{
	int err_code;
	struct buf_ctx packet;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	mqtt_mutex_lock(client);

	tx_buf_init(client, &packet);

	err_code = verify_tx_state(client);
	if (err_code < 0) {
		goto error;
	}

	err_code = unsubscribe_encode(param, &packet);
	if (err_code < 0) {
		goto error;
	}

	err_code = client_write(client, packet.cur, packet.end - packet.cur);

error:
	mqtt_mutex_unlock(client);

	return err_code;
}

int mqtt_ping(struct mqtt_client *client)
{
	int err_code;
	struct buf_ctx packet;

	NULL_PARAM_CHECK(client);

	mqtt_mutex_lock(client);

	tx_buf_init(client, &packet);

	err_code = verify_tx_state(client);
	if (err_code < 0) {
		goto error;
	}

	err_code = ping_request_encode(&packet);
	if (err_code < 0) {
		goto error;
	}

	err_code = client_write(client, packet.cur, packet.end - packet.cur);

	if (client->unacked_ping >= INT8_MAX) {
		NET_WARN("PING count overflow!");
	} else {
		client->unacked_ping++;
	}

error:
	mqtt_mutex_unlock(client);

	return err_code;
}

int mqtt_abort(struct mqtt_client *client)
{
	NULL_PARAM_CHECK(client);

	mqtt_mutex_lock(client);

	if (client->internal.state != MQTT_STATE_IDLE) {
		client_disconnect(client, -ECONNABORTED, true);
	}

	mqtt_mutex_unlock(client);

	return 0;
}

int mqtt_live(struct mqtt_client *client)
{
	int err_code = 0;
	uint32_t elapsed_time;
	bool ping_sent = false;

	NULL_PARAM_CHECK(client);

	mqtt_mutex_lock(client);

	elapsed_time = mqtt_elapsed_time_in_ms_get(
				client->internal.last_activity);
	if ((client->keepalive > 0) &&
	    (elapsed_time >= (client->keepalive * 1000))) {
		err_code = mqtt_ping(client);
		ping_sent = true;
	}

	mqtt_mutex_unlock(client);

	if (ping_sent) {
		return err_code;
	} else {
		return -EAGAIN;
	}
}

int mqtt_keepalive_time_left(const struct mqtt_client *client)
{
	uint32_t elapsed_time = mqtt_elapsed_time_in_ms_get(
					client->internal.last_activity);
	uint32_t keepalive_ms = 1000U * client->keepalive;

	if (client->keepalive == 0) {
		/* Keep alive not enabled. */
		return -1;
	}

	if (keepalive_ms <= elapsed_time) {
		return 0;
	}

	return keepalive_ms - elapsed_time;
}

int mqtt_input(struct mqtt_client *client)
{
	int err_code = 0;

	NULL_PARAM_CHECK(client);

	mqtt_mutex_lock(client);

	NET_DBG("state:0x%08x", client->internal.state);

	if (MQTT_HAS_STATE(client, MQTT_STATE_TCP_CONNECTED)) {
		err_code = client_read(client);
	} else {
		err_code = -ENOTCONN;
	}

	mqtt_mutex_unlock(client);

	return err_code;
}

static int read_publish_payload(struct mqtt_client *client, void *buffer,
				size_t length, bool shall_block)
{
	int ret;

	NULL_PARAM_CHECK(client);

	mqtt_mutex_lock(client);

	if (client->internal.remaining_payload == 0U) {
		ret = 0;
		goto exit;
	}

	if (client->internal.remaining_payload < length) {
		length = client->internal.remaining_payload;
	}

	ret = mqtt_transport_read(client, buffer, length, shall_block);
	if (!shall_block && ret == -EAGAIN) {
		goto exit;
	}

	if (ret <= 0) {
		if (ret == 0) {
			ret = -ENOTCONN;
		}

		client_disconnect(client, ret, true);
		goto exit;
	}

	client->internal.remaining_payload -= ret;

exit:
	mqtt_mutex_unlock(client);

	return ret;
}

int mqtt_read_publish_payload(struct mqtt_client *client, void *buffer,
			      size_t length)
{
	return read_publish_payload(client, buffer, length, false);
}

int mqtt_read_publish_payload_blocking(struct mqtt_client *client, void *buffer,
				       size_t length)
{
	return read_publish_payload(client, buffer, length, true);
}

int mqtt_readall_publish_payload(struct mqtt_client *client, uint8_t *buffer,
				 size_t length)
{
	uint8_t *end = buffer + length;

	while (buffer < end) {
		int ret = mqtt_read_publish_payload_blocking(client, buffer,
							     end - buffer);

		if (ret < 0) {
			return ret;
		} else if (ret == 0) {
			return -EIO;
		}

		buffer += ret;
	}

	return 0;
}
