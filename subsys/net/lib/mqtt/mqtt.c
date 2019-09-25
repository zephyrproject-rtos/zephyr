/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt.c
 *
 * @brief MQTT Client API Implementation.
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_mqtt, CONFIG_MQTT_LOG_LEVEL);

#include <net/mqtt.h>

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

/**@brief Notifies disconnection event to the application.
 *
 * @param[in] client Identifies the client for which the procedure is requested.
 * @param[in] result Reason for disconnection.
 */
static void disconnect_event_notify(struct mqtt_client *client, int result)
{
	struct mqtt_evt evt;

	/* Determine appropriate event to generate. */
	if (MQTT_HAS_STATE(client, MQTT_STATE_CONNECTED) ||
	    MQTT_HAS_STATE(client, MQTT_STATE_DISCONNECTING)) {
		evt.type = MQTT_EVT_DISCONNECT;
		evt.result = result;
	} else {
		evt.type = MQTT_EVT_CONNACK;
		evt.result = -ECONNREFUSED;
	}

	/* Notify application. */
	event_notify(client, &evt);

	/* Reset internal state. */
	client_reset(client);
}

void event_notify(struct mqtt_client *client, const struct mqtt_evt *evt)
{
	if (client->evt_cb != NULL) {
		mqtt_mutex_unlock(client);

		client->evt_cb(client, evt);

		mqtt_mutex_lock(client);
	}
}

static void client_disconnect(struct mqtt_client *client, int result)
{
	int err_code;

	err_code = mqtt_transport_disconnect(client);
	if (err_code < 0) {
		MQTT_ERR("Failed to disconnect transport!");
	}

	disconnect_event_notify(client, result);
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

	MQTT_TRC("Connect completed");

	return 0;

error:
	client_disconnect(client, err_code);
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
		client_disconnect(client, err_code);
	}

	return err_code;
}

static int client_write(struct mqtt_client *client, const u8_t *data,
			u32_t datalen)
{
	int err_code;

	MQTT_TRC("[%p]: Transport writing %d bytes.", client, datalen);

	err_code = mqtt_transport_write(client, data, datalen);
	if (err_code < 0) {
		MQTT_TRC("TCP write failed, errno = %d, "
			 "closing connection", errno);
		client_disconnect(client, err_code);
		return err_code;
	}

	MQTT_TRC("[%p]: Transport write complete.", client);
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
	client->clean_session = 1U;
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

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	MQTT_TRC("[CID %p]:[State 0x%02x]: >> Topic size 0x%08x, "
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

	err_code = client_write(client, packet.cur, packet.end - packet.cur);
	if (err_code < 0) {
		goto error;
	}

	err_code = client_write(client, param->message.payload.data,
				param->message.payload.len);

error:
	MQTT_TRC("[CID %p]:[State 0x%02x]: << result 0x%08x",
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

	MQTT_TRC("[CID %p]:[State 0x%02x]: >> Message id 0x%04x",
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
	MQTT_TRC("[CID %p]:[State 0x%02x]: << result 0x%08x",
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

	MQTT_TRC("[CID %p]:[State 0x%02x]: >> Message id 0x%04x",
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
	MQTT_TRC("[CID %p]:[State 0x%02x]: << result 0x%08x",
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

	MQTT_TRC("[CID %p]:[State 0x%02x]: >> Message id 0x%04x",
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
	MQTT_TRC("[CID %p]:[State 0x%02x]: << result 0x%08x",
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

	MQTT_TRC("[CID %p]:[State 0x%02x]: >> Message id 0x%04x",
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
	MQTT_TRC("[CID %p]:[State 0x%02x]: << result 0x%08x",
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

	MQTT_SET_STATE_EXCLUSIVE(client, MQTT_STATE_DISCONNECTING);

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

	MQTT_TRC("[CID %p]:[State 0x%02x]: >> message id 0x%04x "
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
	MQTT_TRC("[CID %p]:[State 0x%02x]: << result 0x%08x",
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

error:
	mqtt_mutex_unlock(client);

	return err_code;
}

int mqtt_abort(struct mqtt_client *client)
{
	mqtt_mutex_lock(client);

	NULL_PARAM_CHECK(client);

	if (client->internal.state != MQTT_STATE_IDLE) {
		client_disconnect(client, -ECONNABORTED);
	}

	mqtt_mutex_unlock(client);

	return 0;
}

int mqtt_live(struct mqtt_client *client)
{
	u32_t elapsed_time;

	NULL_PARAM_CHECK(client);

	mqtt_mutex_lock(client);

	if (MQTT_HAS_STATE(client, MQTT_STATE_DISCONNECTING)) {
		client_disconnect(client, 0);
	} else {
		elapsed_time = mqtt_elapsed_time_in_ms_get(
					client->internal.last_activity);

		if ((client->keepalive > 0) &&
		    (elapsed_time >= (client->keepalive * 1000))) {
			(void)mqtt_ping(client);
		}
	}

	mqtt_mutex_unlock(client);

	return 0;
}

int mqtt_input(struct mqtt_client *client)
{
	int err_code = 0;

	NULL_PARAM_CHECK(client);

	mqtt_mutex_lock(client);

	MQTT_TRC("state:0x%08x", client->internal.state);

	if (MQTT_HAS_STATE(client, MQTT_STATE_DISCONNECTING)) {
		client_disconnect(client, 0);
	} else if (MQTT_HAS_STATE(client, MQTT_STATE_TCP_CONNECTED)) {
		err_code = client_read(client);
	} else {
		err_code = -EACCES;
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

		client_disconnect(client, ret);
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

int mqtt_readall_publish_payload(struct mqtt_client *client, u8_t *buffer,
				 size_t length)
{
	u8_t *end = buffer + length;

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

