/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <net/mqtt.h>
#include <net/net_context.h>
#include <logging/sys_log.h>

#include "mqtt.h"

#define ZEPHYR_ADDR "0.0.0.0"

#define APP_SLEEP_MSECS 5000
#define APP_TX_RX_TIMEOUT 800
#define APP_CONNECT_TRIES 10

/**
 * @brief mqtt_client_ctx	Container of some structures used by the
 *				publisher app.
 */
struct mqtt_client_ctx {
	/**
	 * The connect message structure is only used during the connect
	 * stage. Developers must set some msg properties before calling the
	 * mqtt_tx_connect routine. See below.
	 */
	struct mqtt_connect_msg connect_msg;
	/**
	 * This is the message that will be received by the server
	 * (MQTT broker).
	 */
	struct mqtt_publish_msg pub_msg;

	/**
	 * This is the MQTT application context variable.
	 */
	struct mqtt_ctx mqtt_ctx;

	/**
	 * This variable will be passed to the connect callback, declared inside
	 * the mqtt context struct. If not used, it could be set to NULL.
	 */
	void *connect_data;

	/**
	 * This variable will be passed to the disconnect callback, declared
	 * inside the mqtt context struct. If not used, it could be set to NULL.
	 */
	void *disconnect_data;

	/**
	 * This variable will be passed to the publish_tx callback, declared
	 * inside the mqtt context struct. If not used, it could be set to NULL.
	 */
	void *publish_data;
};

/* This is the network context structure. */
static struct net_context *net_ctx;

/* The mqtt client struct */
static struct mqtt_client_ctx client_ctx;

/* Status of mqtt */
static int mqtt_status;

/* This routine sets some basic properties for the network context variable */
static int network_setup(struct net_context **net_ctx, const char *local_addr,
			 const char *server_addr, u16_t server_port);

/* The signature of this routine must match the connect callback declared at
 * the mqtt.h header.
 */
static void connect_cb(struct mqtt_ctx *mqtt_ctx)
{
	struct mqtt_client_ctx *client_ctx;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);

	SYS_LOG_INF("[%s:%d]", __func__, __LINE__);

	if (client_ctx->connect_data) {
		SYS_LOG_INF(" user_data: %s",
			    (const char *)client_ctx->connect_data);
	}

	mqtt_status = 1;
}

/* The signature of this routine must match the disconnect callback declared at
 * the mqtt.h header.
 */
static void disconnect_cb(struct mqtt_ctx *mqtt_ctx)
{
	struct mqtt_client_ctx *client_ctx;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);

	SYS_LOG_INF("[%s:%d]", __func__, __LINE__);

	if (client_ctx->disconnect_data) {
		SYS_LOG_INF(" user_data: %s",
			    (const char *)client_ctx->disconnect_data);
	}

	mqtt_status = 0;
}

/**
 * The signature of this routine must match the publish_tx callback declared at
 * the mqtt.h header.
 *
 * NOTE: we have two callbacks for MQTT Publish related stuff:
 *	- publish_tx, for publishers
 *	- publish_rx, for subscribers
 *
 * Applications must keep a "message database" with pkt_id's. So far, this is
 * not implemented here. For example, if we receive a PUBREC message with an
 * unknown pkt_id, this routine must return an error, for example -EINVAL or
 * any negative value.
 */
static int publish_cb(struct mqtt_ctx *mqtt_ctx, u16_t pkt_id,
		      enum mqtt_packet type)
{
	struct mqtt_client_ctx *client_ctx;
	const char *str;
	int rc = 0;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);

	switch (type) {
	case MQTT_PUBACK:
		str = "MQTT_PUBACK";
		break;
	case MQTT_PUBCOMP:
		str = "MQTT_PUBCOMP";
		break;
	case MQTT_PUBREC:
		str = "MQTT_PUBREC";
		break;
	default:
		rc = -EINVAL;
		str = "Invalid MQTT packet";
	}

	SYS_LOG_INF("[%s:%d] <%s> packet id: %u", __func__, __LINE__, str, pkt_id);

	if (client_ctx->publish_data) {
		SYS_LOG_INF(", user_data: %s",
			    (const char *)client_ctx->publish_data);
	}

	return rc;
}

/**
 * The signature of this routine must match the malformed callback declared at
 * the mqtt.h header.
 */
static void malformed_cb(struct mqtt_ctx *mqtt_ctx, u16_t pkt_type)
{
	SYS_LOG_INF("[%s:%d] pkt_type: %u\n", __func__, __LINE__, pkt_type);
}

/* In this routine we block until the connected variable is 1 */
static int try_to_connect(struct mqtt_client_ctx *client_ctx)
{

	int i = 0;

	while (i++ < APP_CONNECT_TRIES && !client_ctx->mqtt_ctx.connected) {
		int rc;

		rc = mqtt_tx_connect(&client_ctx->mqtt_ctx,
				     &client_ctx->connect_msg);
		k_sleep(APP_SLEEP_MSECS);
		SYS_LOG_INF("try_to_connect:mqtt_tx_connect return %d connected %d",
			    rc, client_ctx->mqtt_ctx.connected);
		if (rc != 0) {
			continue;
		}
	}

	if (client_ctx->mqtt_ctx.connected) {
		SYS_LOG_INF("try_to_connect:CONNECTED\n");
		return 0;
	}

	return -EINVAL;
}

