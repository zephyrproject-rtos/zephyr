/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_WIFI_W91_H_
#define ZEPHYR_DRIVERS_WIFI_WIFI_W91_H_

#include <zephyr/sys/util.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/ethernet.h>
#include <ipc/ipc_based_driver.h>

#define W91_WIFI_L2_CTX_TYPE        void*

enum {
	IPC_DISPATCHER_WIFI_L2_DATA = IPC_DISPATCHER_WIFI,
};

__packed struct ipc_msg {
	uint32_t id;
	uint8_t data[NET_ETH_MTU];
};

struct wifi_w91_data_l2 {
	uint8_t mac[WIFI_MAC_ADDR_LEN];
	struct ipc_msg ipc_tx;
};

struct wifi_w91_data {
	struct ipc_based_driver ipc;
	struct wifi_w91_data_l2 l2;
};

struct wifi_w91_config {
	uint8_t instance_id;
};

NET_L2_DECLARE_PUBLIC(W91_WIFI_L2);

#endif /* ZEPHYR_DRIVERS_WIFI_WIFI_W91_H_ */
