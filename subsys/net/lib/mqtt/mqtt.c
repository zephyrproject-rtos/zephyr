/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "net_utils.h"
#include "mqtt_pkt.h"

#include <net/net_ip.h>
#include <net/zstream.h>
#include <net/async_socket.h>
/*
 * Issue #5817 workaround:
 * Ensuring mqtt.h after socket.h so .connect -> .zsock_connect
 * everywhere:
 */
#include <net/mqtt.h>
#include <net/buf.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define MEM_ALIGN	(sizeof(uint32_t))
#define MSG_SIZE	CONFIG_MQTT_MSG_MAX_SIZE
#define MQTT_BUF_CTR	(2 + CONFIG_MQTT_ADDITIONAL_BUFFER_CTR)

#define INVALID_SOCK	(-1)

/* Reset rc to 0 if rc > 0 (meaning, rc bytes sent), indicating success: */
#define SET_ERRNO_AND_RC(rc) { rc = (rc < 0 ? -errno : 0); }

/*
 * Memory pool for MQTT message buffers.
 * The number of buffers should equal the number of mqtt contexts
 * created by the application + one.
 * - Each mqtt context needs a buffer for sending.
 * - One buffer is needed for receiving across all contexts.
 */
K_MEM_SLAB_DEFINE(mqtt_msg_pool, MSG_SIZE, MQTT_BUF_CTR, MEM_ALIGN);

static void *_mqtt_msg_alloc()
{
	void *buf = NULL;

	if (k_mem_slab_alloc(&mqtt_msg_pool, (void **)&buf, K_NO_WAIT) < 0) {
		/*
		 * We assert, as this is a logic error, due to the application
		 * overallocating from the mqtt msg pool.
		 */
		__ASSERT(0, "Increase size of mqtt msg pool");
	}
	return buf;
}

static void _mqtt_msg_free(void *buf)
{
	k_mem_slab_free(&mqtt_msg_pool, (void **)&buf);
}


#define MQTT_PUBLISHER_MIN_MSG_SIZE	2

#if defined(CONFIG_MQTT_LIB_TLS)
#define TLS_HS_DEFAULT_TIMEOUT 3000
#endif

int mqtt_tx_connect(struct mqtt_ctx *ctx, struct mqtt_connect_msg *msg)
{
	void *data = NULL;
	u16_t len;
	int rc;

	data = _mqtt_msg_alloc();
	if (data == NULL) {
		return -ENOMEM;
	}

	ctx->clean_session = msg->clean_session ? 1 : 0;

	rc = mqtt_pack_connect(data, &len, MSG_SIZE, msg);
	if (rc != 0) {
		rc = -EINVAL;
	} else {
		rc = async_send(ctx->stream, data, len, NULL, NULL, 0);
		SET_ERRNO_AND_RC(rc);
	}

	_mqtt_msg_free(data);

	return rc;
}

int mqtt_tx_disconnect(struct mqtt_ctx *ctx)
{
	/* DISCONNECT is a zero length message: 2 bytes required, no payload */
	u8_t msg[2];
	u16_t len;
	int rc;

	rc = mqtt_pack_disconnect(msg, &len, sizeof(msg));
	if (rc != 0) {
		return -EINVAL;
	}

	rc = async_send(ctx->stream, msg, len, NULL, NULL, 0);
	if (rc < 0) {
		rc = -errno;
	} else {
		rc = 0; /* Because caller expects 0 for success */
		ctx->connected = 0;
		/* Call MQTT client's disconnect callback: */
		if (ctx->disconnect) {
			ctx->disconnect(ctx);
		}
	}

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
 * @retval -ENOMEM if a tx pktfer is not available
 * @retval -EIO on network error
 */
static
int mqtt_tx_pub_msgs(struct mqtt_ctx *ctx, u16_t id,
		     enum mqtt_packet pkt_type)
{
	u8_t msg[4];
	u16_t len;
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
		rc = -EINVAL;
	} else {
		rc = async_send(ctx->stream, msg, len, NULL, NULL, 0);
		SET_ERRNO_AND_RC(rc);
	}

	return rc;
}

int mqtt_tx_puback(struct mqtt_ctx *ctx, u16_t id)
{
	return mqtt_tx_pub_msgs(ctx, id, MQTT_PUBACK);
}

