/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_ST67W611M1_DRV_H_
#define ZEPHYR_DRIVERS_WIFI_ST67W611M1_DRV_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>

#include <modem_context.h>
#include <modem_cmd_handler.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ST67W611M1_ETH_MTU 1500

#define ST67W611M1_AT_CMD_MAX_LEN 256

#define ST67W611M1_AT_CMD_TIMEOUT        K_SECONDS(5)
#define ST67W611M1_INIT_TIMEOUT          K_SECONDS(10)
#define ST67W611M1_SCAN_TIMEOUT          K_SECONDS(10)
#define ST67W611M1_NET_PKT_ALLOC_TIMEOUT K_SECONDS(5)

#define ST67W611M1_CWNETMODE_T01_VAL 1
#define ST67W611M1_CWNETMODE_T02_VAL 0

/*
 * Changes made to this AT command require corresponding changes to the on_cmd_cwlap() parsing
 * function.
 */
#define ST67W611M1_CWLAPOPT_CMD                                                                    \
	"AT+CWLAPOPT=1,31,-100,255," STRINGIFY(CONFIG_ST67W611M1_SCAN_RESULT_MAX_NUMBER) "\r\n"

#define CONN_CMD_MAX_LEN                                                                           \
	(sizeof("AT+CWJAP=\"\",\"\"\r\n") + WIFI_SSID_MAX_LEN * 2 + WIFI_PSK_MAX_LEN * 2)

#define ST67W611M1_CWLAP_MAC_ADDR_STR_LEN 18

#define ST67W611M1_WIFI_STATUS_CONN_WRONG_PASSWORD   2
#define ST67W611M1_WIFI_STATUS_CONN_WRONG_PASSWORD_2 7
#define ST67W611M1_WIFI_STATUS_CONN_TIMEOUT          11
#define ST67W611M1_WIFI_STATUS_CONN_AP_NOT_FOUND     12

#define ST67W611M1_WIFI_REASON_DISCONN_AP_LEAVING   7
#define ST67W611M1_WIFI_REASON_DISCONN_AP_LEAVING_2 16
#define ST67W611M1_WIFI_REASON_DISCONN_USER_REQUEST 19

static const enum wifi_security_type st67w611m1_ap_security_type[] = {
	WIFI_SECURITY_TYPE_NONE,
	WIFI_SECURITY_TYPE_WEP,
	WIFI_SECURITY_TYPE_WPA_PSK,
	WIFI_SECURITY_TYPE_PSK,
	WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL,
	WIFI_SECURITY_TYPE_EAP,
	WIFI_SECURITY_TYPE_SAE,
	WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL};

enum st67_sta_current_state {
	ST67_DISCONNECTED = 0,
	ST67_CONNECTING,
	ST67_CONNECTED,
};

struct st67_driver_data {
	struct net_if *net_iface;

	struct modem_context mctx;
	struct modem_cmd_handler_data cmd_handler_data;
	uint8_t cmd_match_buf[ST67W611M1_AT_CMD_MAX_LEN];

	char conn_cmd[CONN_CMD_MAX_LEN];

	scan_result_cb_t scan_cb;

	struct k_work_q workq;
	struct k_work init_work;
	struct k_work scan_work;
	struct k_work connect_work;
	struct k_work disconnect_work;

	struct k_sem sem_st67_init_over;
	struct k_sem sem_cmd_response_wait;
	struct k_sem sem_wifi_scan_done_wait;
	struct k_sem sem_rx_wait;

	uint8_t sta_mac_addr[NET_ETH_ADDR_LEN];

	enum st67_sta_current_state sta_current_state;

	bool is_supported_st67_firmware_detected;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_WIFI_ST67W611M1_DRV_H_ */
