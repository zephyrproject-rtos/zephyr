/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_rx, CONFIG_MQTT_LOG_LEVEL);

#include "mqtt_internal.h"
#include "mqtt_transport.h"
#include "mqtt_os.h"

/** @file mqtt_rx.c
 *
 * @brief MQTT Received data handling.
 */

static int mqtt_handle_packet(struct mqtt_client *client,
			      uint8_t type_and_flags,
			      uint32_t var_length,
			      struct buf_ctx *buf)
{
	int err_code = 0;
	bool notify_event = true;
	struct mqtt_evt evt = { 0 };

	/* Success by default, overwritten in special cases. */
	evt.result = 0;

	switch (type_and_flags & 0xF0) {
	case MQTT_PKT_TYPE_CONNACK:
		NET_DBG("[CID %p]: Received MQTT_PKT_TYPE_CONNACK!", client);

		evt.type = MQTT_EVT_CONNACK;
		err_code = connect_ack_decode(client, buf, &evt.param.connack);
		if (err_code == 0) {
			NET_DBG("[CID %p]: return_code: %d", client,
				 evt.param.connack.return_code);

			/* For MQTT 5.0 this is still valid as MQTT_CONNACK_SUCCESS
			 * is encoded as 0 as well.
			 */
			if (evt.param.connack.return_code ==
						MQTT_CONNECTION_ACCEPTED) {
				/* Set state. */
				MQTT_SET_STATE(client, MQTT_STATE_CONNECTED);
			} else {
				err_code = -ECONNREFUSED;
			}

			evt.result = evt.param.connack.return_code;
		} else {
			evt.result = err_code;
		}

		break;

	case MQTT_PKT_TYPE_PUBLISH:
		NET_DBG("[CID %p]: Received MQTT_PKT_TYPE_PUBLISH", client);

		evt.type = MQTT_EVT_PUBLISH;
		err_code = publish_decode(client, type_and_flags, var_length,
					  buf, &evt.param.publish);
		evt.result = err_code;

		client->internal.remaining_payload =
					evt.param.publish.message.payload.len;

		NET_DBG("PUB QoS:%02x, message len %08x, topic len %08x",
			 evt.param.publish.message.topic.qos,
			 evt.param.publish.message.payload.len,
			 evt.param.publish.message.topic.topic.size);

		break;

	case MQTT_PKT_TYPE_PUBACK:
		NET_DBG("[CID %p]: Received MQTT_PKT_TYPE_PUBACK!", client);

		evt.type = MQTT_EVT_PUBACK;
		err_code = publish_ack_decode(buf, &evt.param.puback);
		evt.result = err_code;
		break;

	case MQTT_PKT_TYPE_PUBREC:
		NET_DBG("[CID %p]: Received MQTT_PKT_TYPE_PUBREC!", client);

		evt.type = MQTT_EVT_PUBREC;
		err_code = publish_receive_decode(buf, &evt.param.pubrec);
		evt.result = err_code;
		break;

	case MQTT_PKT_TYPE_PUBREL:
		NET_DBG("[CID %p]: Received MQTT_PKT_TYPE_PUBREL!", client);

		evt.type = MQTT_EVT_PUBREL;
		err_code = publish_release_decode(buf, &evt.param.pubrel);
		evt.result = err_code;
		break;

	case MQTT_PKT_TYPE_PUBCOMP:
		NET_DBG("[CID %p]: Received MQTT_PKT_TYPE_PUBCOMP!", client);

		evt.type = MQTT_EVT_PUBCOMP;
		err_code = publish_complete_decode(buf, &evt.param.pubcomp);
		evt.result = err_code;
		break;

	case MQTT_PKT_TYPE_SUBACK:
		NET_DBG("[CID %p]: Received MQTT_PKT_TYPE_SUBACK!", client);

		evt.type = MQTT_EVT_SUBACK;
		err_code = subscribe_ack_decode(buf, &evt.param.suback);
		evt.result = err_code;
		break;

	case MQTT_PKT_TYPE_UNSUBACK:
		NET_DBG("[CID %p]: Received MQTT_PKT_TYPE_UNSUBACK!", client);

		evt.type = MQTT_EVT_UNSUBACK;
		err_code = unsubscribe_ack_decode(buf, &evt.param.unsuback);
		evt.result = err_code;
		break;

	case MQTT_PKT_TYPE_PINGRSP:
		NET_DBG("[CID %p]: Received MQTT_PKT_TYPE_PINGRSP!", client);

		if (client->unacked_ping <= 0) {
			NET_WARN("Unexpected PINGRSP");
			client->unacked_ping = 0;
		} else {
			client->unacked_ping--;
		}

		evt.type = MQTT_EVT_PINGRESP;
		break;

	default:
		/* Nothing to notify. */
		notify_event = false;
		break;
	}

	if (notify_event == true) {
		event_notify(client, &evt);
	}

	return err_code;
}

static int mqtt_read_message_chunk(struct mqtt_client *client,
				   struct buf_ctx *buf, uint32_t length)
{
	uint32_t remaining;
	int len;

