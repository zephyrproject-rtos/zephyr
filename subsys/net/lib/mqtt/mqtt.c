/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/mqtt.h>
#include "mqtt_pkt.h"

#include <net/net_ip.h>
#include <net/nbuf.h>
#include <net/buf.h>
#include <errno.h>

#define MSG_SIZE	CONFIG_MQTT_MSG_MAX_SIZE
#define MQTT_BUF_CTR	(1 + CONFIG_MQTT_ADDITIONAL_BUFFER_CTR)

/* Memory pool internally used to handle messages that may exceed the size of
 * system defined network buffer. By using this memory pool, routines don't deal
 * with fragmentation, so algorithms are more easy to implement.
 */
NET_BUF_POOL_DEFINE(mqtt_msg_pool, MQTT_BUF_CTR, MSG_SIZE, 0, NULL);

#define MQTT_PUBLISHER_MIN_MSG_SIZE	2

int mqtt_tx_connect(struct mqtt_ctx *ctx, struct mqtt_connect_msg *msg)
{
	struct net_buf *data = NULL;
	struct net_buf *tx = NULL;
	int rc;

	data = net_buf_alloc(&mqtt_msg_pool, ctx->net_timeout);
	if (data == NULL) {
		rc = -ENOMEM;
		goto exit_connect;
	}

	ctx->clean_session = msg->clean_session ? 1 : 0;

	rc = mqtt_pack_connect(data->data, &data->len, MSG_SIZE, msg);
	if (rc != 0) {
		rc = -EINVAL;
		goto exit_connect;
	}

	tx = net_nbuf_get_tx(ctx->net_ctx, ctx->net_timeout);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_connect;
	}

	net_buf_frag_add(tx, data);
	data = NULL;

	rc = net_context_send(tx, NULL, ctx->net_timeout, NULL, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_connect;
	}

	tx = NULL;

exit_connect:
	net_nbuf_unref(data);
	net_nbuf_unref(tx);

	return rc;
}

int mqtt_tx_disconnect(struct mqtt_ctx *ctx)
{
	struct net_buf *tx = NULL;
	/* DISCONNECT is a zero length message: 2 bytes required, no payload */
	uint8_t msg[2];
	uint16_t len;
	int rc;

	rc = mqtt_pack_disconnect(msg, &len, sizeof(msg));
	if (rc != 0) {
		rc = -EINVAL;
		goto exit_disconnect;
	}

	tx = net_nbuf_get_tx(ctx->net_ctx, ctx->net_timeout);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_disconnect;
	}

	rc = net_nbuf_append(tx, len, msg, ctx->net_timeout);
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
	tx = NULL;

	if (ctx->disconnect) {
		ctx->disconnect(ctx);
	}

exit_disconnect:
	net_nbuf_unref(tx);

	return rc;
}

/**
 * Writes the MQTT PUBxxx msg indicated by pkt_type with identifier 'id'
 *
 * @param [in] ctx MQTT context
 * @param [in] id MQTT packet identifier
 * @param [in] pkt_type MQTT packet type
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM if a tx buffer is not available
 * @retval -EIO on network error
 */
static
int mqtt_tx_pub_msgs(struct mqtt_ctx *ctx, uint16_t id,
		     enum mqtt_packet pkt_type)
{
	struct net_buf *tx = NULL;
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

	tx = net_nbuf_get_tx(ctx->net_ctx, ctx->net_timeout);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_send;
	}

	rc = net_nbuf_append(tx, len, msg, ctx->net_timeout);
	if (rc != true) {
		rc = -ENOMEM;
		goto exit_send;
	}

	rc = net_context_send(tx, NULL, ctx->net_timeout, NULL, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_send;
	}

	tx = NULL;

exit_send:
	net_nbuf_unref(tx);

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
	struct net_buf *data = NULL;
	struct net_buf *tx = NULL;
	int rc;

	data = net_buf_alloc(&mqtt_msg_pool, ctx->net_timeout);
	if (data == NULL) {
		rc = -ENOMEM;
		goto exit_publish;
	}

	rc = mqtt_pack_publish(data->data, &data->len, data->size, msg);
	if (rc != 0) {
		rc = -EINVAL;
		goto exit_publish;
	}

	tx = net_nbuf_get_tx(ctx->net_ctx, ctx->net_timeout);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_publish;
	}

	net_buf_frag_add(tx, data);
	data = NULL;

	rc = net_context_send(tx, NULL, ctx->net_timeout, NULL, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_publish;
	}

	tx = NULL;

