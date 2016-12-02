/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iot/mqtt.h>
#include "mqtt_pkt.h"

#include <net/net_ip.h>
#include <net/nbuf.h>
#include <net/buf.h>
#include <errno.h>

#define MSG_SIZE	CONFIG_MQTT_MSG_MAX_SIZE
#define MQTT_BUF_CTR	(1 + CONFIG_MQTT_ADDITIONAL_BUFFER_CTR)

static struct nano_fifo mqtt_msg_fifo;

/* Memory pool internally used to handle messages that may exceed the size of
 * system defined network buffer. By using this memory pool, routines don't deal
 * with fragmentation, so algorithms are more easy to implement.
 */
static NET_BUF_POOL(mqtt_msg_pool, MQTT_BUF_CTR, MSG_SIZE, &mqtt_msg_fifo,
		    NULL, 0);

int mqtt_init(struct mqtt_ctx *ctx, enum mqtt_app app_type)
{
	ctx->app_type = app_type;

	/* So far, only clean session = 1 is supported */
	ctx->clean_session = 1;

	net_buf_pool_init(mqtt_msg_pool);

	return 0;
}

int mqtt_tx_connect(struct mqtt_ctx *ctx, struct mqtt_connect_msg *msg)
{
	struct net_buf *data;
	struct net_buf *tx;
	int rc;

	data = net_buf_get_timeout(&mqtt_msg_fifo, 0, ctx->net_timeout);
	if (data == NULL) {
		rc = -ENOMEM;
		goto exit_connect;
	}

	ctx->clean_session = msg->clean_session ? 1 : 0;

	rc = mqtt_pack_connect(data->data, &data->len, MSG_SIZE, msg);
	if (rc != 0) {
		net_nbuf_unref(data);
		rc = -EINVAL;
		goto exit_connect;
	}

	tx = net_nbuf_get_tx(ctx->net_ctx);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_connect;
	}

	net_buf_frag_add(tx, data);

	rc = net_context_send(tx, NULL, ctx->net_timeout, NULL, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_connect;
	}

	rc = 0;

exit_connect:
	return rc;
}

int mqtt_tx_disconnect(struct mqtt_ctx *ctx)
{
	struct net_buf *tx;
	/* DISCONNECT is a zero length message: 2 bytes required, no payload */
	uint8_t msg[2];
	uint16_t len;
	int rc;

	rc = mqtt_pack_disconnect(msg, &len, sizeof(msg));
	if (rc != 0) {
		rc = -EINVAL;
		goto exit_disconnect;
	}

	tx = net_nbuf_get_tx(ctx->net_ctx);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_disconnect;
	}

	rc = net_nbuf_append(tx, len, msg);
	if (rc != true) {
		rc = -ENOMEM;
		goto exit_disconnect;
	}

	rc = net_context_send(tx, NULL, ctx->net_timeout, NULL, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_disconnect;
	}

	ctx->connected = 0;
	rc = 0;

	if (ctx->disconnect) {
		ctx->disconnect(ctx->disconnect_data);
	}

exit_disconnect:
	return rc;
}

/**
 * @brief mqtt_tx_pub_msgs	Writes the MQTT PUBxxx msg indicated by pkt_type
 *				with identifier 'id'
 * @param [in] ctx		MQTT context
 * @param [in] id		MQTT packet identifier
 * @param [in] pkt_type		MQTT packet type
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed to
 *				this routine
 * @return			-ENOMEM if a tx buffer is not available
 * @return			-EIO on network error
 */
static
int mqtt_tx_pub_msgs(struct mqtt_ctx *ctx, uint16_t id,
		     enum mqtt_packet pkt_type)
{
	struct net_buf *tx;
	uint8_t msg[4];
	uint16_t len;
	int rc;

