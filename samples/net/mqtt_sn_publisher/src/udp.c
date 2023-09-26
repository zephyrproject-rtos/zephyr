/*
 * Copyright (c) 2022 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/mqtt_sn.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(mqtt_sn_publisher_sample);

static void process_thread(void);

K_THREAD_DEFINE(udp_thread_id, STACK_SIZE, process_thread, NULL, NULL, NULL, THREAD_PRIORITY,
		IS_ENABLED(CONFIG_USERSPACE) ? K_USER : 0, -1);

static APP_BMEM struct mqtt_sn_client mqtt_client;
static APP_BMEM struct mqtt_sn_transport_udp tp;
static APP_DMEM struct mqtt_sn_data client_id = MQTT_SN_DATA_STRING_LITERAL("ZEPHYR");

static APP_BMEM uint8_t tx_buf[CONFIG_NET_SAMPLE_MQTT_SN_BUFFER_SIZE];
static APP_BMEM uint8_t rx_buf[CONFIG_NET_SAMPLE_MQTT_SN_BUFFER_SIZE];

static APP_BMEM bool mqtt_sn_connected;

static void evt_cb(struct mqtt_sn_client *client, const struct mqtt_sn_evt *evt)
{
	switch (evt->type) {
	case MQTT_SN_EVT_CONNECTED: /* Connected to a gateway */
		LOG_INF("MQTT-SN event EVT_CONNECTED");
		mqtt_sn_connected = true;
		break;
	case MQTT_SN_EVT_DISCONNECTED: /* Disconnected */
		LOG_INF("MQTT-SN event EVT_DISCONNECTED");
		mqtt_sn_connected = false;
		break;
	case MQTT_SN_EVT_ASLEEP: /* Entered ASLEEP state */
		LOG_INF("MQTT-SN event EVT_ASLEEP");
		break;
	case MQTT_SN_EVT_AWAKE: /* Entered AWAKE state */
		LOG_INF("MQTT-SN event EVT_AWAKE");
		break;
	case MQTT_SN_EVT_PUBLISH: /* Received a PUBLISH message */
		LOG_INF("MQTT-SN event EVT_PUBLISH");
		LOG_HEXDUMP_INF(evt->param.publish.data.data, evt->param.publish.data.size,
				"Published data");
		break;
	case MQTT_SN_EVT_PINGRESP: /* Received a PINGRESP */
		LOG_INF("MQTT-SN event EVT_PINGRESP");
		break;
	}
}

static int do_work(void)
{
	static APP_BMEM bool subscribed;
	static APP_BMEM int64_t ts;
	static APP_DMEM struct mqtt_sn_data topic_p = MQTT_SN_DATA_STRING_LITERAL("/uptime");
	static APP_DMEM struct mqtt_sn_data topic_s = MQTT_SN_DATA_STRING_LITERAL("/number");
	char out[20];
	struct mqtt_sn_data pubdata = {.data = out};
	const int64_t now = k_uptime_get();
	int err;

	err = mqtt_sn_input(&mqtt_client);
	if (err < 0) {
		LOG_ERR("failed: input: %d", err);
		return err;
	}

	if (mqtt_sn_connected && !subscribed) {
		err = mqtt_sn_subscribe(&mqtt_client, MQTT_SN_QOS_0, &topic_s);
		if (err < 0) {
			return err;
		}
		subscribed = true;
	}

	if (now - ts > 10000 && mqtt_sn_connected) {
		LOG_INF("Publishing timestamp");

		ts = now;

		err = snprintk(out, sizeof(out), "%" PRIi64, ts);
		if (err < 0) {
			LOG_ERR("failed: snprintf");
			return err;
		}

		pubdata.size = MIN(sizeof(out), err);

		err = mqtt_sn_publish(&mqtt_client, MQTT_SN_QOS_0, &topic_p, false, &pubdata);
		if (err < 0) {
			LOG_ERR("failed: publish: %d", err);
			return err;
		}
	}

	return 0;
}

static void process_thread(void)
{
	struct sockaddr_in gateway = {0};
	int err;

	LOG_DBG("Parsing MQTT host IP " CONFIG_NET_SAMPLE_MQTT_SN_GATEWAY_IP);
	gateway.sin_family = AF_INET;
	gateway.sin_port = htons(CONFIG_NET_SAMPLE_MQTT_SN_GATEWAY_PORT);
	err = zsock_inet_pton(AF_INET, CONFIG_NET_SAMPLE_MQTT_SN_GATEWAY_IP, &gateway.sin_addr);
	__ASSERT(err == 1, "zsock_inet_pton() failed %d", err);

	LOG_INF("Waiting for connection...");
	LOG_HEXDUMP_DBG(&gateway, sizeof(gateway), "gateway");

	LOG_INF("Connecting client");

	err = mqtt_sn_transport_udp_init(&tp, (struct sockaddr *)&gateway, sizeof((gateway)));
	__ASSERT(err == 0, "mqtt_sn_transport_udp_init() failed %d", err);

	err = mqtt_sn_client_init(&mqtt_client, &client_id, &tp.tp, evt_cb, tx_buf, sizeof(tx_buf),
				  rx_buf, sizeof(rx_buf));
	__ASSERT(err == 0, "mqtt_sn_client_init() failed %d", err);

	err = mqtt_sn_connect(&mqtt_client, false, true);
	__ASSERT(err == 0, "mqtt_sn_connect() failed %d", err);

	while (err == 0) {
		k_sleep(K_MSEC(500));
		err = do_work();
	}

	LOG_ERR("Exiting thread: %d", err);
}

int start_thread(void)
{
	int rc;
#if defined(CONFIG_USERSPACE)
	rc = k_mem_domain_add_thread(&app_domain, udp_thread_id);
	if (rc < 0) {
		return rc;
	}
#endif

	rc = k_thread_name_set(udp_thread_id, "udp");
	if (rc < 0 && rc != -ENOSYS) {
		LOG_ERR("Failed: k_thread_name_set() %d", rc);
		return rc;
	}

	k_thread_start(udp_thread_id);

	return k_thread_join(udp_thread_id, K_FOREVER);
}