exit_publish:
	net_nbuf_unref(data);
	net_nbuf_unref(tx);

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

	tx = net_nbuf_get_tx(ctx->net_ctx, ctx->net_timeout);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_pingreq;
	}

	rc = net_nbuf_append(tx, len, msg, ctx->net_timeout);
	if (rc != true) {
		rc = -ENOMEM;
		goto exit_pingreq;
	}

	rc = net_context_send(tx, NULL, ctx->net_timeout, NULL, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_pingreq;
	}

	tx = NULL;

exit_pingreq:
	net_nbuf_unref(tx);

	return rc;
}

int mqtt_tx_subscribe(struct mqtt_ctx *ctx, uint16_t pkt_id, uint8_t items,
		      const char *topics[], const enum mqtt_qos qos[])
{
	struct net_buf *data = NULL;
	struct net_buf *tx = NULL;
	int rc;

	data = net_buf_alloc(&mqtt_msg_pool, ctx->net_timeout);
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

	tx = net_nbuf_get_tx(ctx->net_ctx, ctx->net_timeout);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_subs;
	}

	net_buf_frag_add(tx, data);
	data = NULL;

	rc = net_context_send(tx, NULL, ctx->net_timeout, NULL, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_subs;
	}

	tx = NULL;

exit_subs:
	net_nbuf_unref(data);
	net_nbuf_unref(tx);

	return rc;
}

int mqtt_tx_unsubscribe(struct mqtt_ctx *ctx, uint16_t pkt_id, uint8_t items,
			const char *topics[])
{
	struct net_buf *data = NULL;
	struct net_buf *tx = NULL;
	int rc;

	data = net_buf_alloc(&mqtt_msg_pool, ctx->net_timeout);
	if (data == NULL) {
		rc = -ENOMEM;
		goto exit_unsub;
	}

	rc = mqtt_pack_unsubscribe(data->data, &data->len, data->size, pkt_id,
				   items, topics);
	if (rc != 0) {
		rc = -EINVAL;
		goto exit_unsub;
	}

	tx = net_nbuf_get_tx(ctx->net_ctx, ctx->net_timeout);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_unsub;
	}

	net_buf_frag_add(tx, data);
	data = NULL;

	rc = net_context_send(tx, NULL, ctx->net_timeout, NULL, NULL);
	if (rc < 0) {
		rc = -EIO;
		goto exit_unsub;
	}

	tx = NULL;

exit_unsub:
	net_buf_unref(data);
	net_buf_unref(tx);

	return rc;
}

int mqtt_rx_connack(struct mqtt_ctx *ctx, struct net_buf *rx, int clean_session)
{
	uint16_t len;
	uint8_t connect_rc;
	uint8_t session;
	uint8_t *data;
	int rc;

	data = rx->data;
	len = rx->len;

	/* CONNACK is 4 bytes len */
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
			goto exit_connect;
		}
		break;
	/* previous session */
	case 0:
		/* TODO */
		/* FALLTHROUGH */
	default:
		rc = -EINVAL;
		goto exit_connect;
	}

	ctx->connected = 1;

	if (ctx->connect) {
		ctx->connect(ctx);
	}

exit_connect:
	return rc;
}

