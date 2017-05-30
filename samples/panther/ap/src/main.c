
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
	.client_id ="panther-board-demo-1",
	.user_id = "/pegasus:"GW_HID,
	.password = GW_PWD,
	.topic = "krs/tel/gts/"GW_HID,
};

/**
 * definitions for board identification
 */
struct device_hid {
	u8_t    mac[6];
	char   *client_id;
	char   *device_hid;
};

struct device_hid dev_hid[] = {
	{{0xF8, 0xF0, 0x05, 0xFC, 0xAB, 0x0A}, "Panther Demo F8F005FCAB0A", "2cc84c225273a8fff87a3fc4a7d00c98e91c4afd"},
	{{0xF8, 0xF0, 0x05, 0xFC, 0xAB, 0x0C}, "Panther Demo F8F005FCAB0C", "3f5f8147161ad5a6654d39b6bbfaf7f2a465d57c"},
	{{0xF8, 0xF0, 0x05, 0xFC, 0xAA, 0xAB}, "Panther Demo F8F005FCAAAB", "1551f923a45d7a0cf01d2e47aa339f834f5d0db5"},
	{{0xF8, 0xF0, 0x05, 0xFC, 0xAA, 0xB0}, "Panther Demo F8F005FCAAB0", "b03764d40ae12e037be6c17e765fe1f3cb59fdf2"},
	{{0xF8, 0xF0, 0x05, 0xFC, 0xAA, 0xCF}, "Panther Demo F8F005FCAACF", "e00f177e85baa96fa73f8cef937b599764a54ae5"},
	{{0xF8, 0xF0, 0x05, 0xFC, 0xAB, 0xDB}, "Panther Demo F8F005FCABDB", "9b88adf90e94d1f1417af7bc5c5ae3e3492b4f59"},
	{{0xF8, 0xF0, 0x05, 0xFC, 0xAA, 0xE0}, "Panther Demo F8F005FCAAE0", "95e744f39f07ab8850fdb585309cb4f006cf4b07"},
	{{0xF8, 0xF0, 0x05, 0xFC, 0xAA, 0xEB}, "Panther Demo F8F005FCAAEB", "eca71170e1293e84f514e9a059c8321485d0dd90"},
	{{0xF8, 0xF0, 0x05, 0xFC, 0xAA, 0xF1}, "Panther Demo F8F005FCAAF1", "bdd6c0ee3102d6866c95a5428e4d2de74be067c8"},
	{{0xF8, 0xF0, 0x05, 0xFC, 0xAA, 0xF2}, "Panther Demo F8F005FCAAF2", "8d90a0dcc3c357e486dbb21922bfbb6942810a8b"},
};


void main(void)
{
	int ret;
	int device_idx;
	u8_t mac[6];
	u8_t is_valid;
	int i;

	printk("Panther test program "__DATE__" "__TIME__"\n");

	/**
	 * getting WiFi MAC address
	 */
	m2m_wifi_get_otp_mac_address(mac, &is_valid);
	printk("WINC1500 MAC Address from OTP (%d) "
			    "%02X:%02X:%02X:%02X:%02X:%02X",
			is_valid,
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	/**
	 * searching in devices table
	 */
	for (i = 0; i < sizeof(dev_hid)/sizeof(struct device_hid); i++) {
		if (memcmp(mac, dev_hid[i].mac, 6) == 0) {
			break;
		}
	}
	device_idx = i;
	printk("MQTT client index is %d, id is %s\n",
			device_idx, dev_hid[device_idx].client_id);

	/**
	 * Setting up client ID for the device
	 */
	client_prm.client_id = dev_hid[device_idx].client_id;

	/**
	 * Initialize IPM communication
	 */
	printk("IPM init\n");
	ret = ipm_init(dev_hid[device_idx].device_hid, client_prm.topic);
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
