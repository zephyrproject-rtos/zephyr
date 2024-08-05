/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, LOG_LEVEL_WRN);

#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#include <string.h>
#include <errno.h>

#include <zephyr/random/random.h>

#include "config.h"

/* This is mqtt payload message. */
char payload[] = "DOORS:OPEN_QoSx";

#define BUFFER_SIZE 128

static uint8_t rx_buffer[BUFFER_SIZE];
static uint8_t tx_buffer[BUFFER_SIZE];
static struct mqtt_client client_ctx;
static struct sockaddr broker;
static struct zsock_pollfd fds[1];
static int nfds;
static bool connected;

static void broker_init(void)
{
#if defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 *broker6 = net_sin6(&broker);

	broker6->sin6_family = AF_INET6;
	broker6->sin6_port = htons(SERVER_PORT);
	zsock_inet_pton(AF_INET6, SERVER_ADDR, &broker6->sin6_addr);
#else
	struct sockaddr_in *broker4 = net_sin(&broker);

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(SERVER_PORT);
	zsock_inet_pton(AF_INET, SERVER_ADDR, &broker4->sin_addr);
#endif
}

static void prepare_fds(struct mqtt_client *client)
{
	if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds[0].fd = client->transport.tcp.sock;
	}

	fds[0].events = ZSOCK_POLLIN;
	nfds = 1;
}

static void clear_fds(void)
{
	nfds = 0;
}

static void wait(int timeout)
{
	if (nfds > 0) {
		if (zsock_poll(fds, nfds, timeout) < 0) {
			TC_PRINT("poll error: %d\n", errno);
		}
	}
}

void mqtt_evt_handler(struct mqtt_client *const client,
		      const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			TC_PRINT("MQTT connect failed %d\n", evt->result);
			break;
		}

		connected = true;
		TC_PRINT("[%s:%d] MQTT_EVT_CONNACK: Connected!\n",
			 __func__, __LINE__);

		break;

	case MQTT_EVT_DISCONNECT:
		TC_PRINT("[%s:%d] MQTT_EVT_DISCONNECT: disconnected %d\n",
			 __func__, __LINE__, evt->result);

		connected = false;
		clear_fds();

		break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			TC_PRINT("MQTT PUBACK error %d\n", evt->result);
			break;
		}

		TC_PRINT("[%s:%d] MQTT_EVT_PUBACK packet id: %u\n",
			 __func__, __LINE__, evt->param.puback.message_id);

		break;

	case MQTT_EVT_PUBREC:
		if (evt->result != 0) {
			TC_PRINT("MQTT PUBREC error %d\n", evt->result);
			break;
		}

		TC_PRINT("[%s:%d] MQTT_EVT_PUBREC packet id: %u\n",
			 __func__, __LINE__, evt->param.pubrec.message_id);

		const struct mqtt_pubrel_param rel_param = {
			.message_id = evt->param.pubrec.message_id
		};

		err = mqtt_publish_qos2_release(client, &rel_param);
		if (err != 0) {
			TC_PRINT("Failed to send MQTT PUBREL: %d\n",
				 err);
		}

		break;

	case MQTT_EVT_PUBCOMP:
		if (evt->result != 0) {
			TC_PRINT("MQTT PUBCOMP error %d\n", evt->result);
			break;
		}

		TC_PRINT("[%s:%d] MQTT_EVT_PUBCOMP packet id: %u\n",
			 __func__, __LINE__, evt->param.pubcomp.message_id);

		break;

	default:
		TC_PRINT("[%s:%d] Invalid MQTT packet\n", __func__, __LINE__);
		break;
	}
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

static void client_init(struct mqtt_client *client)
{
	mqtt_client_init(client);

	broker_init();

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_evt_handler;
	client->client_id.utf8 = (uint8_t *)MQTT_CLIENTID;
	client->client_id.size = strlen(MQTT_CLIENTID);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;
	client->transport.type = MQTT_TRANSPORT_NON_SECURE;

	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);
}

static int publish(enum mqtt_qos qos)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = (uint8_t *)get_mqtt_topic();
	param.message.topic.topic.size =
			strlen(param.message.topic.topic.utf8);
	param.message.payload.data = get_mqtt_payload(qos);
	param.message.payload.len =
			strlen(param.message.payload.data);
	param.message_id = sys_rand16_get();
	param.dup_flag = 0U;
	param.retain_flag = 0U;

	return mqtt_publish(&client_ctx, &param);
}

/* In this routine we block until the connected variable is 1 */
static int try_to_connect(struct mqtt_client *client)
{
	int rc, i = 0;

	while (i++ < APP_CONNECT_TRIES && !connected) {

		client_init(&client_ctx);

		rc = mqtt_connect(client);
		if (rc != 0) {
			k_sleep(K_MSEC(APP_SLEEP_MSECS));
			continue;
		}

		prepare_fds(client);

		wait(APP_SLEEP_MSECS);
		mqtt_input(client);

		if (!connected) {
			mqtt_abort(client);
		}
	}

	if (connected) {
		return 0;
	}

	return -EINVAL;
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

	rc = mqtt_ping(&client_ctx);
	if (rc != 0) {
		return TC_FAIL;
	}

	wait(APP_SLEEP_MSECS);
	mqtt_input(&client_ctx);

	return TC_PASS;
}

static int test_publish(enum mqtt_qos qos)
{
	int rc;

	rc = publish(qos);
	if (rc != 0) {
		return TC_FAIL;
	}

	wait(APP_SLEEP_MSECS);
	mqtt_input(&client_ctx);

	/* Second input handle for expected Publish Complete response. */
	if (qos == MQTT_QOS_2_EXACTLY_ONCE) {
		wait(APP_SLEEP_MSECS);
		mqtt_input(&client_ctx);
	}

	return TC_PASS;
}

static int test_disconnect(void)
{
	int rc;

	rc = mqtt_disconnect(&client_ctx);
	if (rc != 0) {
		return TC_FAIL;
	}

	wait(APP_SLEEP_MSECS);

	return TC_PASS;
}

void test_mqtt_connect(void)
{
	zassert_true(test_connect() == TC_PASS);
}

void test_mqtt_pingreq(void)
{
	zassert_true(test_pingreq() == TC_PASS);
}

void test_mqtt_publish(void)
{
	zassert_true(test_publish(MQTT_QOS_0_AT_MOST_ONCE) == TC_PASS);
	zassert_true(test_publish(MQTT_QOS_1_AT_LEAST_ONCE) == TC_PASS);
	zassert_true(test_publish(MQTT_QOS_2_EXACTLY_ONCE) == TC_PASS);
}

void test_mqtt_disconnect(void)
{
	zassert_true(test_disconnect() == TC_PASS);
}
