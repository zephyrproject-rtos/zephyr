
/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <misc/byteorder.h>
#include <misc/printk.h>
#include <zephyr.h>
#include <string.h>
#include <net/wifi_mgmt.h>

#include "wifi.h"
#include "mqtt.h"

int ipm_init(char *device_hid, char *topic);

/**
 * definition for WiFi connection
 *
 *	ap_name 	SSID of your network access point
 *	password 	password
 *	security 	security see wifi_security_t
 *
 */
struct wifi_conn_params wifi_params = {
	.ap_name = "SSID",
	.password = "password",
	.security = WIFI_SECURITY_OPEN,
};

/**
 * definitions for MQTT
 *
 *		server_addr 	server address
 *		server_port 	server port
 *		device_hid 		device identification
 *		client_id 		client identification
 *		user_id 		user identification
 *		password 		password
 *		topic 			publish topic
 */
struct mqtt_client_prm client_prm = {
	.server_addr = "104.214.111.120",
	.server_port = 1883,
	.device_hid = "device_hid",
	.client_id = "client_id",
	.user_id = "user",
	.password = "password",
	.topic = "topic",
};

void main(void)
{
	int ret;

	printk("Panther mqtt test program "__DATE__" "__TIME__"\n");

	/**
	 * Initialize IPM communication
	 */
	printk("IPM init\n");
	ret = ipm_init(client_prm.device_hid, client_prm.topic);
	if (ret) {
		printk("IPM init FAIL\n");
	}

	/**
	 * Initialize WiFi connection and start MQTT Publisher after WiFi is connected
	 */
	printk("Test WiFi mqtt\n");
	ret = wifi_init(&wifi_params, mqtt_pub_init, (void *)&client_prm);
	if (ret) {
		printk("WiFi init callback function FAIL\n");
	}

	k_sleep(K_FOREVER);
}
