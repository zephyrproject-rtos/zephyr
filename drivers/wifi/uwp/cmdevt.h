/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WIFI_CMDEVT_H__
#define __WIFI_CMDEVT_H__

#include <net/wifi_drv.h>

#include "main.h"

#define MAX_SSID_LEN (33) /* SSID end with 0 */
#define MAX_KEY_LEN (128) /* FIXME: Max size 64 */
#define MAX_AP_KEY_LEN (64)



enum cmd_type {
	WIFI_CMD_BEGIN = 0x00,
	/* Common command. */
	WIFI_CMD_GET_CP_INFO,   /* Get cp version number. */
	WIFI_CMD_DOWNLOAD_INI,
	WIFI_CMD_OPEN,          /* Open a wifi mode, ap or sta. */
	WIFI_CMD_CLOSE,         /* Stop wifi mode. */
	WIFI_CMD_NPI_MSG,

	/* Sta command. */
	WIFI_CMD_SCAN,
	WIFI_CMD_CONNECT,
	WIFI_CMD_DISCONNECT,
	WIFI_CMD_GET_STATION,   /* Get sta's RSSI and tx rate. */
	/* Ap command. */
	WIFI_CMD_START_AP,      /* Start ap mode. */
	WIFI_CMD_DEL_STATION,
	WIFI_CMD_SET_BLACKLIST,
	WIFI_CMD_SET_WHITELIST,

	WIFI_CMD_NOTIFY_IP_ACQUIRED = 0x0F, /* Set IP address. */
	WIFI_CMD_RTT = 0x10,

	WIFI_CMD_MAX,
};

enum event_type {
	WIFI_EVENT_BEGIN = 0x80,
	/* Sta event. */
	WIFI_EVENT_CONNECT,
	WIFI_EVENT_DISCONNECT,
	WIFI_EVENT_SCAN_RESULT,
	WIFI_EVENT_SCAN_DONE,
	/* Ap event. */
	WIFI_EVENT_NEW_STATION,

	WIFI_EVENT_RTT = 0x86,

	WIFI_EVENT_MAX,
};

enum vht_chan_bw {
	VHT_CHAN_BW_20M = 0x00,
	VHT_CHAN_BW_40M,
	VHT_CHAN_BW_80M,
	VHT_CHAN_BW_160M,
	VHT_CHAN_BW_80_80M,
	VHT_CHAN_BW_5M,
	VHT_CHAN_BW_10M,

	VHT_CHAN_BW_DEF = 0xFF,
};

enum blacklist_sub_type {
	ADD_MAC_ACL = 3,
	DEL_MAC_ACL,
};

enum rtt_sub_type {
	RTT_ENABLE,
	RTT_DISABLE,
	RTT_GET_CAPABILITIES,
	RTT_RANGE_REQUEST,
	RTT_RANGE_CANCEL,
	RTT_SET_CLI,
	RTT_SET_CLR,
	RTT_GET_RESPONDER_INFO,
	RTT_ENABLE_RESPONDER,
	RTT_DISABLE_RESPONDER,
};

enum rtt_sub_evt {
	RTT_SESSION_END,
	RTT_PER_DEST_RES,
	RTT_PER_DEST_METER,
};

/* These structures are shared by command and event. */
struct trans_hdr {
	u8_t type;      /* Which command or event will be transferred. */
	u8_t seq;       /* Event sequence. */
	u8_t response;  /* 1 for command response. 0 for cp event. */
	char status;    /* 0 for response success, other for fail. */
	u16_t len;      /* Message length. */
	char data[0];   /* The start of message data. */
} __packed;

struct cmd_download_ini {
	struct trans_hdr trans_header;
	u32_t sec_num;
	char data[0];
} __packed;

struct cmd_open {
	struct trans_hdr trans_header;
	char mode;
	char mac[ETH_ALEN];
} __packed;

struct cmd_get_cp_info {
	struct trans_hdr trans_header;
	/* Response value */
	u32_t version;
	char mac[ETH_ALEN];
	u8_t max_ap_assoc_sta_num;
	u8_t max_ap_blacklist_sta_num;
	u8_t max_rtt_num;
} __packed;

struct cmd_stop {
	struct trans_hdr trans_header;
	char mode;
	char mac[ETH_ALEN];
} __packed;

/* Command struct for ap. */
struct cmd_start_ap {
	struct trans_hdr trans_header;
	u8_t ssid_len;
	u8_t ssid[MAX_SSID_LEN];
	u8_t password_len;
	char password[MAX_AP_KEY_LEN];
	u8_t channel;
	u8_t vht_chwidth;
	u8_t vht_chan_center_freq_seg0_idx;
	u8_t vht_chan_center_freq_seg1_idx;
} __packed;

struct cmd_del_sta {
	struct trans_hdr trans_header;
	/**
	 * If mac set to FF:FF:FF:FF:FF:FF,
	 * all station will be disconnected.
	 */
	u8_t mac[ETH_ALEN];
	u16_t reason_code;
} __packed;

struct cmd_set_blacklist {
	struct trans_hdr trans_header;
	u8_t sub_type; /* 3:add, 4:del */
	u8_t mac_num;
	u8_t mac_addr[0];
} __packed;

/* Command struct for sta. */
struct cmd_scan {
	struct trans_hdr trans_header;
	u32_t channels_2g; /* One bit for one 2.4G channel. */
	u32_t flags;
	u16_t ssid_len; /* Hidden ssid length. */
	u8_t ssid[0]; /* FIXME: Invalid reservation. Hidden ssid. */
	u16_t channels_5g_cnt; /* Number of 5G channels. */
	u16_t channels_5g[0]; /* 5G channels to be scanned. */
} __packed;

