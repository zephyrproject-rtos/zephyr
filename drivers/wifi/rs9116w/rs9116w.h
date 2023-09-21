/*
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_RS9116W_RS9116W_H_
#define ZEPHYR_DRIVERS_WIFI_RS9116W_RS9116W_H_

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

/* Undef macros before redefining to eliminate warnings */
#undef AF_INET
#undef AF_INET6
#undef AF_UNSPEC
#undef PF_INET
#undef PF_INET6
#undef TCP_NODELAY
#undef IP_TOS
#ifndef ntohs
#define reset_ntohs
#endif
#include <rsi_socket.h>
#ifdef reset_ntohs
#undef ntohs
#undef ntohl
#undef htons
#undef htonl
#endif
#include <rsi_wlan_apis.h>

/*  NOTE: Zephyr defines AF_INET as 1 and AF_INET6 as 2 */
#define RS_AF_INET  2
#define RS_AF_INET6 3

#define MAX_PER_PACKET_SIZE 1500
struct rs9116w_device {
	struct net_if *net_iface; /* ptr to the net_iface */
	struct spi_dt_spec spi;
	uint8_t fw_version[20];
	char mac[6];
#if CONFIG_NET_IPV4
	bool has_ipv4;
#endif
#if CONFIG_NET_IPV6
	bool has_ipv6;
#endif
	rsi_rsp_scan_t scan_results;
};

struct rs9116w_device *rs9116w_by_iface_idx(uint8_t iface);

int rs9116w_offload_init(struct rs9116w_device *rs9116w);

#if defined(CONFIG_NET_SOCKETS_OFFLOAD)
int rs9116w_socket_offload_init(void);
int rs9116w_socket_create(int family, int type, int proto);
#endif
#endif
