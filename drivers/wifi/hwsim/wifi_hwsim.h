/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_HWSIM_H_
#define ZEPHYR_DRIVERS_WIFI_HWSIM_H_

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi.h>

#define HWSIM_FRAME_MTU      2346
#define HWSIM_MAX_RADIOS     CONFIG_WIFI_HWSIM_NUM_RADIOS
#define HWSIM_BEACON_IE_MAX  384
#define HWSIM_ETH_ALEN       6

struct hwsim_frame {
	sys_snode_t  node;
	uint16_t     len;
	uint8_t      src_idx;
	bool         is_mgmt;
	int8_t       signal_dbm;
	uint32_t     freq_mhz;
	uint8_t      data[HWSIM_FRAME_MTU];
};

struct hwsim_radio {
	struct net_if   *iface;
	struct k_fifo    rx_queue;
	struct k_thread  rx_thread;

	K_KERNEL_STACK_MEMBER(rx_stack, CONFIG_WIFI_HWSIM_RX_STACK_SIZE);

	uint32_t   freq_mhz;
	int8_t     tx_power_dbm;
	uint8_t    loss_pct;

	bool       ap_mode;
	uint8_t    bssid[6];
	uint8_t    ssid[WIFI_SSID_MAX_LEN + 1];
	uint8_t    ssid_len;
	uint8_t    channel;
	uint8_t    security;
	/* Beacon IEs from wpa_supp (start_ap); used for scan results. */
	uint8_t    beacon_ie[HWSIM_BEACON_IE_MAX];
	size_t     beacon_ie_len;

	bool       associated;
	uint8_t    ap_bssid[6];
	uint8_t    idx;

	/* Set by wpa_supplicant driver when CONFIG_WIFI_NM_WPA_SUPPLICANT */
	void       *supp_ctx;
};

/* Called from RX path for 802.11 mgmt frames when supp_ctx is set. Weak stub. */
void hwsim_supp_mgmt_rx(struct hwsim_radio *radio, const uint8_t *data,
			unsigned int len, unsigned int freq_mhz, int rssi_dbm);

int  hwsim_medium_register(struct hwsim_radio *radio, uint8_t idx);
/*
 * Relay a frame onto the shared medium. @is_mgmt selects routing on the
 * receive side: management frames (802.11) are handed to wpa_supplicant,
 * data frames (802.3) are injected into the peer's network RX path. Data
 * frames are delivered only to the radio whose link address matches the
 * L2 destination (or to all peers for broadcast/multicast).
 */
int  hwsim_medium_xmit(uint8_t src_idx, const uint8_t *frame, uint16_t len,
		       bool is_mgmt);
struct hwsim_radio *hwsim_medium_get_radio(uint8_t idx);

extern struct k_mem_slab hwsim_frame_slab;

void hwsim_set_channel(uint8_t idx, uint32_t freq_mhz);
void hwsim_set_signal(uint8_t idx, int8_t dbm);
void hwsim_set_loss(uint8_t idx, uint8_t pct);

#endif /* ZEPHYR_DRIVERS_WIFI_HWSIM_H_ */
