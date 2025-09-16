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

#ifdef CONFIG_NET_SAMPLE_MQTT_SN_STATIC_GATEWAY
#define SAMPLE_GW_ADDRESS CONFIG_NET_SAMPLE_MQTT_SN_GATEWAY_ADDRESS
#else
#define SAMPLE_GW_ADDRESS ""
#endif

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
	case MQTT_SN_EVT_ADVERTISE: /* Received a ADVERTISE */
		LOG_INF("MQTT-SN event EVT_ADVERTISE");
		break;
	case MQTT_SN_EVT_GWINFO: /* Received a GWINFO */
		LOG_INF("MQTT-SN event EVT_GWINFO");
		break;
	case MQTT_SN_EVT_SEARCHGW: /* Received a SEARCHGW */
		LOG_INF("MQTT-SN event EVT_SEARCHGW");
		break;
	default:
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
	struct sockaddr bcaddr = {0};
	int err;
	bool success;

	LOG_DBG("Parsing Broadcast Address " CONFIG_NET_SAMPLE_MQTT_SN_BROADCAST_ADDRESS);
	success = net_ipaddr_parse(CONFIG_NET_SAMPLE_MQTT_SN_BROADCAST_ADDRESS,
				   strlen(CONFIG_NET_SAMPLE_MQTT_SN_BROADCAST_ADDRESS), &bcaddr);
	__ASSERT(success, "net_ipaddr_parse() failed");

	LOG_INF("Waiting for connection...");
	LOG_HEXDUMP_DBG(&bcaddr, sizeof(bcaddr), " broadcast address");

	err = mqtt_sn_transport_udp_init(&tp, &bcaddr, sizeof((bcaddr)));
	__ASSERT(err == 0, "mqtt_sn_transport_udp_init() failed %d", err);

	err = mqtt_sn_client_init(&mqtt_client, &client_id, &tp.tp, evt_cb, tx_buf, sizeof(tx_buf),
				  rx_buf, sizeof(rx_buf));
	__ASSERT(err == 0, "mqtt_sn_client_init() failed %d", err);

	if (IS_ENABLED(CONFIG_NET_SAMPLE_MQTT_SN_STATIC_GATEWAY)) {
		LOG_INF("Adding predefined Gateway");
		struct sockaddr gwaddr = {0};

		LOG_DBG("Parsing Gateway address %s", SAMPLE_GW_ADDRESS);
		success = net_ipaddr_parse(SAMPLE_GW_ADDRESS, strlen(SAMPLE_GW_ADDRESS), &gwaddr);
		__ASSERT(success, "net_ipaddr_parse() failed");
		struct mqtt_sn_data gwaddr_data = {.data = (uint8_t *)&gwaddr,
						   .size = sizeof(gwaddr)};
		/* Reduce size to allow this to work with smaller values for
		 * CONFIG_MQTT_SN_LIB_MAX_ADDR_SIZE.
		 */
		switch (gwaddr.sa_family) {
		case AF_INET:
			gwaddr_data.size = sizeof(struct sockaddr_in);
			break;
		case AF_INET6:
			gwaddr_data.size = sizeof(struct sockaddr_in6);
			break;
		}

		err = mqtt_sn_add_gw(&mqtt_client, 0x1f, gwaddr_data);
		__ASSERT(err == 0, "mqtt_sn_add_gw() failed %d", err);
	} else {
		LOG_INF("Searching for Gateway");
		err = mqtt_sn_search(&mqtt_client, 1);
		k_sleep(K_SECONDS(10));
		err = mqtt_sn_input(&mqtt_client);
		__ASSERT(err == 0, "mqtt_sn_search() failed %d", err);
	}

	LOG_INF("Connecting client");
	err = mqtt_sn_connect(&mqtt_client, false, true);
	__ASSERT(err == 0, "mqtt_sn_connect() failed %d", err);

	while (err == 0) {
		k_sleep(K_MSEC(500));
		err = do_work();
	}

	LOG_ERR("Exiting thread: %d", err);
}

void start_thread(void)
{
	int rc;
#if defined(CONFIG_USERSPACE)
	rc = k_mem_domain_add_thread(&app_domain, udp_thread_id);
	if (rc < 0) {
		LOG_ERR("Failed: k_mem_domain_add_thread() %d", rc);
		return;
	}
#endif

	rc = k_thread_name_set(udp_thread_id, "udp");
	if (rc < 0 && rc != -ENOSYS) {
		LOG_ERR("Failed: k_thread_name_set() %d", rc);
		return;
	}

	k_thread_start(udp_thread_id);

	rc = k_thread_join(udp_thread_id, K_FOREVER);

	if (rc != 0) {
		LOG_ERR("Failed: k_thread_join() %d", rc);
	}
}