	switch (pkt_type) {
	case MQTT_PUBACK:
		rc = mqtt_pack_puback(msg, &len, sizeof(msg), id);
		break;
	case MQTT_PUBCOMP:
		rc = mqtt_pack_pubcomp(msg, &len, sizeof(msg), id);
		break;
	case MQTT_PUBREC:
		rc = mqtt_pack_pubrec(msg, &len, sizeof(msg), id);
		break;
	case MQTT_PUBREL:
		rc = mqtt_pack_pubrel(msg, &len, sizeof(msg), id);
		break;
	default:
		return -EINVAL;
	}

	if (rc != 0) {
		return -EINVAL;
	}

	tx = net_nbuf_get_tx(ctx->net_ctx);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_send;
	}

	rc = net_nbuf_append(tx, len, msg);
	if (rc != true) {
		rc = -ENOMEM;
		goto exit_send;
	}

	rc = net_context_send(tx, NULL, ctx->net_timeout, NULL, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_send;
	}

	rc = 0;

exit_send:
	return rc;
}

int mqtt_tx_puback(struct mqtt_ctx *ctx, uint16_t id)
{
	return mqtt_tx_pub_msgs(ctx, id, MQTT_PUBACK);
}

int mqtt_tx_pubcomp(struct mqtt_ctx *ctx, uint16_t id)
{
	return mqtt_tx_pub_msgs(ctx, id, MQTT_PUBCOMP);
}

int mqtt_tx_pubrec(struct mqtt_ctx *ctx, uint16_t id)
{
	return mqtt_tx_pub_msgs(ctx, id, MQTT_PUBREC);
}

int mqtt_tx_pubrel(struct mqtt_ctx *ctx, uint16_t id)
{
	return mqtt_tx_pub_msgs(ctx, id, MQTT_PUBREL);
}

int mqtt_tx_publish(struct mqtt_ctx *ctx, struct mqtt_publish_msg *msg)
{
	struct net_buf *data;
	struct net_buf *tx;
	int rc;

	data = net_buf_get_timeout(&mqtt_msg_fifo, 0, ctx->net_timeout);
	if (data == NULL) {
		rc = -ENOMEM;
		goto exit_publish;
	}

	rc = mqtt_pack_publish(data->data, &data->len, data->size, msg);
	if (rc != 0) {
		net_nbuf_unref(data);
		rc = -EINVAL;
		goto exit_publish;
	}

	tx = net_nbuf_get_tx(ctx->net_ctx);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_publish;
	}

	net_buf_frag_add(tx, data);

	rc = net_context_send(tx, NULL, ctx->net_timeout, NULL, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_publish;
	}

	rc = 0;

exit_publish:
	return rc;
}

int mqtt_tx_pingreq(struct mqtt_ctx *ctx)
{
	struct net_buf *tx = NULL;
	uint8_t msg[2];
	uint16_t len;
	int rc;

	rc = mqtt_pack_pingreq(msg, &len, sizeof(msg));
	if (rc != 0) {
		rc = -EINVAL;
		goto exit_pingreq;
	}

	tx = net_nbuf_get_tx(ctx->net_ctx);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_pingreq;
	}

	net_nbuf_append(tx, len, msg);

	rc = net_context_send(tx, NULL, ctx->net_timeout, NULL, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_pingreq;
	}

	rc = 0;

exit_pingreq:
	return rc;
}

int mqtt_tx_subscribe(struct mqtt_ctx *ctx, uint16_t pkt_id, int items,
		      const char *topics[], enum mqtt_qos qos[])
{
	struct net_buf *data;
	struct net_buf *tx;
	int rc;

	data = net_buf_get_timeout(&mqtt_msg_fifo, 0, ctx->net_timeout);
	if (data == NULL) {
		rc = -ENOMEM;
		goto exit_subs;
	}

	rc = mqtt_pack_subscribe(data->data, &data->len, data->size,
				 pkt_id, items, topics, qos);
	if (rc != 0) {
		net_nbuf_unref(data);
		rc = -EINVAL;
		goto exit_subs;
	}

	tx = net_nbuf_get_tx(ctx->net_ctx);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_subs;
	}

	net_buf_frag_add(tx, data);

	rc = net_context_send(tx, NULL, ctx->net_timeout, NULL, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_subs;
	}

	rc = 0;

