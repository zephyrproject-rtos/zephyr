/** @file
 * @brief Generic raw socket connection related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_conn_raw
#define NET_LOG_LEVEL CONFIG_NET_CONN_LOG_LEVEL

#include <errno.h>
#include <misc/util.h>

#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/ethernet.h>

#include "net_private.h"
#include "connection_raw.h"
#include "net_stats.h"

/** Is this connection used or not */
#define NET_CONN_IN_USE BIT(0)

static struct net_conn_raw conns[CONFIG_NET_MAX_CONN_RAW];
static int register_count;

int net_conn_raw_unregister(struct net_conn_raw_handle *handle)
{
	struct net_conn_raw *conn = (struct net_conn_raw *)handle;

	if (conn < &conns[0] || conn > &conns[CONFIG_NET_MAX_CONN_RAW]) {
		return -EINVAL;
	}

	if (!(conn->flags & NET_CONN_IN_USE)) {
		return -ENOENT;
	}

	NET_DBG("[%zu] connection handler %p removed", conn - conns, conn);

	(void)memset(conn, 0, sizeof(*conn));

	register_count--;
	if (register_count < 0) {
		register_count = 0;
	}

	return 0;
}

int net_conn_raw_change_callback(struct net_conn_raw_handle *handle,
				 net_conn_raw_cb_t cb, void *user_data)
{
	struct net_conn_raw *conn = (struct net_conn_raw *)handle;

	if (conn < &conns[0] || conn > &conns[CONFIG_NET_MAX_CONN_RAW]) {
		return -EINVAL;
	}

	if (!(conn->flags & NET_CONN_IN_USE)) {
		return -ENOENT;
	}

	NET_DBG("[%zu] connection handler %p changed callback", conn - conns,
		conn);

	conn->cb = cb;
	conn->user_data = user_data;

	return 0;
}

int net_conn_raw_register(u16_t proto,
			  net_conn_raw_cb_t cb,
			  void *user_data,
			  struct net_conn_raw_handle **handle)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_CONN_RAW; i++) {
		if (conns[i].flags & NET_CONN_IN_USE) {
			continue;
		}

		conns[i].flags |= NET_CONN_IN_USE;
		conns[i].cb = cb;
		conns[i].user_data = user_data;
		conns[i].proto = proto;

		if (handle) {
			*handle = (struct net_conn_raw_handle *)&conns[i];
		}

		register_count++;

		return 0;
	}

	return -ENOENT;
}

void net_conn_raw_input(u16_t proto, struct net_pkt *pkt)
{
	struct net_if *pkt_iface = net_pkt_iface(pkt);
	int i, consumed = 0, dropped = 0;
	enum net_verdict verdict;

	if (register_count == 0) {
		return;
	}

	for (i = 0; i < CONFIG_NET_MAX_CONN_RAW; i++) {
		if (!(conns[i].flags & NET_CONN_IN_USE)) {
			continue;
		}

		if (conns[i].proto != ntohs(proto) &&
		    conns[i].proto != NET_ETH_PTYPE_ALL) {
			continue;
		}

		NET_DBG("[%d] match found cb %p ud %p proto 0x%04x",
			i, conns[i].cb, conns[i].user_data, conns[i].proto);

		/* We ref the packet as it is now application responsibility
		 * to free it after it has worked with it. If the app returns
		 * NET_DROP, then we unref the packet here too as it means that
		 * the application was not interested in about the packet.
		 */
		verdict = conns[i].cb(&conns[i], net_pkt_ref(pkt),
				      conns[i].user_data);
		if (verdict == NET_DROP) {
			net_pkt_unref(pkt);
			dropped++;
		} else {
			consumed++;
		}
	}

	if (dropped > 0) {
		net_stats_update_per_l2_proto_drop(pkt_iface, proto, dropped);
	}

	if (consumed > 0) {
		net_stats_update_per_l2_proto_recv(pkt_iface, proto, consumed);
	}
}

void net_conn_raw_foreach(net_conn_raw_foreach_cb_t cb, void *user_data)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_CONN_RAW; i++) {
		if (!(conns[i].flags & NET_CONN_IN_USE)) {
			continue;
		}

		cb(&conns[i], user_data);
	}
}
