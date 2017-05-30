/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <net/mqtt.h>

#include <net/net_context.h>

#include <misc/printk.h>
#include <string.h>
#include <errno.h>

#if defined(CONFIG_NET_L2_BLUETOOTH)
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <gatt/ipss.h>
#endif

#include "config.h"

#define CONN_TRIES 20

/* Container for some structures used by the MQTT publisher app. */
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

	printk("[%s:%d]", __func__, __LINE__);

	if (client_ctx->connect_data) {
		printk(" user_data: %s",
		       (const char *)client_ctx->connect_data);
	}

	printk("\n");
}

/* The signature of this routine must match the disconnect callback declared at
 * the mqtt.h header.
 */
static void disconnect_cb(struct mqtt_ctx *mqtt_ctx)
{
	struct mqtt_client_ctx *client_ctx;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);

	printk("[%s:%d]", __func__, __LINE__);

	if (client_ctx->disconnect_data) {
		printk(" user_data: %s",
		       (const char *)client_ctx->disconnect_data);
	}

	printk("\n");
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

	printk("[%s:%d] <%s> packet id: %u", __func__, __LINE__, str, pkt_id);

	if (client_ctx->publish_data) {
		printk(", user_data: %s",
		       (const char *)client_ctx->publish_data);
	}

	printk("\n");

	return rc;
}

/**
 * The signature of this routine must match the malformed callback declared at
 * the mqtt.h header.
 */
static void malformed_cb(struct mqtt_ctx *mqtt_ctx, u16_t pkt_type)
{
	printk("[%s:%d] pkt_type: %u\n", __func__, __LINE__, pkt_type);
}

static char *get_mqtt_payload(enum mqtt_qos qos)
{
#if APP_BLUEMIX_TOPIC
	static char payload[30];

	snprintk(payload, sizeof(payload), "{d:{temperature:%d}}",
		 (u8_t)sys_rand32_get());
#else
	static char payload[] = "DOORS:OPEN_QoSx";

	payload[strlen(payload) - 1] = '0' + qos;
#endif

	return payload;
}

static char *get_mqtt_topic(void)
{
#if APP_BLUEMIX_TOPIC
	return "iot-2/type/"BLUEMIX_DEVTYPE"/id/"BLUEMIX_DEVID
	       "/evt/"BLUEMIX_EVENT"/fmt/"BLUEMIX_FORMAT;
#else
	return "sensors";
#endif
}

static void prepare_mqtt_publish_msg(struct mqtt_publish_msg *pub_msg,
				     enum mqtt_qos qos)
{
	/* MQTT message payload may be anything, we we use C strings */
	pub_msg->msg = get_mqtt_payload(qos);
	/* Payload's length */
	pub_msg->msg_len = strlen(client_ctx.pub_msg.msg);
	/* MQTT Quality of Service */
	pub_msg->qos = qos;
	/* Message's topic */
	pub_msg->topic = get_mqtt_topic();
	pub_msg->topic_len = strlen(client_ctx.pub_msg.topic);
	/* Packet Identifier, always use different values */
	pub_msg->pkt_id = sys_rand32_get();
}

#define RC_STR(rc)	((rc) == 0 ? "OK" : "ERROR")

#define PRINT_RESULT(func, rc)	\
	printk("[%s:%d] %s: %d <%s>\n", __func__, __LINE__, \
	       (func), rc, RC_STR(rc))

/* In this routine we block until the connected variable is 1 */
static int try_to_connect(struct mqtt_client_ctx *client_ctx)
{
	int i = 0;

	while (i++ < APP_CONNECT_TRIES && !client_ctx->mqtt_ctx.connected) {
		int rc;

		rc = mqtt_tx_connect(&client_ctx->mqtt_ctx,
				     &client_ctx->connect_msg);
		k_sleep(APP_SLEEP_MSECS);
		PRINT_RESULT("mqtt_tx_connect", rc);
		if (rc != 0) {
			continue;
		}
	}

	if (client_ctx->mqtt_ctx.connected) {
		return 0;
	}

	return -EINVAL;
}

