/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_WIFI_W91_H_
#define ZEPHYR_DRIVERS_WIFI_WIFI_W91_H_

#include <zephyr/sys/util.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/ethernet.h>
#include <ipc/ipc_based_driver.h>

#define W91_WIFI_L2_CTX_TYPE        void*

enum {
	IPC_DISPATCHER_WIFI_INIT = IPC_DISPATCHER_WIFI,
	IPC_DISPATCHER_WIFI_SCAN,
	IPC_DISPATCHER_WIFI_CONNECT,
	IPC_DISPATCHER_WIFI_DISCONNECT,
	IPC_DISPATCHER_WIFI_AP_ENABLE,
	IPC_DISPATCHER_WIFI_AP_DISABLE,
	IPC_DISPATCHER_WIFI_L2_DATA,
	IPC_DISPATCHER_WIFI_EVENT,
};

struct ipc_msg {
	uint32_t id;
	uint8_t data[NET_ETH_MAX_FRAME_SIZE];
} __packed;

struct wifi_w91_data_base {
	uint8_t mac[WIFI_MAC_ADDR_LEN];
	scan_result_cb_t scan_cb;
	uint8_t state;
	struct net_if *iface;
	struct wifi_iface_status if_state;
};

struct wifi_w91_data_l2 {
	struct ipc_msg ipc_tx;
};

struct wifi_w91_data {
	struct ipc_based_driver ipc;
	struct wifi_w91_data_base base;
	struct wifi_w91_data_l2 l2;
};

struct wifi_w91_config {
	uint8_t instance_id;
	struct k_thread *thread;
	k_thread_stack_t *thread_stack;
	struct k_msgq *thread_msgq;
};

#endif /* ZEPHYR_DRIVERS_WIFI_WIFI_W91_H_ */
