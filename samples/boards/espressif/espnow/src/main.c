/*
 * Copyright (c) 2026 Rapyuta Robotics
 * SPDX-License-Identifier: Apache-2.0
 *
 * ESP-NOW sample -- broadcast heartbeat beacons and/or receive them.
 *
 * Role is selected at build time via Kconfig:
 *   CONFIG_ESPNOW_ROLE_SENDER   -- TX only
 *   CONFIG_ESPNOW_ROLE_RECEIVER -- RX only
 *   CONFIG_ESPNOW_ROLE_BIDIR     -- TX + RX (default; loopback test with two boards)
 *
 * Both devices must be on the same Wi-Fi channel (CONFIG_ESPNOW_CHANNEL).
 * No AP is required -- the channel is fixed via esp_wifi_set_channel().
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <inttypes.h>
#include <string.h>

#include <esp_now.h>
#include <esp_wifi.h>

LOG_MODULE_REGISTER(espnow_sample, LOG_LEVEL_INF);

/*
 * Heartbeat frame -- fixed-layout, 14 bytes on the wire.
 * seq and uptime_ms use little-endian (native ESP32).
 */
struct __packed espnow_heartbeat {
	uint32_t seq;
	uint32_t uptime_ms;
	uint8_t src_mac[ESP_NOW_ETH_ALEN];
};

static const uint8_t bcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/*
 * RX callback -- Wi-Fi task context; no blocking, no heap alloc.
 */
#if defined(CONFIG_ESPNOW_ROLE_RECEIVER) || defined(CONFIG_ESPNOW_ROLE_BIDIR)
static void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
	if (!info || !data || len <= 0) {
		return;
	}

	const uint8_t *mac = info->src_addr;
	const uint8_t *dst = info->des_addr;
	int8_t rssi = info->rx_ctrl->rssi;
	bool bcast = (memcmp(dst, bcast_mac, ESP_NOW_ETH_ALEN) == 0);

	LOG_INF("RX [%s] from %02x:%02x:%02x:%02x:%02x:%02x  rssi=%d  len=%d",
		bcast ? "BCAST" : "UNICAST", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], rssi,
		len);

	if (len == (int)sizeof(struct espnow_heartbeat)) {
		const struct espnow_heartbeat *hb = (const struct espnow_heartbeat *)data;

		LOG_INF("  heartbeat seq=%" PRIu32 "  uptime=%" PRIu32 " ms"
			"  src=%02x:%02x:%02x:%02x:%02x:%02x",
			hb->seq, hb->uptime_ms, hb->src_mac[0], hb->src_mac[1], hb->src_mac[2],
			hb->src_mac[3], hb->src_mac[4], hb->src_mac[5]);
	} else {
		LOG_HEXDUMP_INF(data, MIN(len, 32), "  payload:");
	}
}
#endif /* RECEIVER || BIDIR */

/*
 * TX callback -- Wi-Fi task context.
 */
#if defined(CONFIG_ESPNOW_ROLE_SENDER) || defined(CONFIG_ESPNOW_ROLE_BIDIR)
static void send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
	if (!tx_info) {
		return;
	}

	const uint8_t *mac = tx_info->des_addr;

	if (status == ESP_NOW_SEND_SUCCESS) {
		LOG_INF("TX OK  -> %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
			mac[4], mac[5]);
	} else {
		LOG_ERR("TX FAIL -> %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
			mac[4], mac[5]);
	}
}

/*
 * Beacon thread -- sends a heartbeat broadcast every BEACON_INTERVAL_S.
 */
#define BEACON_STACK_SIZE 2048
#define BEACON_PRIORITY   5

static void beacon_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	k_thread_name_set(k_current_get(), "beacon");

	uint32_t seq = 0;
	esp_err_t err;

	while (true) {
		struct espnow_heartbeat hb = {};

		hb.seq = seq++;
		hb.uptime_ms = k_uptime_get_32();
		err = esp_wifi_get_mac(WIFI_IF_STA, hb.src_mac);
		if (err != ESP_OK) {
			LOG_ERR("esp_wifi_get_mac failed: 0x%x", err);
		}

		LOG_INF("TX beacon seq=%" PRIu32 "  uptime=%" PRIu32 " ms"
			"  mac=%02x:%02x:%02x:%02x:%02x:%02x",
			hb.seq, hb.uptime_ms, hb.src_mac[0], hb.src_mac[1], hb.src_mac[2],
			hb.src_mac[3], hb.src_mac[4], hb.src_mac[5]);

		err = esp_now_send(bcast_mac, (const uint8_t *)&hb, sizeof(hb));

		if (err != ESP_OK) {
			LOG_ERR("esp_now_send failed: 0x%x", err);
		}

		k_sleep(K_SECONDS(CONFIG_ESPNOW_BEACON_INTERVAL_S));
	}
}