	/* In case all data requested has already been buffered, return. */
	if (length <= (buf->end - buf->cur)) {
		return 0;
	}

	/* Calculate how much data we need to read from the transport,
	 * given the already buffered data.
	 */
	remaining = length - (buf->end - buf->cur);

	/* Check if read does not exceed the buffer. */
	if ((buf->end + remaining > client->rx_buf + client->rx_buf_size) ||
	    (buf->end + remaining < client->rx_buf)) {
		NET_ERR("[CID %p]: Read would exceed RX buffer bounds.",
			 client);
		return -ENOMEM;
	}

	len = mqtt_transport_read(client, buf->end, remaining, false);
	if (len < 0) {
		if (len != -EAGAIN) {
			NET_ERR("[CID %p]: Transport read error: %d", client, len);
		}
		return len;
	}

	if (len == 0) {
		NET_ERR("[CID %p]: Connection closed.", client);
		return -ENOTCONN;
	}

	client->internal.rx_buf_datalen += len;
	buf->end += len;

	if (len < remaining) {
		NET_ERR("[CID %p]: Message partially received.", client);
		return -EAGAIN;
	}

	return 0;
}

static int mqtt_read_publish_var_header(struct mqtt_client *client,
					uint8_t type_and_flags,
					struct buf_ctx *buf)
{
	uint8_t qos = (type_and_flags & MQTT_HEADER_QOS_MASK) >> 1;
	int err_code;
	uint32_t variable_header_length;

	/* Read topic length field. */
	err_code = mqtt_read_message_chunk(client, buf, sizeof(uint16_t));
	if (err_code < 0) {
		return err_code;
	}

	variable_header_length = *buf->cur << 8; /* MSB */
	variable_header_length |= *(buf->cur + 1); /* LSB */

	/* Add two bytes for topic length field. */
	variable_header_length += sizeof(uint16_t);

	/* Add two bytes for message_id, if needed. */
	if (qos > MQTT_QOS_0_AT_MOST_ONCE) {
		variable_header_length += sizeof(uint16_t);
	}

	if (mqtt_is_version_5_0(client)) {
		struct buf_ctx backup;
		uint8_t var_len = 1;
		uint32_t prop_len = 0;

		while (true) {
			err_code = mqtt_read_message_chunk(
				client, buf, variable_header_length + var_len);
			if (err_code < 0) {
				return err_code;
			}

			backup = *buf;
			buf->cur += variable_header_length;

			/* Try to decode variable integer, in case integer is
			 * not complete, read more bytes from the stream and retry.
			 */
			err_code = unpack_variable_int(buf, &prop_len);
			if (err_code >= 0) {
				break;
			}

			if (err_code != -EAGAIN) {
				return err_code;
			}

			/* Try again. */
			var_len++;
			*buf = backup;
		}

		*buf = backup;
		variable_header_length += var_len + prop_len;
	}

	err_code = mqtt_read_message_chunk(client, buf,
					   variable_header_length);
	if (err_code < 0) {
		return err_code;
	}

	return 0;
}

static int mqtt_read_and_parse_fixed_header(struct mqtt_client *client,
					    uint8_t *type_and_flags,
					    uint32_t *var_length,
					    struct buf_ctx *buf)
{
	/* Read the mandatory part of the fixed header in first iteration. */
	uint8_t chunk_size = MQTT_FIXED_HEADER_MIN_SIZE;
	int err_code;

	do {
		err_code = mqtt_read_message_chunk(client, buf, chunk_size);
		if (err_code < 0) {
			return err_code;
		}

		/* Reset to pointer to the beginning of the frame. */
		buf->cur = client->rx_buf;
		chunk_size = 1U;

		err_code = fixed_header_decode(buf, type_and_flags, var_length);
	} while (err_code == -EAGAIN);

	return err_code;
}

int mqtt_handle_rx(struct mqtt_client *client)
{
	int err_code;
	uint8_t type_and_flags;
	uint32_t var_length;
	struct buf_ctx buf;

	buf.cur = client->rx_buf;
	buf.end = client->rx_buf + client->internal.rx_buf_datalen;

	err_code = mqtt_read_and_parse_fixed_header(client, &type_and_flags,
						    &var_length, &buf);
	if (err_code < 0) {
		return (err_code == -EAGAIN) ? 0 : err_code;
	}

	if ((type_and_flags & 0xF0) == MQTT_PKT_TYPE_PUBLISH) {
		err_code = mqtt_read_publish_var_header(client, type_and_flags,
							&buf);
	} else {
		err_code = mqtt_read_message_chunk(client, &buf, var_length);
	}

	if (err_code < 0) {
		return (err_code == -EAGAIN) ? 0 : err_code;
	}

	/* At this point, packet is ready to be passed to the application. */
	err_code = mqtt_handle_packet(client, type_and_flags, var_length, &buf);
	if (err_code < 0) {
		return err_code;
	}

	client->internal.rx_buf_datalen = 0U;

	return 0;
}