exit_subs:
	return rc;
}

int mqtt_tx_unsubscribe(struct mqtt_ctx *ctx, uint16_t pkt_id, int items,
			const char *topics[])
{
	struct net_buf *data;
	struct net_buf *tx;
	int rc;

	data = net_buf_get_timeout(&mqtt_msg_fifo, 0, ctx->net_timeout);
	if (data == NULL) {
		rc = -ENOMEM;
		goto exit_unsub;
	}

	rc = mqtt_pack_unsubscribe(data->data, &data->len, data->size, pkt_id,
				   items, topics);
	if (rc != 0) {
		net_buf_unref(data);
		rc = -EINVAL;
		goto exit_unsub;
	}

	tx = net_nbuf_get_tx(ctx->net_ctx);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_unsub;
	}

	net_buf_frag_add(tx, data);

	rc = net_context_send(tx, NULL, ctx->net_timeout, NULL, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_unsub;
	}

	rc = 0;

exit_unsub:
	return rc;
}

int mqtt_rx_connack(struct mqtt_ctx *ctx, struct net_buf *rx, int clean_session)
{
	uint16_t len;
	uint8_t connect_rc;
	uint8_t session;
	uint8_t *data;
	int rc;

	data = net_nbuf_appdata(rx);
	len = net_nbuf_appdatalen(rx);

	/* CONNACK is only 4 bytes len, so it is assumed
	 * that net buf traversing is not required here
	 */
	rc = mqtt_unpack_connack(data, len, &session, &connect_rc);
	if (rc != 0) {
		rc = -EINVAL;
		goto exit_connect;
	}

	switch (clean_session) {
	/* new session */
	case 1:
		/* server acks there is no previous session
		 * and server connection return code is OK
		 */
		if (session == 0 && connect_rc == 0) {
			rc = 0;
		} else {
			rc = -EINVAL;
		}
		break;
	/* previous session */
	case 0:
		/* TODO */
		/* FALLTHROUGH */
	default:
		rc = -EINVAL;
		break;
	}

	ctx->connected = 1;

	if (ctx->connect) {
		ctx->connect(ctx->connect_data);
	}

exit_connect:
	return rc;
}

/**
 * @brief mqtt_rx_pub_msgs	Parses and validates the MQTT PUBxxxx message
 *				contained in the rx buffer. It validates against
 *				message structure and Packet Identifier.
 * @details			For the MQTT PUBREC and PUBREL messages, this
 *				function writes the corresponding MQTT PUB msg.
 * @param ctx			MQTT context
 * @param rx			RX buffer
 * @param type			MQTT Packet type
 * @return			0 on success
 * @return			-EINVAL on error
 */
static
int mqtt_rx_pub_msgs(struct mqtt_ctx *ctx, struct net_buf *rx,
		     enum mqtt_packet type)
{
	int (*unpack)(uint8_t *, uint16_t, uint16_t *) = NULL;
	int (*response)(struct mqtt_ctx *, uint16_t) = NULL;
	uint16_t pkt_id;
	uint16_t len;
	uint8_t *data;
	int rc;

	switch (type) {
	case MQTT_PUBACK:
		unpack = mqtt_unpack_puback;
		break;
	case MQTT_PUBCOMP:
		unpack = mqtt_unpack_pubcomp;
		break;
	case MQTT_PUBREC:
		unpack = mqtt_unpack_pubrec;
		response = mqtt_tx_pubrel;
		break;
	case MQTT_PUBREL:
		unpack = mqtt_unpack_pubrel;
		response = mqtt_tx_pubcomp;
		break;
	default:
		return -EINVAL;
	}

	data = net_nbuf_appdata(rx);
	len = net_nbuf_appdatalen(rx);