int mqtt_tx_pubcomp(struct mqtt_ctx *ctx, u16_t id)
{
	return mqtt_tx_pub_msgs(ctx, id, MQTT_PUBCOMP);
}

int mqtt_tx_pubrec(struct mqtt_ctx *ctx, u16_t id)
{
	return mqtt_tx_pub_msgs(ctx, id, MQTT_PUBREC);
}

int mqtt_tx_pubrel(struct mqtt_ctx *ctx, u16_t id)
{
	return mqtt_tx_pub_msgs(ctx, id, MQTT_PUBREL);
}

int mqtt_tx_publish(struct mqtt_ctx *ctx, struct mqtt_publish_msg *msg)
{
	void *data = NULL;
	u16_t len;
	int rc;

	data = _mqtt_msg_alloc();
	if (data == NULL) {
		return -ENOMEM;
	}

	rc = mqtt_pack_publish(data, &len, MSG_SIZE, msg);
	if (rc != 0) {
		rc = -EINVAL;
	} else {
		rc = async_send(ctx->stream, data, len, NULL, NULL, 0);
		SET_ERRNO_AND_RC(rc);
	}

	_mqtt_msg_free(data);

	return rc;
}

int mqtt_tx_pingreq(struct mqtt_ctx *ctx)
{
	u8_t msg[2];
	u16_t len;
	int rc;

	rc = mqtt_pack_pingreq(msg, &len, sizeof(msg));
	if (rc != 0) {
		return -EINVAL;
	}

	rc = async_send(ctx->stream, msg, len, NULL, NULL, 0);
	SET_ERRNO_AND_RC(rc);

	return rc;
}

int mqtt_tx_subscribe(struct mqtt_ctx *ctx, u16_t pkt_id, u8_t items,
		      const char *topics[], const enum mqtt_qos qos[])
{
	void *data = NULL;
	u16_t len;
	int rc;

	data = _mqtt_msg_alloc();
	if (data == NULL) {
		return -ENOMEM;
	}

	rc = mqtt_pack_subscribe(data, &len, MSG_SIZE,
				 pkt_id, items, topics, qos);
	if (rc != 0) {
		rc = -EINVAL;
	} else {
		rc = async_send(ctx->stream, data, len, NULL, NULL, 0);
		SET_ERRNO_AND_RC(rc);
	}

	_mqtt_msg_free(data);

	return rc;
}

int mqtt_tx_unsubscribe(struct mqtt_ctx *ctx, u16_t pkt_id, u8_t items,
			const char *topics[])
{
	void *data = NULL;
	u16_t len;
	int rc;

	data = _mqtt_msg_alloc();
	if (data == NULL) {
		return -ENOMEM;
	}

	rc = mqtt_pack_unsubscribe(data, &len, MSG_SIZE, pkt_id,
				   items, topics);
	if (rc != 0) {
		rc = -EINVAL;
	} else {
		rc = async_send(ctx->stream, data, len, NULL, NULL, 0);
		SET_ERRNO_AND_RC(rc);
	}

	_mqtt_msg_free(data);

	return rc;
}

