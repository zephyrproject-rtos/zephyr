/**
 * Copyright (c) 2018 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_ESWIFI_ESWIFI_OFFLOAD_H_
#define ZEPHYR_DRIVERS_WIFI_ESWIFI_ESWIFI_OFFLOAD_H_

#include <net/net_offload.h>

#include "eswifi.h"

#define ESWIFI_OFFLOAD_MAX_SOCKETS 4

enum eswifi_transport_type {
	ESWIFI_TRANSPORT_TCP,
	ESWIFI_TRANSPORT_UDP,
	ESWIFI_TRANSPORT_UDP_LITE,
	ESWIFI_TRANSPORT_TCP_SSL,
};

enum eswifi_socket_state {
	ESWIFI_SOCKET_STATE_NONE,
	ESWIFI_SOCKET_STATE_CONNECTED,
};

struct eswifi_off_socket {
	u8_t index;
	enum eswifi_transport_type type;
	enum eswifi_socket_state state;
	struct net_context *context;
	net_context_recv_cb_t recv_cb;
	void *user_data;
};

#endif
