/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the MQTT client conformance suite.
 *
 * An ordinary publisher: connect to the broker, publish to one topic every
 * couple of seconds, and keep the connection alive in between. When the
 * connection goes, start again. Nothing here exists for the test.
 *
 * Starting again is what lets a suite of independent test cases work. Each one
 * takes the connection the client makes to it, drives the part of the exchange
 * it is about, and closes; the client notices and connects afresh for the next.
 *
 * The keep alive is short so that a test that waits for a ping does not wait a
 * minute for it, and the publish interval is longer still so that there is
 * silence for a ping to fall due in.
 *
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/mqtt.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mqtt_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>

#define BROKER_PORT	1883
#define CLIENT_ID	"zephyr-conformance"
#define TOPIC		"zephyr/conformance"
#define PAYLOAD		"mqtt"

/* Longer than the keep alive, so that the connection is genuinely idle
 * between publishes and a ping falls due. A client that published faster than
 * it pings would never have anything to prove it was alive with.
 */
#define PUBLISH_INTERVAL_MS	6000
#define RECONNECT_INTERVAL	K_SECONDS(1)
#define POLL_TIMEOUT		500

static struct mqtt_client client;
static struct net_sockaddr_in broker;
static uint8_t rx_buffer[256];
static uint8_t tx_buffer[256];
static bool connected;

static void evt_handler(struct mqtt_client *c, const struct mqtt_evt *evt)
{
	ARG_UNUSED(c);

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result == 0) {
			connected = true;
		}
		break;
	case MQTT_EVT_DISCONNECT:
		connected = false;
		break;
	default:
		break;
	}
}

static void client_setup(void)
{
	mqtt_client_init(&client);

	client.broker = &broker;
	client.evt_cb = evt_handler;
	client.client_id.utf8 = (uint8_t *)CLIENT_ID;
	client.client_id.size = sizeof(CLIENT_ID) - 1;
	client.password = NULL;
	client.user_name = NULL;
	client.protocol_version = MQTT_VERSION_3_1_1;
	client.rx_buf = rx_buffer;
	client.rx_buf_size = sizeof(rx_buffer);
	client.tx_buf = tx_buffer;
	client.tx_buf_size = sizeof(tx_buffer);
	client.transport.type = MQTT_TRANSPORT_NON_SECURE;
}

static int publish_once(void)
{
	static uint16_t message_id = 1;
	struct mqtt_publish_param param = { 0 };

	param.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE;
	param.message.topic.topic.utf8 = (uint8_t *)TOPIC;
	param.message.topic.topic.size = sizeof(TOPIC) - 1;
	param.message.payload.data = (uint8_t *)PAYLOAD;
	param.message.payload.len = sizeof(PAYLOAD) - 1;
	param.message_id = message_id++;
	param.dup_flag = 0;
	param.retain_flag = 0;

	return mqtt_publish(&client, &param);
}

/* Read whatever the broker has sent and let the client send what it owes,
 * which is how a ping gets sent and how a retransmission gets timed.
 */
static int service(void)
{
	struct zsock_pollfd fds = {
		.fd = client.transport.tcp.sock,
		.events = ZSOCK_POLLIN,
	};
	int ret;

	ret = zsock_poll(&fds, 1, POLL_TIMEOUT);
	if (ret < 0) {
		return -errno;
	}

	if (ret > 0 && (fds.revents & ZSOCK_POLLIN) != 0) {
		ret = mqtt_input(&client);
		if (ret < 0) {
			return ret;
		}
	}

	return mqtt_live(&client);
}

static void session(void)
{
	int64_t next_publish;
	int ret;

	client_setup();

	if (mqtt_connect(&client) < 0) {
		return;
	}

	next_publish = k_uptime_get() + PUBLISH_INTERVAL_MS;

	while (true) {
		ret = service();
		if (ret < 0 && ret != -EAGAIN) {
			break;
		}

		if (connected && k_uptime_get() >= next_publish) {
			if (publish_once() < 0) {
				break;
			}

			next_publish = k_uptime_get() + PUBLISH_INTERVAL_MS;
		}
	}

	(void)mqtt_abort(&client);
	connected = false;
}

int main(void)
{
	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	broker.sin_family = NET_AF_INET;
	broker.sin_port = net_htons(BROKER_PORT);

	if (net_addr_pton(NET_AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
			  &broker.sin_addr) < 0) {
		LOG_ERR("Cannot parse the broker address");
		return -EINVAL;
	}

	LOG_INF("MQTT client ready");

	while (true) {
		session();
		k_sleep(RECONNECT_INTERVAL);
	}

	return 0;
}
