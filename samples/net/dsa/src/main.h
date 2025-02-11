/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DSA_SAMPLE__
#define __DSA_SAMPLE__

#include <zephyr/kernel.h>
#include <errno.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>

#define MCAST_DEST_MAC0 0x01
#define MCAST_DEST_MAC1 0x80
#define MCAST_DEST_MAC2 0xc2
#define MCAST_DEST_MAC3 0x00
#define MCAST_DEST_MAC4 0x00
#define MCAST_DEST_MAC5 0x03

#define RECV_BUFFER_SIZE 1280
#define ETH_ALEN 6
#define PACKET_LEN 128

extern struct ud user_data_ifaces;

struct eth_addr {
	uint8_t addr[ETH_ALEN]; /* origin hardware address */
};

struct instance_data {
	char *if_name;
	int sock;
	char recv_buffer[RECV_BUFFER_SIZE];
};

/* User data for the interface callback */
struct ud {
	struct net_if *lan[3];
	struct net_if *master;
};

static inline bool check_ll_ether_addr(const uint8_t *a, const uint8_t *b)
{
	return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2]) |
		(a[3] ^ b[3]) | (a[4] ^ b[4]) | (a[5] ^ b[5])) == 0;
}

static inline void dsa_buf_write_be16(uint16_t tl, uint8_t **p)
{
	uint8_t *v = (uint8_t *) &tl;
	**p = v[1];
	(*p)++;
	**p = v[0];
	(*p)++;
}

int start_slave_port_packet_socket(struct net_if *iface,
				   struct instance_data *pd);

enum net_verdict dsa_ll_addr_switch_cb(struct net_if *iface,
				       struct net_pkt *pkt);

#define CMD_DISCOVER 0
#define CMD_ACK      1
#define DSA_STACK_SIZE 4096
#define DSA_PRIORITY      5
#define DSA_THREAD_START_DELAY 4000

#define DSA_THREAD(ID, FN_RECV, FN_SEND)                                       \
	static void dsa_thread_##ID(void *t1, void *t2, void *t3);             \
	K_THREAD_DEFINE(dsa_tid_##ID, DSA_STACK_SIZE,                          \
		dsa_thread_##ID, NULL, NULL, NULL,                             \
			DSA_PRIORITY, 0, DSA_THREAD_START_DELAY);              \
									       \
	void dsa_thread_##ID(void *t1, void *t2, void *t3)                     \
	{                                                                      \
		int origin_port, ret;                                          \
		uint16_t seq;                                                  \
		struct eth_addr origin_addr;                                   \
		struct instance_data data;                                     \
		struct net_if *iface;                                          \
									       \
		iface = user_data_ifaces.lan[ID-1];                            \
									       \
		data.if_name = "lan"#ID;                                       \
		ret = start_slave_port_packet_socket(iface, &data);            \
		if (ret < 0) {                                                 \
			LOG_ERR("start_slave_port_packet_socket failed %d",    \
				ret);                                          \
			return;                                                \
		}                                                              \
		dsa_register_recv_callback(iface,                              \
						dsa_ll_addr_switch_cb);        \
									       \
		LOG_INF("DSA -> eth/lan"#ID" idx: %d sock: %d",                \
			net_if_get_by_iface(iface), data.sock);                \
		do {                                                           \
			ret = FN_RECV(iface, &data, &seq,                      \
					&origin_port, &origin_addr);           \
			if (ret) {                                             \
				break;                                         \
			}                                                      \
			ret = FN_SEND(iface, &data,                            \
					seq, 0, origin_port, CMD_ACK,          \
					&origin_addr);                         \
			if (ret) {                                             \
				break;                                         \
			}                                                      \
		} while (true);                                                \
	}

#endif /* __DSA_SAMPLE__ */
