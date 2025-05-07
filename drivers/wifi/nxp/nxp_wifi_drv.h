/**
 * Copyright 2023-2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file nxp_wifi_drv.h
 * Shim layer between wifi driver connection manager and zephyr
 * Wi-Fi L2 layer
 */

#ifndef ZEPHYR_DRIVERS_WIFI_NNP_WIFI_DRV_H_
#define ZEPHYR_DRIVERS_WIFI_NXP_WIFI_DRV_H_

#include <zephyr/kernel.h>
#include <stdio.h>
#ifdef CONFIG_SDIO_STACK
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sdio.h>
#endif
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>

#include "wlan_bt_fw.h"
#include "wlan.h"
#include "wm_net.h"
#if defined(CONFIG_NXP_WIFI_SHELL)
#include "wifi_shell.h"
#endif
#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT)
#include "wifi_nxp.h"
#include "rtos_wpa_supp_if.h"
#endif

#define MAX_DATA_SIZE 1600

#define NXP_WIFI_SYNC_TIMEOUT_MS K_FOREVER

#define NXP_WIFI_UAP_NETWORK_NAME "uap-network"

#define NXP_WIFI_STA_NETWORK_NAME "sta-network"

#define NXP_WIFI_EVENT_BIT(event) (1 << event)

#define NXP_WIFI_SYNC_INIT_GROUP                                                                   \
	NXP_WIFI_EVENT_BIT(WLAN_REASON_INITIALIZED) |                                              \
		NXP_WIFI_EVENT_BIT(WLAN_REASON_INITIALIZATION_FAILED)

#define NXP_WIFI_SYNC_PS_GROUP                                                                     \
	NXP_WIFI_EVENT_BIT(WLAN_REASON_PS_ENTER) | NXP_WIFI_EVENT_BIT(WLAN_REASON_PS_EXIT)

#define NXP_WIFI_SAP_BEACON_PERIOD_DEFAULT 100
#define NXP_WIFI_SAP_DTIM_PERIOD_DEFAULT 1

enum nxp_wifi_ret {
	NXP_WIFI_RET_SUCCESS,
	NXP_WIFI_RET_FAIL,
	NXP_WIFI_RET_NOT_FOUND,
	NXP_WIFI_RET_AUTH_FAILED,
	NXP_WIFI_RET_ADDR_FAILED,
	NXP_WIFI_RET_NOT_CONNECTED,
	NXP_WIFI_RET_NOT_READY,
	NXP_WIFI_RET_TIMEOUT,
	NXP_WIFI_RET_BAD_PARAM,
};

enum nxp_wifi_state {
	NXP_WIFI_NOT_INITIALIZED,
	NXP_WIFI_INITIALIZED,
	NXP_WIFI_STARTED,
};

struct nxp_wifi_dev {
	struct net_if *iface;
	scan_result_cb_t scan_cb;
	struct k_mutex mutex;
};

int nxp_wifi_wlan_event_callback(enum wlan_event_reason reason, void *data);

#if defined(CONFIG_NXP_WIFI_SHELL)
void nxp_wifi_shell_register(struct nxp_wifi_dev *dev);
#else
#define nxp_wifi_shell_register(dev)
#endif

#endif