/**
 * Parses and validates the MQTT PUBxxxx message contained in the rx buffer.
 *
 *
 * @details It validates against message structure and Packet Identifier.
 * For the MQTT PUBREC and PUBREL messages, this function writes the
 * corresponding MQTT PUB msg.
 *
 * @param ctx MQTT context
 * @param rx RX buffer
 * @param type MQTT Packet type
 *
 * @retval 0 on success
 * @retval -EINVAL on error
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

	data = rx->data;
	len = rx->len;

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
			rc = ctx->publish_rx(ctx, NULL, pkt_id, MQTT_PUBREL);
		} else {
			rc = -EINVAL;
		}
	} else {
		rc = ctx->publish_tx(ctx, pkt_id, type);
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
	int rc;

	ARG_UNUSED(ctx);

	/* 2 bytes message */
	rc = mqtt_unpack_pingresp(rx->data, rx->len);

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

	data = rx->data;
	len = rx->len;

	rc = mqtt_unpack_suback(data, len, &pkt_id, &items,
				CONFIG_MQTT_SUBSCRIBE_MAX_TOPICS, suback_qos);
	if (rc != 0) {
		return -EINVAL;
	}

	if (!ctx->subscribe) {
		return -EINVAL;
	}

	rc = ctx->subscribe(ctx, pkt_id, items, suback_qos);
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

	data = rx->data;
	len = rx->len;

	/* 4 bytes message */
	rc = mqtt_unpack_unsuback(data, len, &pkt_id);
	if (rc != 0) {
		return -EINVAL;
	}

	if (!ctx->unsubscribe) {
		return -EINVAL;
	}

	rc = ctx->unsubscribe(ctx, pkt_id);
	if (rc != 0) {
		return -EINVAL;
	}

	return 0;
}

int mqtt_rx_publish(struct mqtt_ctx *ctx, struct net_buf *rx)
{
	struct mqtt_publish_msg msg;
	int rc;

	rc = mqtt_unpack_publish(rx->data, rx->len, &msg);
	if (rc != 0) {
		return -EINVAL;
	}

	rc = ctx->publish_rx(ctx, &msg, msg.pkt_id, MQTT_PUBLISH);
	if (rc != 0) {
		return -EINVAL;
	}

	switch (msg.qos) {
	case MQTT_QoS2:
		rc = mqtt_tx_pubrec(ctx, msg.pkt_id);
		break;
	case MQTT_QoS1:
		rc = mqtt_tx_puback(ctx, msg.pkt_id);
		break;
	case MQTT_QoS0:
		break;
	default:
		rc = -EINVAL;
	}

	return rc;
}

/**
 * Linearizes an IP fragmented buffer
 *
 * @param [in] ctx MQTT context structure
 * @param [in] rx RX IP stack buffer
 * @param [in] min_size Min message size allowed. This allows us to exit if the
 * rx buffer is shorter than the expected msg size
 *
 * @retval Data buffer
 * @retval NULL on error
 */
static
struct net_buf *mqtt_linearize_buffer(struct mqtt_ctx *ctx, struct net_buf *rx,
				      uint16_t min_size)
{
	struct net_buf *data = NULL;
	uint16_t data_len;
	uint16_t offset;
	int rc;

	/* CONFIG_MQTT_MSG_MAX_SIZE is defined via Kconfig. So here it's
	 * determined if the input buffer could fit our data buffer or if
	 * it has the expected size.
	 */
	data_len = net_nbuf_appdatalen(rx);
	if (data_len < min_size || data_len > CONFIG_MQTT_MSG_MAX_SIZE) {
		return NULL;
	}

	data = net_buf_alloc(&mqtt_msg_pool, ctx->net_timeout);
	if (data == NULL) {
		return NULL;
	}

	offset = net_buf_frags_len(rx) - data_len;
	rc = net_nbuf_linear_copy(data, rx, offset, data_len);
	if (rc != 0) {
		goto exit_error;
	}

	return data;

exit_error:
	net_nbuf_unref(data);

	return NULL;
}

/**
 * Calls the appropriate rx routine for the MQTT message contained in rx
 *
 * @details On error, this routine will execute the 'ctx->malformed' callback
 * (if defined)
 *
 * @param ctx MQTT context
 * @param rx RX buffer
 *
 * @retval 0 on success
 * @retval -EINVAL if an unknown message is received
 * @retval -ENOMEM if no data buffer is available
 * @retval mqtt_rx_connack, mqtt_rx_puback, mqtt_rx_pubrec, mqtt_rx_pubcomp
 *         and mqtt_rx_pingresp return codes
 */
