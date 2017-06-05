/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <string.h>
#include <net/wifi_mgmt.h>
#include <logging/sys_log.h>

#include "wifi.h"
#include "mqtt.h"
#include "config.h"

int ipm_init(char *topic);

/**
 * definition for WiFi connection
 *
 *	ap_name         SSID of your network access point
 *	password        password
 *	security        security see wifi_security_t
 *
 */
struct wifi_conn_params wifi_params = {
	.ap_name  = CONFIG_WIFI_SSID,
	.password = CONFIG_WIFI_PASSWORD,
	.security = CONFIG_WIFI_SECURITY,
};

/**
 * definitions for MQTT
 *
 *		server_addr     server address
 *		server_port     server port
 *		client_id       client identification
 *		user_id         user identification
 *		password        password
 *		topic           publish topic
 */
struct mqtt_client_prm client_prm = {
	.server_addr = CONFIG_MQTT_SERVER_ADDR,
	.server_port = CONFIG_MQTT_SERVER_PORT,
	.client_id   = CONFIG_MQTT_CLIENT_ID,
	.user_id     = CONFIG_MQTT_USER_ID,
	.password    = CONFIG_MQTT_PASSWORD,
	.topic       = CONFIG_MQTT_TOPIC,
};

void main(void)
{
	int ret;

	SYS_LOG_INF("Panther test program "__DATE__ " "__TIME__);

	SYS_LOG_INF("client_prm\n"
		    "  server_addr = %s\n"
		    "  server_port = %d\n"
		    "  client_id   = %s\n"
		    "  user_id     = %s\n"
		    "  password    = %s\n"
		    "  topic       = %s\n",
		    client_prm.server_addr,
		    client_prm.server_port,
		    client_prm.client_id,
		    client_prm.user_id,
		    client_prm.password,
		    client_prm.topic);

	/**
	 * Initialize IPM communication
	 */
	SYS_LOG_INF("IPM init");
	ret = ipm_init(client_prm.topic);
	if (ret) {
		SYS_LOG_ERR("IPM init FAIL");
	}

	/**
	 * Initialize WiFi connection and start MQTT Publisher
	 * after WiFi is connected
	 */
	SYS_LOG_INF("Test WiFi mqtt");
	ret = wifi_init(&wifi_params, mqtt_pub_init, (void *)&client_prm);
	if (ret) {
		SYS_LOG_ERR("WiFi init callback function FAIL");
	}

	k_sleep(K_FOREVER);
}