int mqtt_rx_connack(struct mqtt_ctx *ctx, void *rx, size_t len,
		    int clean_session)
{
	u8_t connect_rc;
	u8_t session;
	u8_t *data;
	int rc;

	data = (u8_t *)rx;

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
 * Parses and validates the MQTT PUBxxxx message contained in the rx packet.
 *
 *
 * @details It validates against message structure and Packet Identifier.
 * For the MQTT PUBREC and PUBREL messages, this function writes the
 * corresponding MQTT PUB msg.
 *
 * @param ctx MQTT context
 * @param rx RX packet
 * @param type MQTT Packet type
 *
 * @retval 0 on success
 * @retval -EINVAL on error
 */
static
int mqtt_rx_pub_msgs(struct mqtt_ctx *ctx, void *data, size_t len,
		     enum mqtt_packet type)
{
	int (*unpack)(u8_t *, u16_t, u16_t *) = NULL;
	int (*response)(struct mqtt_ctx *, u16_t) = NULL;
	u16_t pkt_id;
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

int mqtt_rx_puback(struct mqtt_ctx *ctx, void *data, size_t len)
{
	return mqtt_rx_pub_msgs(ctx, data, len, MQTT_PUBACK);
}

int mqtt_rx_pubcomp(struct mqtt_ctx *ctx, void *data, size_t len)
{
	return mqtt_rx_pub_msgs(ctx, data, len, MQTT_PUBCOMP);
}

int mqtt_rx_pubrec(struct mqtt_ctx *ctx, void *data, size_t len)
{
	return mqtt_rx_pub_msgs(ctx, data, len, MQTT_PUBREC);
}

int mqtt_rx_pubrel(struct mqtt_ctx *ctx, void *data, size_t len)
{
	return mqtt_rx_pub_msgs(ctx, data, len, MQTT_PUBREL);
}

int mqtt_rx_pingresp(struct mqtt_ctx *ctx, void *data, size_t len)
{
	int rc;

	ARG_UNUSED(ctx);

	/* 2 bytes message */
	rc = mqtt_unpack_pingresp(data, len);

	if (rc != 0) {
		return -EINVAL;
	}

	return 0;
}

int mqtt_rx_suback(struct mqtt_ctx *ctx, void *data, size_t len)
{
	enum mqtt_qos suback_qos[CONFIG_MQTT_SUBSCRIBE_MAX_TOPICS];
	u16_t pkt_id;
	u8_t items;
	int rc;

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

int mqtt_rx_unsuback(struct mqtt_ctx *ctx, void *data, size_t len)
{
	u16_t pkt_id;
	int rc;

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

int mqtt_rx_publish(struct mqtt_ctx *ctx, void *data, size_t len)
{
	struct mqtt_publish_msg msg;
	int rc;

	rc = mqtt_unpack_publish(data, len, &msg);
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
 * Calls the appropriate rx routine for the MQTT message contained in rx
 *
 * @details On error, this routine will execute the 'ctx->malformed' callback
 * (if defined)
 *
 * @param ctx MQTT context
 * @param data Received data buffer
 * @param data Length of data buffer
 *
 * @retval 0 on success
 * @retval -EINVAL if an unknown message is received
 * @retval -ENOMEM if no data buffer is available
 * @retval mqtt_rx_connack, mqtt_rx_pingresp, mqtt_rx_puback, mqtt_rx_pubcomp,
 *         mqtt_rx_publish, mqtt_rx_pubrec, mqtt_rx_pubrel and mqtt_rx_suback
 *         return codes
 */
static
int mqtt_parser(struct mqtt_ctx *ctx, void *data, size_t len)
{
	u16_t pkt_type = MQTT_INVALID;
	int rc = -EINVAL;

	/*
	 * CONFIG_MQTT_MSG_MAX_SIZE is defined via Kconfig. So here it's
	 * determined if the received input could fit our data buffer or if
	 * it has the expected size.
	 */
	if (len < MQTT_PUBLISHER_MIN_MSG_SIZE ||
	    len > CONFIG_MQTT_MSG_MAX_SIZE) {
		return -EMSGSIZE;
	}

	pkt_type = MQTT_PACKET_TYPE(((u8_t *)data)[0]);

	switch (pkt_type) {
	case MQTT_CONNACK:
		if (!ctx->connected) {
			rc = mqtt_rx_connack(ctx, data, len,
					     ctx->clean_session);
		} else {
			rc = -EINVAL;
		}
		break;
	case MQTT_PUBACK:
		rc = mqtt_rx_puback(ctx, data, len);
		break;
	case MQTT_PUBREC:
		rc = mqtt_rx_pubrec(ctx, data, len);
		break;
	case MQTT_PUBCOMP:
		rc = mqtt_rx_pubcomp(ctx, data, len);
		break;
	case MQTT_PINGRESP:
		rc = mqtt_rx_pingresp(ctx, data, len);
		break;
	case MQTT_PUBLISH:
		rc = mqtt_rx_publish(ctx, data, len);
		break;
	case MQTT_PUBREL:
		rc = mqtt_rx_pubrel(ctx, data, len);
		break;
	case MQTT_SUBACK:
		rc = mqtt_rx_suback(ctx, data, len);
		break;
	default:
		rc = -EINVAL;
		break;
	}

	if (rc != 0 && ctx->malformed) {
		ctx->malformed(ctx, pkt_type);
	}

	return rc;
}

static
void async_connected_cb(int sock, int status, void *cb_data)
{
	struct mqtt_ctx *mqtt = (struct mqtt_ctx *)cb_data;

	ARG_UNUSED(sock);
	ARG_UNUSED(status);

	if (!mqtt || !status) {
		return;
	}

#if defined(CONFIG_MQTT_LIB_TLS)
	k_sem_give(&mqtt->tls_hs_wait);
#endif
}

static
void async_recv_cb(int sock, void *data, size_t bytes_received, void *cb_data)
{
	struct mqtt_ctx *mqtt = (struct mqtt_ctx *)cb_data;

	ARG_UNUSED(sock);

	if (!data || bytes_received <= 0) {
		return;
	}

	mqtt->rcv(mqtt, data, bytes_received);
}

int mqtt_connect(struct mqtt_ctx *ctx)
{
	int rc = 0;
	struct sockaddr addr;
	struct sockaddr peer_addr;

	if (!ctx) {
		return -EFAULT;
	}

	/* Duplicate some functionality of net_app_init_tcp_client:
	 *  - Parse peer_addr_str, peer_port
	 *  - Create the socket for this context.
	 *  - TBD: Setup TLS thread
	 */
	memset(&peer_addr, 0, sizeof(peer_addr));
	memset(&addr, 0, sizeof(addr));
	rc = net_util_init_tcp_client(&addr, &peer_addr,
				      ctx->peer_addr_str, ctx->peer_port);
	if (!rc) {
		goto error_connect;
	}

	/* Create the socket: */
	ctx->sock = async_socket(addr.sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (ctx->sock == INVALID_SOCK) {
		rc = -errno;
		goto error_connect;
	}

	zstream_sock_init(&ctx->stream_sock, ctx->sock);
	ctx->stream = (zstream)&ctx->stream_sock;

	/*
	 * Setup receive callback, which will call mqtt_parser() on a received
	 * socket buffer:
	 */
	rc = async_recv(ctx->sock, ctx->stream, ctx->rcv_buf, MSG_SIZE,
			async_recv_cb, (void *)ctx);
	if (rc < 0) {
		goto error_connect;
	}

#if defined(CONFIG_MQTT_LIB_TLS)
	rc = net_app_client_tls(&ctx->net_app_ctx,
			ctx->request_buf,
			ctx->request_buf_len,
			ctx->personalization_data,
			ctx->personalization_data_len,
			ctx->cert_cb,
			ctx->cert_host,
			ctx->entropy_src_cb,
			ctx->tls_mem_pool,
			ctx->tls_stack,
			ctx->tls_stack_size);
	if (rc < 0) {
		goto error_connect;
	}
#endif

	rc = async_connect(ctx->sock, &peer_addr, sizeof(peer_addr),
			   async_connected_cb, (void *)ctx);
	if (rc < 0) {
		goto error_connect;
	}

#if defined(CONFIG_MQTT_LIB_TLS)
	/* TLS handshake is not finished until app_connected is called */
	rc = k_sem_take(&ctx->tls_hs_wait, ctx->tls_hs_timeout);
	if (rc < 0) {
		goto error_connect;
	}
#endif

	return rc;

error_connect:
	mqtt_close(ctx);

	return rc;
}

int mqtt_init(struct mqtt_ctx *ctx, enum mqtt_app app_type)
{
	/* So far, only clean session = 1 is supported */
	ctx->clean_session = 1;
	ctx->connected = 0;

	ctx->app_type = app_type;
	ctx->rcv = mqtt_parser;

	ctx->sock = INVALID_SOCK;

#if defined(CONFIG_MQTT_LIB_TLS)
	if (ctx->tls_hs_timeout == 0) {
		ctx->tls_hs_timeout = TLS_HS_DEFAULT_TIMEOUT;
	}

	k_sem_init(&ctx->tls_hs_wait, 0, 1);
#endif

	ctx->rcv_buf = _mqtt_msg_alloc();

	return (ctx->rcv_buf != NULL ? 0 : -ENOMEM);
}

int mqtt_close(struct mqtt_ctx *ctx)
{
	if (!ctx) {
		return -EFAULT;
	}

	if (ctx->sock != INVALID_SOCK) {
		async_close(ctx->sock, ctx->stream);
		ctx->sock = INVALID_SOCK;
	}

	if (ctx->rcv_buf) {
		_mqtt_msg_free(ctx->rcv_buf);
	}

	return 0;
}