static void publisher(void)
{
	int i, rc;

	/* The net_ctx variable must be ready BEFORE passing it to the MQTT API.
	 */
	for (i = 0; i < CONN_TRIES; i++) {
		rc = network_setup(&net_ctx, ZEPHYR_ADDR, SERVER_ADDR,
				   SERVER_PORT);
		if (!rc) {
			goto connected;
		}
	}

	PRINT_RESULT("network_setup", rc);
	goto exit_app;

connected:
	/* Set everything to 0 and later just assign the required fields. */
	memset(&client_ctx, 0x00, sizeof(client_ctx));

	/* The network context is the only field that must be set BEFORE
	 * calling the mqtt_init routine.
	 */
	client_ctx.mqtt_ctx.net_ctx = net_ctx;

	/* connect, disconnect and malformed may be set to NULL */
	client_ctx.mqtt_ctx.connect = connect_cb;

	client_ctx.mqtt_ctx.disconnect = disconnect_cb;
	client_ctx.mqtt_ctx.malformed = malformed_cb;

	client_ctx.mqtt_ctx.net_timeout = APP_TX_RX_TIMEOUT;

	/* Publisher apps TX the MQTT PUBLISH msg */
	client_ctx.mqtt_ctx.publish_tx = publish_cb;

	rc = mqtt_init(&client_ctx.mqtt_ctx, MQTT_APP_PUBLISHER);
	PRINT_RESULT("mqtt_init", rc);
	if (rc != 0) {
		goto exit_app;
	}

	/* The connect message will be sent to the MQTT server (broker).
	 * If clean_session here is 0, the mqtt_ctx clean_session variable
	 * will be set to 0 also. Please don't do that, set always to 1.
	 * Clean session = 0 is not yet supported.
	 */
	client_ctx.connect_msg.client_id = MQTT_CLIENTID;
	client_ctx.connect_msg.client_id_len = strlen(MQTT_CLIENTID);
	client_ctx.connect_msg.clean_session = 1;

	client_ctx.connect_data = "CONNECTED";
	client_ctx.disconnect_data = "DISCONNECTED";
	client_ctx.publish_data = "PUBLISH";

	rc = try_to_connect(&client_ctx);
	PRINT_RESULT("try_to_connect", rc);
	if (rc != 0) {
		goto exit_app;
	}

	i = 0;
	while (i++ < APP_MAX_ITERATIONS) {
		rc = mqtt_tx_pingreq(&client_ctx.mqtt_ctx);
		k_sleep(APP_SLEEP_MSECS);
		PRINT_RESULT("mqtt_tx_pingreq", rc);

		prepare_mqtt_publish_msg(&client_ctx.pub_msg, MQTT_QoS0);
		rc = mqtt_tx_publish(&client_ctx.mqtt_ctx, &client_ctx.pub_msg);
		k_sleep(APP_SLEEP_MSECS);
		PRINT_RESULT("mqtt_tx_publish", rc);

		prepare_mqtt_publish_msg(&client_ctx.pub_msg, MQTT_QoS1);
		rc = mqtt_tx_publish(&client_ctx.mqtt_ctx, &client_ctx.pub_msg);
		k_sleep(APP_SLEEP_MSECS);
		PRINT_RESULT("mqtt_tx_publish", rc);

		prepare_mqtt_publish_msg(&client_ctx.pub_msg, MQTT_QoS2);
		rc = mqtt_tx_publish(&client_ctx.mqtt_ctx, &client_ctx.pub_msg);
		k_sleep(APP_SLEEP_MSECS);
		PRINT_RESULT("mqtt_tx_publish", rc);
	}

	rc = mqtt_tx_disconnect(&client_ctx.mqtt_ctx);
	PRINT_RESULT("mqtt_tx_disconnect", rc);

exit_app:
	net_context_put(net_ctx);
	printk("\nBye!\n");
}