	/* 4 bytes message */
	rc = unpack(data, len, &pkt_id);
	if (rc != 0) {
		return -EINVAL;
	}

	/* Only MQTT_APP_SUBSCRIBER, MQTT_APP_PUBLISHER_SUBSCRIBER and
	 * MQTT_APP_SERVER apps must receive the MQTT_PUBREL msg.
	 */
	if (type == MQTT_PUBREL) {
		if (ctx->app_type != MQTT_APP_PUBLISHER) {
			rc = ctx->publish_rx(ctx->publish_rx_data, NULL, pkt_id,
					     MQTT_PUBREL);
		} else {
			rc = -EINVAL;
		}
	} else {
		rc = ctx->publish_tx(ctx->publish_tx_data, pkt_id, type);
	}

	if (rc != 0) {
		return -EINVAL;
	}

	if (!response)  {
		return 0;
	}

	rc = response(ctx, pkt_id);
	if (rc != 0) {
		return -EINVAL;
	}

	return 0;
}

int mqtt_rx_puback(struct mqtt_ctx *ctx, struct net_buf *rx)
{
	return mqtt_rx_pub_msgs(ctx, rx, MQTT_PUBACK);
}

int mqtt_rx_pubcomp(struct mqtt_ctx *ctx, struct net_buf *rx)
{
	return mqtt_rx_pub_msgs(ctx, rx, MQTT_PUBCOMP);
}

int mqtt_rx_pubrec(struct mqtt_ctx *ctx, struct net_buf *rx)
{
	return mqtt_rx_pub_msgs(ctx, rx, MQTT_PUBREC);
}

int mqtt_rx_pubrel(struct mqtt_ctx *ctx, struct net_buf *rx)
{
	return mqtt_rx_pub_msgs(ctx, rx, MQTT_PUBREL);
}

int mqtt_rx_pingresp(struct mqtt_ctx *ctx, struct net_buf *rx)
{
	uint8_t *data;
	uint16_t len;
	int rc;

	ARG_UNUSED(ctx);

	data = net_nbuf_appdata(rx);
	len = net_nbuf_appdatalen(rx);

	/* 2 bytes message */
	rc = mqtt_unpack_pingresp(data, len);

	if (rc != 0) {
		return -EINVAL;
	}

	return 0;
}

int mqtt_rx_suback(struct mqtt_ctx *ctx, struct net_buf *rx)
{
	enum mqtt_qos suback_qos[CONFIG_MQTT_SUBSCRIBE_MAX_TOPICS];
	uint16_t pkt_id;
	uint16_t len;
	uint8_t items;
	uint8_t *data;
	int rc;

	data = net_nbuf_appdata(rx);
	len = net_nbuf_appdatalen(rx);

	rc = mqtt_unpack_suback(data, len, &pkt_id, &items,
				CONFIG_MQTT_SUBSCRIBE_MAX_TOPICS, suback_qos);
	if (rc != 0) {
		return -EINVAL;
	}

	if (!ctx->subscribe) {
		return -EINVAL;
	}

	rc = ctx->subscribe(ctx->subscribe_data, pkt_id, items, suback_qos);
	if (rc != 0) {
		return -EINVAL;
	}

	return 0;
}

int mqtt_rx_unsuback(struct mqtt_ctx *ctx, struct net_buf *rx)
{
	uint16_t pkt_id;
	uint16_t len;
	uint8_t *data;
	int rc;

	data = net_nbuf_appdata(rx);
	len = net_nbuf_appdatalen(rx);

	/* 4 bytes message */
	rc = mqtt_unpack_unsuback(data, len, &pkt_id);
	if (rc != 0) {
		return -EINVAL;
	}

	if (!ctx->unsubscribe) {
		return -EINVAL;
	}

	rc = ctx->unsubscribe(ctx->subscribe_data, pkt_id);
	if (rc != 0) {
		return -EINVAL;
	}

	return 0;
}