static
int mqtt_publisher_parser(struct mqtt_ctx *ctx, struct net_buf *rx)
{
	uint16_t pkt_type = MQTT_INVALID;
	struct net_buf *data = NULL;
	int rc = -EINVAL;

	data = mqtt_linearize_buffer(ctx, rx, MQTT_PUBLISHER_MIN_MSG_SIZE);
	if (!data) {
		rc = -ENOMEM;
		goto exit_parser;
	}

	pkt_type = MQTT_PACKET_TYPE(data->data[0]);

	switch (pkt_type) {
	case MQTT_CONNACK:
		if (!ctx->connected) {
			rc = mqtt_rx_connack(ctx, data, ctx->clean_session);
		} else {
			rc = -EINVAL;
		}
		break;
	case MQTT_PUBACK:
		rc = mqtt_rx_puback(ctx, data);
		break;
	case MQTT_PUBREC:
		rc = mqtt_rx_pubrec(ctx, data);
		break;
	case MQTT_PUBCOMP:
		rc = mqtt_rx_pubcomp(ctx, data);
		break;
	case MQTT_PINGRESP:
		rc = mqtt_rx_pingresp(ctx, data);
		break;
	default:
		rc = -EINVAL;
		break;
	}

exit_parser:
	if (rc != 0 && ctx->malformed) {
		ctx->malformed(ctx, pkt_type);
	}

	net_nbuf_unref(data);

	return rc;
}


/**
 * Calls the appropriate rx routine for the MQTT message contained in rx
 *
 * @details On error, this routine will execute the 'ctx->malformed' callback
 * (if defined)
 *
 * @param ctx MQTT context
 * @param rx RX buffer
 *
 * @retval 0 on success
 * @retval -EINVAL if an unknown message is received
 * @retval -ENOMEM if no data buffer is available
 * @retval mqtt_rx_publish, mqtt_rx_pubrel, mqtt_rx_pubrel and mqtt_rx_suback
 *         return codes
 */
static
int mqtt_subscriber_parser(struct mqtt_ctx *ctx, struct net_buf *rx)
{
	uint16_t pkt_type = MQTT_INVALID;
	struct net_buf *data = NULL;
	int rc = 0;

	data = mqtt_linearize_buffer(ctx, rx, MQTT_PUBLISHER_MIN_MSG_SIZE);
	if (!data) {
		rc = -EINVAL;
		goto exit_parser;
	}

	pkt_type = MQTT_PACKET_TYPE(data->data[0]);

	switch (pkt_type) {
	case MQTT_CONNACK:
		if (!ctx->connected) {
			rc = mqtt_rx_connack(ctx, data, ctx->clean_session);
		} else {
			rc = -EINVAL;
		}

		break;
	case MQTT_PUBLISH:
		rc = mqtt_rx_publish(ctx, data);
		break;
	case MQTT_PUBREL:
		rc = mqtt_rx_pubrel(ctx, data);
		break;
	case MQTT_PINGRESP:
		rc = mqtt_rx_pubrel(ctx, data);
		break;
	case MQTT_SUBACK:
		rc = mqtt_rx_suback(ctx, data);
		break;
	default:
		rc = -EINVAL;
		break;
	}

exit_parser:
	if (rc != 0 && ctx->malformed) {
		ctx->malformed(ctx, pkt_type);
	}

	net_nbuf_unref(data);

	return rc;
}

static
void mqtt_recv(struct net_context *net_ctx, struct net_buf *buf, int status,
	       void *data)
{
	struct mqtt_ctx *mqtt = (struct mqtt_ctx *)data;

	/* net_ctx is already referenced to by the mqtt_ctx struct */
	ARG_UNUSED(net_ctx);

	if (status || !buf) {
		return;
	}

	if (net_nbuf_appdatalen(buf) == 0) {
		goto lb_exit;
	}

	mqtt->rcv(mqtt, buf);

lb_exit:
	net_nbuf_unref(buf);
}

int mqtt_init(struct mqtt_ctx *ctx, enum mqtt_app app_type)
{
	/* So far, only clean session = 1 is supported */
	ctx->clean_session = 1;
	ctx->connected = 0;

	/* Install the receiver callback, timeout is set to K_NO_WAIT.
	 * In this case, no return code is evaluated.
	 */
	(void)net_context_recv(ctx->net_ctx, mqtt_recv, K_NO_WAIT, ctx);

	ctx->app_type = app_type;

	switch (ctx->app_type) {
	case MQTT_APP_PUBLISHER:
		ctx->rcv = mqtt_publisher_parser;
		break;
	case MQTT_APP_SUBSCRIBER:
		ctx->rcv = mqtt_subscriber_parser;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
