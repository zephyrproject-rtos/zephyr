
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
 */
struct wifi_conn_params wifi_params = {
	.ap_name = "iptronix",
	.password = "000w1f101ptr0n1x0w1f1000",
	.security = WIFI_SECURITY_WPA2_MIXED_PSK,
};

/**
 * definitions for MQTT
 */
#define GW_HID "c1b93ec39295c3d127b7f554873be61fa842fa13"
#define GW_PWD "7251ba7bf6f857059c32296a87ec37d71605d8598dfe0f2bfc82d97d665016d0"

struct mqtt_client_prm client_prm = {
	.server_addr = "104.214.111.120",
	.server_port = 1883,
	.device_hid = "2cc84c225273a8fff87a3fc4a7d00c98e91c4afd",
	.client_id = "Panther Demo F8F005FCAB0A",
	.user_id = "/pegasus:"GW_HID,
	.password = GW_PWD,
	.topic = "krs/tel/gts/"GW_HID,
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