struct cmd_connect {
	struct trans_hdr trans_header;
	u32_t wpa_versions;
	u8_t bssid[ETH_ALEN];
	u8_t channel;
	u8_t auth_type;
	u8_t pairwise_cipher;
	u8_t group_cipher;
	u8_t key_mgmt;
	u8_t mfp_enable;
	u8_t psk_len;
	u8_t ssid_len;
	u8_t psk[MAX_KEY_LEN];
	u8_t ssid[MAX_SSID_LEN];
} __packed;

struct cmd_disconnect {
	struct trans_hdr trans_header;
	u8_t reason_code;
} __packed;

struct cmd_set_ip {
	struct trans_hdr trans_header;
	/**
	 * IPV4 address
	 * TODO: For compatibility with ipv6,
	 * it might be greater than 4 bytes.
	 */
	u8_t ip[IPV4_LEN];
} __packed;

struct cmd_get_sta {
	struct trans_hdr trans_header;
	/**
	 * Response value
	 * Now needs signal only.
	 */
	u8_t reserved[5];
	s8_t signal;
	u8_t reserved_1[6];
} __packed;

struct cmd_npi {
	struct trans_hdr trans_header;
	u8_t data[0];
} __packed;

struct wifi_channel_info {
	u32_t width;
	s32_t chan_num;
	s32_t chan_num0;
	s32_t chan_num1;
} __packed;

struct rtt_peer_info {
	u8_t mac[ETH_ALEN];
	u8_t wifi_rtt_type;
	u32_t rtt_peer_type;
	struct wifi_channel_info chan_info;
	u32_t burst_period; /* 0-31 */
	u32_t num_burst; /* 0-15 */
	u32_t num_frames_per_burst; /* 0-31 */
	u32_t num_retries_per_rtt_frame; /* 0-3 */
	u32_t num_retries_per_rtt_ftmr;
	u8_t LCI_request;
	u8_t LCR_request;
	u32_t burst_duration;
	u8_t preamble;
	u8_t bw;
} __packed;

struct cmd_session_request {
	struct trans_hdr trans_header;
	u8_t sub_type;
	u8_t len;
	u8_t n_peers;
	struct rtt_peer_info peers[0];
} __packed;

struct event_scan_result {
	u8_t band;
	u8_t channel;
	s8_t rssi;
	u8_t encrypt_mode;
	char bssid[ETH_ALEN];
	char ssid[MAX_SSID_LEN];
	/**
	 * Extra functon supported,
	 * like RTT.
	 */
	u32_t extra;
} __packed;

struct event_scan_done {
	/* 0: success 1: fail */
	u8_t status;
} __packed;

struct event_connect {
	u8_t status;
	u8_t bssid[ETH_ALEN];
	u8_t primary_chan_num;
} __packed;

struct event_disconnect {
	u8_t reason_code;
} __packed;

struct event_new_station {
	u8_t is_connect; /* 1 for connected, 0 for disconnected. */
	u8_t mac[ETH_ALEN];
	/* u16_t ie_len; */
	/* u8_t ie[0]; */
} __packed;

struct event_rtt {
	u8_t sub_type;
	u8_t data[0];
} __packed;

struct event_rtt_per_dest_meter {
	u8_t sub_type;
	u8_t mac[ETH_ALEN];
	s32_t meter;
} __packed;

int wifi_cmd_get_cp_info(struct wifi_priv *priv);
int wifi_cmd_open(struct wifi_device *wifi_dev);
int wifi_cmd_close(struct wifi_device *wifi_dev);
int wifi_cmd_scan(struct wifi_device *wifi_dev,
		struct wifi_drv_scan_params *params);
int wifi_cmd_connect(struct wifi_device *wifi_dev,
			    struct wifi_drv_connect_params *params);
int wifi_cmd_disconnect(struct wifi_device *wifi_dev);
int wifi_cmd_get_sta(struct wifi_device *wifi_dev,
		s8_t *signal);
int wifi_cmd_start_ap(struct wifi_device *wifi_dev,
		struct wifi_drv_start_ap_params *params);
int wifi_cmd_del_sta(struct wifi_device *wifi_dev,
		u8_t *mac, u16_t reason_code);
int wifi_cmd_set_blacklist(struct wifi_device *wifi_dev,
		u8_t sub_type, u8_t mac_num, u8_t **mac_addr);
int wifi_cmd_stop_ap(struct wifi_device *wifi_dev);
int wifi_cmd_hw_test(struct wifi_device *wifi_dev,
		char *t_buf, u32_t t_len,
		char *r_buf, u32_t *r_len);
int wifi_cmd_notify_ip_acquired(struct wifi_device *wifi_dev,
		u8_t *ip, u8_t len);
int wifi_cmd_session_request(struct wifi_device *wifi_dev,
		struct wifi_drv_rtt_request *params);

int wifi_cmd_send(u8_t cmd, char *data, int len,
			 char *rbuf, int *rlen);
int wifi_cmd_load_ini(const u8_t *data, u32_t len, u8_t sec_num);
int wifi_cmdevt_process(struct wifi_priv *priv, char *data, int len);
int wifi_cmdevt_init(void);

#endif /* __WIFI_CMDEVT_H__ */
