/*
 * Copyright (c) 2026 Witekio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_EMW3080_API_H
#define ZEPHYR_DRIVERS_WIFI_EMW3080_API_H

#include <stdint.h>

#include <zephyr/toolchain.h>

/* System commands */
#define EMW3080_IPC_CMD_SYS_ECHO             0x0001U
#define EMW3080_IPC_CMD_SYS_REBOOT           0x0002U
#define EMW3080_IPC_CMD_SYS_VERSION          0x0003U
#define EMW3080_IPC_CMD_SYS_RESET            0x0004U
#define EMW3080_IPC_CMD_SYS_FOTA_START       0x0005U
#define EMW3080_IPC_CMD_SYS_CFG_SERVER_START 0x0006U
#define EMW3080_IPC_CMD_SYS_CFG_SERVER_STOP  0x0007U

/* Wi-Fi commands */
#define EMW3080_IPC_CMD_WIFI_GET_MAC       0x0101U
#define EMW3080_IPC_CMD_WIFI_SCAN          0x0102U
#define EMW3080_IPC_CMD_WIFI_CONNECT       0x0103U
#define EMW3080_IPC_CMD_WIFI_DISCONNECT    0x0104U
#define EMW3080_IPC_CMD_WIFI_SOFTAP_START  0x0105U
#define EMW3080_IPC_CMD_WIFI_SOFTAP_STOP   0x0106U
#define EMW3080_IPC_CMD_WIFI_GET_IP        0x0107U
#define EMW3080_IPC_CMD_WIFI_GET_LINKINFO  0x0108U
#define EMW3080_IPC_CMD_WIFI_PS_ON         0x0109U
#define EMW3080_IPC_CMD_WIFI_PS_OFF        0x010AU
#define EMW3080_IPC_CMD_WIFI_PING          0x010BU
#define EMW3080_IPC_CMD_WIFI_BYPASS_SET    0x010CU
#define EMW3080_IPC_CMD_WIFI_BYPASS_GET    0x010DU
#define EMW3080_IPC_CMD_WIFI_BYPASS_OUT    0x010EU
#define EMW3080_IPC_CMD_WIFI_EAP_SET_CERT  0x010FU
#define EMW3080_IPC_CMD_WIFI_EAP_CONNECT   0x0110U
#define EMW3080_IPC_CMD_WIFI_WPS_CONNECT   0x0111U
#define EMW3080_IPC_CMD_WIFI_WPS_STOP      0x0112U
#define EMW3080_IPC_CMD_WIFI_GET_IP6_STATE 0x0113U
#define EMW3080_IPC_CMD_WIFI_GET_IP6_ADDR  0x0114U
#define EMW3080_IPC_CMD_WIFI_GET_SOFT_MAC  0x0115U
#define EMW3080_IPC_CMD_WIFI_PING6         0x0116U

/* Events */
#define EMW3080_IPC_EVENT_SYS_REBOOT        0x8001U
#define EMW3080_IPC_EVENT_SYS_FOTA_STATUS   0x8002U
#define EMW3080_IPC_EVENT_WIFI_STATUS       0x8101U
#define EMW3080_IPC_EVENT_WIFI_BYPASS_INPUT 0x8102U

/* Payload of EMW3080_IPC_EVENT_WIFI_STATUS */
enum emw3080_wifi_event_payload {
	EMW3080_IPC_EVENTP_WIFI_NONE = 0x00,
	EMW3080_IPC_EVENTP_WIFI_STA_DOWN = 0x01,
	EMW3080_IPC_EVENTP_WIFI_STA_UP = 0x02,
	EMW3080_IPC_EVENTP_WIFI_STA_GOT_IP = 0x03,
	EMW3080_IPC_EVENTP_WIFI_AP_DOWN = 0x04,
	EMW3080_IPC_EVENTP_WIFI_AP_UP = 0x05,
};

/* Payload of EMW3080_IPC_EVENT_WIFI_BYPASS_INPUT*/
/* (Followed by tot_len bytes of Ethernet frame) */
typedef struct __packed {
	int32_t idx;
	uint8_t reserved[16];
	uint16_t tot_len;
} emw3080_bypass_input_event_payload_t;