K_THREAD_STACK_DEFINE(s_beacon_stack, BEACON_STACK_SIZE);
static struct k_thread s_beacon_tid;
#endif /* SENDER || BIDIR */

/*
 * main
 */
int main(void)
{
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_now_peer_info_t peer = {};
	wifi_mode_t cur_mode;
	esp_err_t rc;

	/* 1. Wi-Fi STA init -- Zephyr driver may call esp_wifi_init() internally;
	 *    guard against double-init by checking current state first.
	 */
	rc = esp_wifi_get_mode(&cur_mode);
	if (rc == ESP_ERR_WIFI_NOT_INIT) {
		rc = esp_wifi_init(&cfg);
		if (rc != ESP_OK) {
			LOG_ERR("esp_wifi_init failed: 0x%x", rc);
			return -EIO;
		}
	}
	rc = esp_wifi_set_storage(WIFI_STORAGE_RAM);
	if (rc != ESP_OK) {
		LOG_ERR("esp_wifi_set_storage failed: 0x%x", rc);
		return -EIO;
	}
	rc = esp_wifi_set_mode(ESP32_WIFI_MODE_STA);
	if (rc != ESP_OK) {
		LOG_ERR("esp_wifi_set_mode failed: 0x%x", rc);
		return -EIO;
	}
	rc = esp_wifi_start();
	if (rc != ESP_OK) {
		LOG_ERR("esp_wifi_start failed: 0x%x", rc);
		return -EIO;
	}

	/* 2. Lock to configured channel -- no AP needed */
	rc = esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
	if (rc != ESP_OK) {
		LOG_ERR("esp_wifi_set_channel failed: 0x%x", rc);
		return -EIO;
	}

	/* 3. ESP-NOW init + callbacks */
	rc = esp_now_init();
	if (rc != ESP_OK) {
		LOG_ERR("esp_now_init failed: 0x%x", rc);
		return -EIO;
	}

#if defined(CONFIG_ESPNOW_ROLE_RECEIVER) || defined(CONFIG_ESPNOW_ROLE_BIDIR)
	rc = esp_now_register_recv_cb(recv_cb);
	if (rc != ESP_OK) {
		LOG_ERR("esp_now_register_recv_cb failed: 0x%x", rc);
		return -EIO;
	}
#endif
#if defined(CONFIG_ESPNOW_ROLE_SENDER) || defined(CONFIG_ESPNOW_ROLE_BIDIR)
	rc = esp_now_register_send_cb(send_cb);
	if (rc != ESP_OK) {
		LOG_ERR("esp_now_register_send_cb failed: 0x%x", rc);
		return -EIO;
	}
#endif

	/* 4. Add broadcast peer (required before esp_now_send to FF:FF:FF:FF:FF:FF) */
	memcpy(peer.peer_addr, bcast_mac, ESP_NOW_ETH_ALEN);
	peer.channel = CONFIG_ESPNOW_CHANNEL;
	peer.ifidx = WIFI_IF_STA;
	peer.encrypt = false;
	rc = esp_now_add_peer(&peer);
	if (rc != ESP_OK) {
		LOG_ERR("esp_now_add_peer failed: 0x%x", rc);
		return -EIO;
	}

	uint32_t ver = 0;

	rc = esp_now_get_version(&ver);
	if (rc != ESP_OK) {
		LOG_ERR("esp_now_get_version failed: 0x%x", rc);
		return -EIO;
	}
	LOG_INF("ESP-NOW v%u ready  ch=%u  role=%s", (unsigned int)ver, CONFIG_ESPNOW_CHANNEL,
#if defined(CONFIG_ESPNOW_ROLE_SENDER)
		"SENDER"
#elif defined(CONFIG_ESPNOW_ROLE_RECEIVER)
		"RECEIVER"
#else
		"BIDIR"
#endif
	);

	/* 5. Start beacon thread (sender / both roles) */
#if defined(CONFIG_ESPNOW_ROLE_SENDER) || defined(CONFIG_ESPNOW_ROLE_BIDIR)
	k_thread_create(&s_beacon_tid, s_beacon_stack, K_THREAD_STACK_SIZEOF(s_beacon_stack),
			beacon_thread, NULL, NULL, NULL, BEACON_PRIORITY, 0, K_NO_WAIT);
	LOG_INF("Beacon thread started  interval=%ds", CONFIG_ESPNOW_BEACON_INTERVAL_S);
#else
	LOG_INF("Receiver-only mode -- waiting for beacons");
#endif

	return 0;
}