static int set_addr(struct sockaddr *sock_addr, const char *addr, u16_t port)
{
	void *ptr;
	int rc;

#ifdef CONFIG_NET_IPV6
	net_sin6(sock_addr)->sin6_port = htons(port);
	sock_addr->family = AF_INET6;
	ptr = &(net_sin6(sock_addr)->sin6_addr);
	rc = net_addr_pton(AF_INET6, addr, ptr);
#else
	net_sin(sock_addr)->sin_port = htons(port);
	sock_addr->family = AF_INET;
	ptr = &(net_sin(sock_addr)->sin_addr);
	rc = net_addr_pton(AF_INET, addr, ptr);
#endif

	if (rc) {
		printk("Invalid IP address: %s\n", addr);
	}

	return rc;
}

#if defined(CONFIG_NET_L2_BLUETOOTH)
static bool bt_connected;

static
void bt_connect_cb(struct bt_conn *conn, u8_t err)
{
	bt_connected = true;
}

static
void bt_disconnect_cb(struct bt_conn *conn, u8_t reason)
{
	bt_connected = false;
	printk("bt disconnected (reason %u)\n", reason);
}

static
struct bt_conn_cb bt_conn_cb = {
	.connected = bt_connect_cb,
	.disconnected = bt_disconnect_cb,
};
#endif

static int network_setup(struct net_context **net_ctx, const char *local_addr,
			 const char *server_addr, u16_t server_port)
{
#ifdef CONFIG_NET_IPV6
	socklen_t addr_len = sizeof(struct sockaddr_in6);
	sa_family_t family = AF_INET6;

#else
	socklen_t addr_len = sizeof(struct sockaddr_in);
	sa_family_t family = AF_INET;
#endif
	struct sockaddr server_sock, local_sock;
	void *p;
	int rc;

#if defined(CONFIG_NET_L2_BLUETOOTH)
	const char *progress_mark = "/-\\|";
	int i = 0;

	rc = bt_enable(NULL);
	if (rc) {
		printk("bluetooth init failed\n");
		return rc;
	}

	ipss_init();
	bt_conn_cb_register(&bt_conn_cb);
	rc = ipss_advertise();
	if (rc) {
		printk("advertising failed to start\n");
		return rc;
	}

	printk("\nwaiting for bt connection: ");
	while (bt_connected == false) {
		k_sleep(250);
		printk("%c\b", progress_mark[i]);
		i = (i + 1) % (sizeof(progress_mark) - 1);
	}
	printk("\n");
#endif

	rc = set_addr(&local_sock, local_addr, 0);
	if (rc) {
		printk("set_addr (local) error\n");
		return rc;
	}

#ifdef CONFIG_NET_IPV6
	p = net_if_ipv6_addr_add(net_if_get_default(),
				 &net_sin6(&local_sock)->sin6_addr,
				 NET_ADDR_MANUAL, 0);
#else
	p = net_if_ipv4_addr_add(net_if_get_default(),
				 &net_sin(&local_sock)->sin_addr,
				 NET_ADDR_MANUAL, 0);
#endif

	if (!p) {
		return -EINVAL;
	}

	rc = net_context_get(family, SOCK_STREAM, IPPROTO_TCP, net_ctx);
	if (rc) {
		printk("net_context_get error\n");
		return rc;
	}

	rc = net_context_bind(*net_ctx, &local_sock, addr_len);
	if (rc) {
		printk("net_context_bind error\n");
		goto lb_exit;
	}

	rc = set_addr(&server_sock, server_addr, server_port);
	if (rc) {
		printk("set_addr (server) error\n");
		goto lb_exit;
	}

	rc = net_context_connect(*net_ctx, &server_sock, addr_len, NULL,
				 APP_SLEEP_MSECS, NULL);
	if (rc) {
		printk("net_context_connect error\n");
		goto lb_exit;
	}

	return 0;

lb_exit:
	net_context_put(*net_ctx);

	return rc;
}

void main(void)
{
	publisher();
}