int mqtt_pub_init(void *arg)
{
	struct mqtt_client_prm *client_prm = (struct mqtt_client_prm *)arg;
	int rc;

	SYS_LOG_INF("mqtt_pub_init");
	/* The net_ctx variable must be ready BEFORE passing it to the MQTT API.
	 */
	rc = network_setup(&net_ctx, ZEPHYR_ADDR,
			   (const char *)client_prm->server_addr, client_prm->server_port);
	SYS_LOG_INF("mqtt_pub_init: network_setup return %d", rc);
	if (rc != 0) {
		goto exit_app;
	}

	/* Set everything to 0 and later just assign the required fields. */
	memset(&client_ctx, 0x00, sizeof(client_ctx));

	/* The network context is the only field that must be set BEFORE
	 * calling the mqtt_init routine.
	 */
	client_ctx.mqtt_ctx.net_ctx = net_ctx;

	/* connect, disconnect and malformed may be set to NULL */
	client_ctx.mqtt_ctx.connect    = connect_cb;
	client_ctx.mqtt_ctx.disconnect = disconnect_cb;
	client_ctx.mqtt_ctx.malformed  = malformed_cb;

	client_ctx.mqtt_ctx.net_timeout = APP_TX_RX_TIMEOUT;

	/* Publisher apps TX the MQTT PUBLISH msg */
	client_ctx.mqtt_ctx.publish_tx = publish_cb;

	rc = mqtt_init(&client_ctx.mqtt_ctx, MQTT_APP_PUBLISHER);
	SYS_LOG_INF("mqtt_pub_init: mqtt_init() return %d", rc);
	if (rc != 0) {
		goto exit_app;
	}

	/* The connect message will be sent to the MQTT server (broker).
	 * If clean_session here is 0, the mqtt_ctx clean_session variable
	 * will be set to 0 also. Please don't do that, set always to 1.
	 * Clean session = 0 is not yet supported.
	 */
	client_ctx.connect_msg.clean_session = 1;
	client_ctx.connect_msg.client_id     = client_prm->client_id;
	client_ctx.connect_msg.client_id_len = strlen(client_prm->client_id);

	client_ctx.connect_msg.keep_alive     = 5;

	client_ctx.connect_msg.user_name      = client_prm->user_id;
	client_ctx.connect_msg.user_name_len  = strlen(client_prm->user_id);
	client_ctx.connect_msg.password       = client_prm->password;
	client_ctx.connect_msg.password_len   = strlen(client_prm->password);

	client_ctx.connect_data    = "CONNECTED";
	client_ctx.disconnect_data = "DISCONNECTED";
	client_ctx.publish_data    = "PUBLISH";

	rc = try_to_connect(&client_ctx);
	if (rc != 0) {
		goto exit_app;
	}

	SYS_LOG_INF("mqtt_pub_init:DONE!");
	return 0;

exit_app:
	SYS_LOG_ERR("mqtt_pub_init:Error!");
	return -1;
}

/**
 */
int mqtt_status_get(void)
{
	return mqtt_status;
}

/**
 *
 */
int mqtt_send(char *topic, char *payload)
{
	struct mqtt_publish_msg *pub_msg = &client_ctx.pub_msg;
	int rc;

	rc = mqtt_tx_pingreq(&client_ctx.mqtt_ctx);
	if (rc) {
		SYS_LOG_ERR("mqttSend:mqtt_tx_pingreq ERROR %d status = %d\n",
			    rc, mqtt_status);
		return rc;
	}

	/* MQTT Quality of Service */
	pub_msg->qos = MQTT_QoS0; /* TODO add global var or define */

	pub_msg->retain = 0;

	/* Packet Identifier, always use different values */
	pub_msg->pkt_id = sys_rand32_get();

	/* Message's topic */
	pub_msg->topic = topic;
	pub_msg->topic_len = strlen(pub_msg->topic);

	/* MQTT message payload */
	pub_msg->msg = payload;
	pub_msg->msg_len = strlen(pub_msg->msg);

	rc = mqtt_tx_publish(&client_ctx.mqtt_ctx, &client_ctx.pub_msg);
	if (rc) {
		SYS_LOG_ERR("mqtt_tx_publish ERROR %d!", rc);
		return rc;
	}

	return 0;
}

static int set_addr(struct sockaddr *sock_addr, const char *addr, u16_t port)
{
	int rc;

	net_sin(sock_addr)->sin_port = htons(port);
	sock_addr->family = AF_INET;

	rc = net_addr_pton(AF_INET, addr, &(net_sin(sock_addr)->sin_addr));
	if (rc) {
		SYS_LOG_ERR("Invalid IP address: %s", addr);
	}

	return rc;
}

static int network_setup(struct net_context **net_ctx, const char *local_addr,
			 const char *server_addr, u16_t server_port)
{
	socklen_t addr_len = sizeof(struct sockaddr_in);
	struct sockaddr server_sock, local_sock;
	int rc;
	int i;

	rc = set_addr(&local_sock, local_addr, 0);
	if (rc) {
		SYS_LOG_ERR("set_addr (local) error");
		return rc;
	}

	rc = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, net_ctx);
	if (rc) {
		SYS_LOG_ERR("net_context_get error %d", rc);
		return rc;
	}

	rc = net_context_bind(*net_ctx, &local_sock, addr_len);
	if (rc) {
		SYS_LOG_ERR("net_context_bind error %d", rc);
		goto lb_exit;
	}

	rc = set_addr(&server_sock, server_addr, server_port);
	if (rc) {
		SYS_LOG_ERR("set_addr (server) error %d", rc);
		goto lb_exit;
	}

	for (i = 0; i < 4; i++) {
		rc = net_context_connect(*net_ctx, &server_sock, addr_len, NULL,
					 APP_SLEEP_MSECS, NULL);
		SYS_LOG_ERR("net_context_connect return %d", rc);
		if (rc == 0) {
			break;
		}
	}
	if (i == 4) {
		SYS_LOG_ERR("net_context_connect error %d.\n"
			    "Is the server (broker) up and running?", rc);
		goto lb_exit;
	}

	return 0;

lb_exit:
	net_context_put(*net_ctx);

	return rc;
}
