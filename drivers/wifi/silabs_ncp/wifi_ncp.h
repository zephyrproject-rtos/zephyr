/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_SILABS_NCP_WIFI_NCP_H_
#define ZEPHYR_DRIVERS_WIFI_SILABS_NCP_WIFI_NCP_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/util.h>

#include <drivers/driver_zephyr.h>

#include "sl_cpc.h"

/* CPC endpoints of the Wi-Fi station. Provisional: the identifiers are not
 * assigned yet.
 */
#define WIFI_NCP_DATA_EP_ID 12
#define WIFI_NCP_MGMT_EP_ID 11

/* Largest Ethernet frame, as a CPC payload */
#define WIFI_NCP_DATA_MTU ROUND_UP(NET_ETH_MAX_FRAME_SIZE, SL_CPC_BUF_MIN_ALIGNMENT)
/* Largest frame exchanged with the network processor */
#define WIFI_NCP_MGMT_MTU 2048

/* Receive buffer handed to CPC */
struct wifi_ncp_data_rx_slot {
	sl_cpc_buf_t buf;
	uint8_t data[WIFI_NCP_DATA_MTU] __aligned(SL_CPC_BUF_MIN_ALIGNMENT);
};

struct wifi_ncp_mgmt_rx_slot {
	sl_cpc_buf_t buf;
	uint8_t data[WIFI_NCP_MGMT_MTU] __aligned(SL_CPC_BUF_MIN_ALIGNMENT);
};

/* Transmit context, alive until the CPC send-done event */
struct wifi_ncp_data_tx_slot {
	sl_cpc_buf_t buf;
	sl_cpc_frame_t frame;
	uint8_t data[WIFI_NCP_DATA_MTU] __aligned(SL_CPC_BUF_MIN_ALIGNMENT);
};

/* State shared with the Zephyr supplicant */
struct wifi_ncp_supp {
	struct zep_drv_if_ctx *if_ctx;
	struct zep_wpa_supp_dev_callbk_fns cb;
};

struct wifi_ncp_config {
	const struct device *parent;
	struct net_eth_mac_config mac;
	struct k_mem_slab *data_tx_slab;
};

struct wifi_ncp_data {
	const struct device *dev;
	struct net_if *iface;
	/* Both endpoints are connected once the device is ready */
	sl_cpc_ep_t data_ep;
	sl_cpc_ep_t mgmt_ep;
	struct k_sem connected;
	struct wifi_ncp_supp supp;
	struct wifi_ncp_data_rx_slot data_rx[CONFIG_WIFI_SILABS_NCP_DATA_RX_COUNT];
	struct wifi_ncp_mgmt_rx_slot mgmt_rx[CONFIG_WIFI_SILABS_NCP_MGMT_RX_COUNT];
};

#endif /* ZEPHYR_DRIVERS_WIFI_SILABS_NCP_WIFI_NCP_H_ */
