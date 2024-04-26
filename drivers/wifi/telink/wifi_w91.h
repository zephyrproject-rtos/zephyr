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

#define W91_WIFI_L2_CTX_TYPE        void*
#define W91_WIFI_L2_MTU             \
	(CONFIG_IPC_SERVICE_ICMSG_CB_BUF_SIZE - sizeof(uint32_t))

__packed struct ipc_msg {
	uint32_t id;
	uint32_t tx_len;
	uint8_t tx[W91_WIFI_L2_MTU];
};

struct wifi_w91_data_l2 {
	uint8_t mac[WIFI_MAC_ADDR_LEN];
	struct ipc_msg ipc_tx;
};

struct wifi_w91_data {
	struct wifi_w91_data_l2 l2;
};

NET_L2_DECLARE_PUBLIC(W91_WIFI_L2);

#endif /* ZEPHYR_DRIVERS_WIFI_WIFI_W91_H_ */