/* Common types */

/* Security type reported by the module */
enum emw3080_ipc_security {
	EMW3080_IPC_SEC_NONE = 0,
	EMW3080_IPC_SEC_WEP = 1,
	EMW3080_IPC_SEC_WPA_TKIP = 2,
	EMW3080_IPC_SEC_WPA_AES = 3,
	EMW3080_IPC_SEC_WPA2_TKIP = 4,
	EMW3080_IPC_SEC_WPA2_AES = 5,
	EMW3080_IPC_SEC_WPA2_MIXED = 6,
	EMW3080_IPC_SEC_AUTO = 7,
};

/* Bypass interface index */
#define EMW3080_IPC_BYPASS_IDX_STA 0
#define EMW3080_IPC_BYPASS_IDX_AP  1

#define EMW3080_IPC_SSID_LEN   32
#define EMW3080_IPC_KEY_LEN    64
#define EMW3080_IPC_MAC_LEN    6
#define EMW3080_IPC_IP_STR_LEN 16

typedef uint8_t emw3080_ipc_security_t;

typedef struct __packed {
	int32_t rssi;
	char ssid[EMW3080_IPC_SSID_LEN + 1];
	uint8_t bssid[EMW3080_IPC_MAC_LEN];
	int32_t channel;
	emw3080_ipc_security_t security;
} emw3080_ipc_ap_info_t;

typedef struct __packed {
	uint8_t bssid[EMW3080_IPC_MAC_LEN];
	uint8_t channel;
	emw3080_ipc_security_t security;
} emw3080_ipc_connect_attr_t;

/* Dotted-decimal ASCII strings */
typedef struct __packed {
	char localip[EMW3080_IPC_IP_STR_LEN];
	char netmask[EMW3080_IPC_IP_STR_LEN];
	char gateway[EMW3080_IPC_IP_STR_LEN];
	char dnserver[EMW3080_IPC_IP_STR_LEN];
} emw3080_ipc_ip_attr_t;

/* This struct appears to not be packed on the module firmware (V2.3.4) */
typedef struct {
	int32_t is_connected;
	int32_t rssi;
	char ssid[EMW3080_IPC_SSID_LEN + 1];
	uint8_t bssid[EMW3080_IPC_MAC_LEN];
	char key[EMW3080_IPC_KEY_LEN + 1];
	int32_t channel;
	emw3080_ipc_security_t security;
} emw3080_ipc_link_info_t;

/* Cmd & rsp types */

/* Common response of commands that only return a status, 0 on success. */
typedef struct __packed {
	int32_t status;
} emw3080_ipc_rsp_status_t;

typedef struct __packed {
	char ssid[EMW3080_IPC_SSID_LEN + 1]; /* empty: scan all networks */
} emw3080_ipc_cmd_wifi_scan_t;

typedef struct __packed {
	uint8_t num;
	emw3080_ipc_ap_info_t ap[];
} emw3080_ipc_rsp_wifi_scan_t;

typedef struct __packed {
	char ssid[EMW3080_IPC_SSID_LEN + 1];
	char key[EMW3080_IPC_KEY_LEN + 1];
	int32_t key_len;
	uint8_t use_attr; /* 1: use attr instead of letting the module detect the AP */
	uint8_t use_ip;   /* 1: static IP on the module, unused in bypass mode */
	emw3080_ipc_connect_attr_t attr;
	emw3080_ipc_ip_attr_t ip;
} emw3080_ipc_cmd_wifi_connect_t;

typedef struct __packed {
	int32_t mode; /* 1: bypass enabled */
} emw3080_ipc_cmd_wifi_bypass_set_t;

/* Followed by data_len bytes of Ethernet frame */
typedef struct __packed {
	int32_t idx;
	uint8_t reserved[16];
	uint16_t data_len;
} emw3080_ipc_cmd_wifi_bypass_out_t;

typedef struct __packed {
	int32_t status;
	emw3080_ipc_link_info_t info;
} emw3080_ipc_rsp_wifi_get_linkinfo_t;

#endif /* ZEPHYR_DRIVERS_WIFI_EMW3080_API_H */
