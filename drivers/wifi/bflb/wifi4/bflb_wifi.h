/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BFLB_WIFI_H_
#define BFLB_WIFI_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

#define BFLB_WIFI_MAC_ADDR_LEN 6U
#define BFLB_WIFI_SSID_MAX_LEN 32U
#define BFLB_WIFI_PSK_MAX_LEN  64U

/* Ethernet frame layout. */
#define BFLB_WIFI_ETH_HDR_LEN    14U
#define BFLB_WIFI_ETH_TYPE_OFF   12U
#define BFLB_WIFI_ETH_TYPE_EAPOL 0x888EU

/* Firmware internal table strides (blob ABI). */
#define BFLB_WIFI_STA_INFO_STRIDE 368U
#define BFLB_WIFI_VIF_INFO_STRIDE 1512U

/* WiFi block base address from devicetree; the NXMAC core and the IPC
 * mailbox sit at fixed offsets from it.
 */
#define BFLB_WIFI_REG_BASE ((uint32_t)DT_REG_ADDR(DT_NODELABEL(wifi0)))

/* NXMAC HW register addresses. */
#define NXMAC_MAC_ADDR_LOW_REG      (BFLB_WIFI_REG_BASE + 0xB00044U)
#define NXMAC_MAC_ADDR_HIGH_REG     (BFLB_WIFI_REG_BASE + 0xB00048U)
#define NXMAC_BSS_ADDR_LOW_REG      (BFLB_WIFI_REG_BASE + 0xB00054U)
#define NXMAC_BSS_ADDR_HIGH_REG     (BFLB_WIFI_REG_BASE + 0xB00058U)
#define NXMAC_RX_CNTRL_REG          (BFLB_WIFI_REG_BASE + 0xB00060U)
#define NXMAC_RX_ACCEPT_DECRYPT_ERR BIT(11)

/* Pending command (one at a time). */
#define BFLB_WIFI_PENDING_CFM_NONE 0xFFFFU

struct bflb_wifi_pending_cmd {
	uint16_t cfm_id; /* confirmation we wait for (or PENDING_CFM_NONE) */
	void *cfm_buf;   /* caller-owned confirmation buffer */
	uint16_t cfm_len;
	struct k_sem cfm; /* signalled on CFM msg */
};

/* Driver events posted from blob/ISR context to the event work queue. */
enum bflb_wifi_event_code {
	BFLB_WIFI_EVT_SCAN_DONE,
	BFLB_WIFI_EVT_ASSOCIATED,   /* 802.11 assoc done, 4-way pending */
	BFLB_WIFI_EVT_CONNECTED,    /* value = sm_connect_ind status_code */
	BFLB_WIFI_EVT_DISCONNECTED, /* value = reason code */
};

struct bflb_wifi_dev {
	struct net_if *iface;
	uint8_t mac_addr[BFLB_WIFI_MAC_ADDR_LEN];

	bool fw_ready;
	bool connected;

	struct k_mutex lock;
	scan_result_cb_t scan_cb;

	/* Command plumbing */
	struct k_mutex cmd_mutex;
	struct bflb_wifi_pending_cmd pending;
	uint32_t msga2e_tkn;

	/* Connection state */
	uint8_t connected_bssid[BFLB_WIFI_MAC_ADDR_LEN];
	uint8_t connected_ssid[BFLB_WIFI_SSID_MAX_LEN];
	uint8_t connected_ssid_len;
	uint8_t connected_channel;
	uint16_t connected_freq;
	enum wifi_security_type connected_security;
	int8_t connected_rssi;
	uint8_t vif_idx;
	uint8_t sta_idx;
};

extern struct bflb_wifi_dev bflb_wifi;

/* Platform glue (soc/bflb/bl60x/wifi). */
int bflb_wifi_hw_init(void);
void wifi_task_create(void);
int wifi_task_wait_ready(k_timeout_t timeout);

/* net */
void bflb_wifi_post_event(int code, int value);
int bflb_wifi_wait_eapol_tx_done(k_timeout_t timeout);

/* Grouped arguments for bflb_wifi_connect_req(). */
struct bflb_wifi_connect_params {
	const uint8_t *ssid;
	uint8_t ssid_len;
	const uint8_t *bssid;
	uint8_t channel;
	bool secured;
	bool sae;
	const char *phrase;
};

/* fw */
int bflb_wifi_mac_init(struct bflb_wifi_dev *d);
void bflb_wifi_handle_e2a_msg(struct bflb_wifi_dev *d, uint16_t id, const void *payload,
			      uint32_t len);
int bflb_wifi_connect_req(struct bflb_wifi_dev *d, const struct bflb_wifi_connect_params *p);
int bflb_wifi_disconnect_req(struct bflb_wifi_dev *d);
int bflb_wifi_set_ps_mode_off(struct bflb_wifi_dev *d);

#endif /* BFLB_WIFI_H_ */
