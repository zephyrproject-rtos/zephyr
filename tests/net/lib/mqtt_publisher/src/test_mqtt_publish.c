/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/mqtt.h>
#include <ztest.h>

#include <net/net_context.h>
#include <net/net_pkt.h>
#include <net/net_app.h>

#include <string.h>
#include <errno.h>

#include "config.h"

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

/* This is mqtt payload message. */
char payload[] = "DOORS:OPEN_QoSx";

/* The mqtt client struct */
static struct mqtt_client_ctx client_ctx;

/* The signature of this routine must match the connect callback declared at
 * the mqtt.h header.
 */
static void connect_cb(struct mqtt_ctx *mqtt_ctx)
{
	struct mqtt_client_ctx *client_ctx;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);

	TC_PRINT("[%s:%d]", __func__, __LINE__);

	if (client_ctx->connect_data) {
		TC_PRINT(" user_data: %s",
				(const char *)client_ctx->connect_data);
	}

	TC_PRINT("\n");
}

/* The signature of this routine must match the disconnect callback declared at
 * the mqtt.h header.
 */
static void disconnect_cb(struct mqtt_ctx *mqtt_ctx)
{
	struct mqtt_client_ctx *client_ctx;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);

	TC_PRINT("[%s:%d]", __func__, __LINE__);

	if (client_ctx->disconnect_data) {
		TC_PRINT(" user_data: %s",
				(const char *)client_ctx->disconnect_data);
	}

	TC_PRINT("\n");
}

/**
 * The signature of this routine must match the publish_tx callback declared at
 * the mqtt.h header.
 *
 * NOTE: we have two callbacks for MQTT Publish related stuff:
 *  - publish_tx, for publishers
 *  - publish_rx, for subscribers
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

	TC_PRINT("[%s:%d] <%s> packet id: %u", __func__, __LINE__, str, pkt_id);

	if (client_ctx->publish_data) {
		TC_PRINT(", user_data: %s",
				(const char *)client_ctx->publish_data);
	}

	TC_PRINT("\n");

	return rc;
}

/**
 * The signature of this routine must match the malformed callback declared at
 * the mqtt.h header.
 */
static void malformed_cb(struct mqtt_ctx *mqtt_ctx, u16_t pkt_type)
{
	TC_PRINT("[%s:%d] pkt_type: %u\n", __func__, __LINE__, pkt_type);
}

static char *get_mqtt_payload(enum mqtt_qos qos)
{
	payload[strlen(payload) - 1] = '0' + qos;

	return payload;
}

static char *get_mqtt_topic(void)
{
	return "sensors";
}

static void prepare_mqtt_publish_msg(struct mqtt_publish_msg *pub_msg,
		enum mqtt_qos qos)
{
	/* MQTT message payload may be anything, we we use C strings */
	pub_msg->msg = (u8_t *)get_mqtt_payload(qos);
	/* Payload's length */
	pub_msg->msg_len = (u16_t)strlen((char *)client_ctx.pub_msg.msg);
	/* MQTT Quality of Service */
	pub_msg->qos = qos;
	/* Message's topic */
	pub_msg->topic = get_mqtt_topic();
	pub_msg->topic_len = strlen(client_ctx.pub_msg.topic);
	/* Packet Identifier, always use different values */
	pub_msg->pkt_id = sys_rand32_get();
}

#define RC_STR(rc) ((rc) == 0 ? "OK" : "ERROR")

#define PRINT_RESULT(func, rc) \
	TC_PRINT("[%s:%d] %s: %d <%s>\n", __func__, __LINE__, \
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

	/* Set everything to 0 and later just assign the required fields. */
	(void)memset(&client_ctx, 0x00, sizeof(client_ctx));

	/* connect, disconnect and malformed may be set to NULL */
	client_ctx.mqtt_ctx.connect = connect_cb;

	client_ctx.mqtt_ctx.disconnect = disconnect_cb;
	client_ctx.mqtt_ctx.malformed = malformed_cb;

	client_ctx.mqtt_ctx.net_init_timeout = APP_NET_INIT_TIMEOUT;
	client_ctx.mqtt_ctx.net_timeout = APP_TX_RX_TIMEOUT;

	client_ctx.mqtt_ctx.peer_addr_str = SERVER_ADDR;
	client_ctx.mqtt_ctx.peer_port = SERVER_PORT;

	/* Publisher apps TX the MQTT PUBLISH msg */
	client_ctx.mqtt_ctx.publish_tx = publish_cb;

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

	rc = mqtt_init(&client_ctx.mqtt_ctx, MQTT_APP_PUBLISHER);
	if (rc != 0) {
		goto exit_app;
	}

	rc = mqtt_connect(&client_ctx.mqtt_ctx);
	if (rc != 0) {
		goto exit_app;
	}

	return TC_PASS;

exit_app:
	mqtt_close(&client_ctx.mqtt_ctx);

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

static int test_pingreq(void)
{
	int rc;

	rc = mqtt_tx_pingreq(&client_ctx.mqtt_ctx);
	k_sleep(APP_SLEEP_MSECS);
	if (rc != 0) {
		return TC_FAIL;
	}

	return TC_PASS;
}

static int test_publish(enum mqtt_qos qos)
{
	int rc;

	prepare_mqtt_publish_msg(&client_ctx.pub_msg, qos);
	rc = mqtt_tx_publish(&client_ctx.mqtt_ctx, &client_ctx.pub_msg);
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

void test_mqtt_init(void)
{
	zassert_true(init_network() == TC_PASS, NULL);
}

void test_mqtt_connect(void)
{
	zassert_true(test_connect() == TC_PASS, NULL);
}

void test_mqtt_pingreq(void)
{
	zassert_true(test_pingreq() == TC_PASS, NULL);
}

void test_mqtt_publish(void)
{
	zassert_true(test_publish(MQTT_QoS0) == TC_PASS, NULL);
	zassert_true(test_publish(MQTT_QoS1) == TC_PASS, NULL);
	zassert_true(test_publish(MQTT_QoS2) == TC_PASS, NULL);
}

void test_mqtt_disconnect(void)
{
	zassert_true(test_disconnect() == TC_PASS, NULL);
}
