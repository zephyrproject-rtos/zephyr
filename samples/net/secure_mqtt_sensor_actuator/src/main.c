/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_main, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/conn_mgr_monitor.h>

#include "mqtt_client.h"
#include "device.h"

#define NET_L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

/* MQTT client struct */
static struct mqtt_client client_ctx;

/* MQTT publish work item */
struct k_work_delayable mqtt_publish_work;

static struct net_mgmt_event_callback net_l4_mgmt_cb;

/* Network connection semaphore */
K_SEM_DEFINE(net_conn_sem, 0, 1);

static void net_l4_evt_handler(struct net_mgmt_event_callback *cb,
				uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_L4_CONNECTED:
		k_sem_give(&net_conn_sem);
		LOG_INF("Network connectivity up!");
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network connectivity down!");
		break;
	default:
		break;
	}
}

/** Print the device's MAC address to console */
void log_mac_addr(struct net_if *iface)
{
	struct net_linkaddr *mac;

	mac = net_if_get_link_addr(iface);

	LOG_INF("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
		mac->addr[0], mac->addr[1], mac->addr[3],
		mac->addr[3], mac->addr[4], mac->addr[5]);
}

/** The system work queue is used to handle periodic MQTT publishing.
 *  Work queuing begins when the MQTT connection is established.
 *  Use CONFIG_NET_SAMPLE_MQTT_PUBLISH_INTERVAL to set the publish frequency.
 */

static void publish_work_handler(struct k_work *work)
{
	int rc;

	if (mqtt_connected) {
		rc = app_mqtt_publish(&client_ctx);
		if (rc != 0) {
			LOG_INF("MQTT Publish failed [%d]", rc);
		}
		k_work_reschedule(&mqtt_publish_work,
					K_SECONDS(CONFIG_NET_SAMPLE_MQTT_PUBLISH_INTERVAL));
	} else {
		k_work_cancel_delayable(&mqtt_publish_work);
	}
}

int main(void)
{
	int rc;
	struct net_if *iface;

	devices_ready();

	iface = net_if_get_default();
	if (iface == NULL) {
		LOG_ERR("No network interface configured");
		return -ENETDOWN;
	}

	log_mac_addr(iface);

	/* Register callbacks for L4 events */
	net_mgmt_init_event_callback(&net_l4_mgmt_cb, &net_l4_evt_handler, NET_L4_EVENT_MASK);
	net_mgmt_add_event_callback(&net_l4_mgmt_cb);

	LOG_INF("Bringing up network..");

#if defined(CONFIG_NET_DHCPV4)
	net_dhcpv4_start(iface);
#else
	/* If using static IP, L4 Connect callback will happen,
	 * before conn mgr is initialised, so resend events here
	 * to check for connectivity
	 */
	conn_mgr_mon_resend_status();
#endif

	/* Wait for network to come up */
	while (k_sem_take(&net_conn_sem, K_MSEC(MSECS_NET_POLL_TIMEOUT)) != 0) {
		LOG_INF("Waiting for network connection..");
	}

	rc = app_mqtt_init(&client_ctx);
	if (rc != 0) {
		LOG_ERR("MQTT Init failed [%d]", rc);
		return rc;
	}

	/* Initialise MQTT publish work item */
	k_work_init_delayable(&mqtt_publish_work, publish_work_handler);

	/* Thread main loop */
	while (1) {
		/* Block until MQTT connection is up */
		app_mqtt_connect(&client_ctx);

		/* We are now connected, begin queueing periodic MQTT publishes */
		k_work_reschedule(&mqtt_publish_work,
					K_SECONDS(CONFIG_NET_SAMPLE_MQTT_PUBLISH_INTERVAL));

		/* Handle MQTT inputs and connection */
		app_mqtt_run(&client_ctx);
	}

	return rc;
}
