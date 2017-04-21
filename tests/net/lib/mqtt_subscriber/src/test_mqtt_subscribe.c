/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <net/mqtt.h>

#include <net/net_context.h>
#include <net/net_pkt.h>

#include <misc/printk.h>
#include <string.h>
#include <errno.h>

#include "config.h"

/* Container for some structures used by the MQTT subscriber app. */
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
	 * This variable will be passed to the subscribe_tx callback, declared
	 * inside the mqtt context struct. If not used, it could be set to NULL.
	 */
	void *subscribe_data;

	/**
	 * This variable will be passed to the unsubscribe_tx callback, declared
	 * inside the mqtt context struct. If not used, it could be set to NULL.
	 */
	void *unsubscribe_data;
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
 * The signature of this routine must match the publish_rx callback declared at
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
static int publish_rx_cb(struct mqtt_ctx *mqtt_ctx, struct mqtt_publish_msg
		*msg, u16_t pkt_id, enum mqtt_packet type)
{
	struct mqtt_client_ctx *client_ctx;
	const char *str;
	int rc = 0;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);

	switch (type) {
	case MQTT_PUBLISH:
		str = "MQTT_PUBLISH";
		printk("[%s:%d] <%s> msg: %s", __func__, __LINE__,
				str, msg->msg);
		break;
	case MQTT_PUBREL:
		str = "MQTT_PUBREL";
		printk("[%s:%d] <%s> packet id: %u", __func__, __LINE__,
				str, pkt_id);
		return 0;
	default:
		rc = -EINVAL;
		str = "Invalid MQTT packet";
	}

	if (client_ctx->subscribe_data) {
		printk(", user_data: %s",
		       (const char *)client_ctx->subscribe_data);
	}

	printk("\n");

	return rc;
}

/**
 * The signature of this routine must match the subscribe callback declared at
 * the mqtt.h header.
 */
static int subscriber_cb(struct mqtt_ctx *mqtt_ctx, u16_t pkt_id,
		u8_t items, enum mqtt_qos qos[])
{
	struct mqtt_client_ctx *client_ctx;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);

	printk("[%s:%d] items: %d packet id: %u", __func__, __LINE__,
			items, pkt_id);

	if (client_ctx->subscribe_data) {
		printk(" user_data: %s",
				(const char *)client_ctx->subscribe_data);
	}

	printk("\n");

	return 0;
}

/**
 * The signature of this routine must match the unsubscribe callback declared at
 * the mqtt.h header.
 */
static int unsubscribe_cb(struct mqtt_ctx *mqtt_ctx, u16_t pkt_id)
{
	struct mqtt_client_ctx *client_ctx;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);

	printk("[%s:%d] packet id: %u", __func__, __LINE__, pkt_id);

	if (client_ctx->unsubscribe_data) {
		printk(" user_data: %s",
				(const char *)client_ctx->unsubscribe_data);
	}

	printk("\n");

	return 0;
}

/**
 * The signature of this routine must match the malformed callback declared at
 * the mqtt.h header.
 */
static void malformed_cb(struct mqtt_ctx *mqtt_ctx, u16_t pkt_type)
{
	printk("[%s:%d] pkt_type: %u\n", __func__, __LINE__, pkt_type);
}

static char *get_mqtt_topic(void)
{
	return "sensors";
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
		if (rc != 0) {
			continue;
		}
	}

	if (client_ctx->mqtt_ctx.connected) {
		return TC_PASS;
	}

	return TC_FAIL;
}

static int init_network(void)
{
	int rc;

	/* The net_ctx variable must be ready BEFORE passing it to the MQTT API.
	 */
	rc = network_setup(&net_ctx, ZEPHYR_ADDR, SERVER_ADDR, SERVER_PORT);
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
	client_ctx.mqtt_ctx.connect = connect_cb;

	client_ctx.mqtt_ctx.disconnect = disconnect_cb;

	client_ctx.mqtt_ctx.malformed = malformed_cb;

	client_ctx.mqtt_ctx.subscribe = subscriber_cb;

	client_ctx.mqtt_ctx.unsubscribe = unsubscribe_cb;

	client_ctx.mqtt_ctx.net_timeout = APP_TX_RX_TIMEOUT;

	/* Publisher apps TX the MQTT PUBLISH msg */
	client_ctx.mqtt_ctx.publish_rx = publish_rx_cb;

	rc = mqtt_init(&client_ctx.mqtt_ctx, MQTT_APP_SUBSCRIBER);
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
	client_ctx.subscribe_data = "SUBSCRIBE";
	client_ctx.unsubscribe_data = "UNSUBSCRIBE";

	return TC_PASS;

exit_app:
	net_context_put(net_ctx);

	return TC_FAIL;
}

static int test_connect(void)
{
	int rc;

	rc = try_to_connect(&client_ctx);
	if (rc != 0) {
		return TC_FAIL;
	}

	return TC_PASS;
}

static int test_subscribe(void)
{
	int rc;
	const char *topic_sub = get_mqtt_topic();
	u16_t pkt_id_sub = sys_rand32_get();
	static enum mqtt_qos  mqtt_qos_sub[1];

	rc = mqtt_tx_subscribe(&client_ctx.mqtt_ctx, pkt_id_sub, 1,
			&topic_sub, mqtt_qos_sub);
	k_sleep(APP_SLEEP_MSECS);
	if (rc != 0) {
		return TC_FAIL;
	}

	return TC_PASS;
}

static int test_unsubscribe(void)
{
	int rc;
	const char *topic_sub = get_mqtt_topic();
	u16_t pkt_id_unsub = sys_rand32_get();

	rc = mqtt_tx_unsubscribe(&client_ctx.mqtt_ctx, pkt_id_unsub,
			1, &topic_sub);
	k_sleep(APP_SLEEP_MSECS);
	if (rc != 0) {
		return TC_FAIL;
	}

	return TC_PASS;
}

static int test_disconnect(void)
{
	int rc;

	rc = mqtt_tx_disconnect(&client_ctx.mqtt_ctx);
	if (rc != 0) {
		return TC_FAIL;
	}

	return TC_PASS;
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
		printk("net_context_connect error\n"
				"Is the server (broker) up and running?\n");
		goto lb_exit;
	}

	return 0;

lb_exit:
	net_context_put(*net_ctx);

	return rc;
}

void test_mqtt_init(void)
{
	zassert_true(init_network() == TC_PASS, NULL);
}

void test_mqtt_connect(void)
{
	zassert_true(test_connect() == TC_PASS, NULL);
}

void test_mqtt_subscribe(void)
{
	zassert_true(test_subscribe() == TC_PASS, NULL);
}

void test_mqtt_unsubscribe(void)
{
	zassert_true(test_unsubscribe() == TC_PASS, NULL);
}

void test_mqtt_disconnect(void)
{
	zassert_true(test_disconnect() == TC_PASS, NULL);
}
