/** @file
 * @brief UDP packet helpers.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_udp, CONFIG_NET_UDP_LOG_LEVEL);

#include "net_private.h"
#include "udp_internal.h"
#include "net_stats.h"

#define PKT_WAIT_TIME K_SECONDS(1)

int net_udp_create(struct net_pkt *pkt, uint16_t src_port, uint16_t dst_port)
{
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	struct net_udp_hdr *udp_hdr;

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);
	if (!udp_hdr) {
		return -ENOBUFS;
	}

	udp_hdr->src_port = src_port;
	udp_hdr->dst_port = dst_port;
	udp_hdr->len      = 0U;
	udp_hdr->chksum   = 0U;

	return net_pkt_set_data(pkt, &udp_access);
}

int net_udp_finalize(struct net_pkt *pkt, bool force_chksum)
{
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	struct net_udp_hdr *udp_hdr;
	uint16_t length = 0;
	enum net_if_checksum_type type = net_pkt_family(pkt) == AF_INET6 ?
		NET_IF_CHECKSUM_IPV6_UDP : NET_IF_CHECKSUM_IPV4_UDP;

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);
	if (!udp_hdr) {
		return -ENOBUFS;
	}

	length = net_pkt_get_len(pkt) - net_pkt_ip_hdr_len(pkt) -
		 net_pkt_ip_opts_len(pkt);

	udp_hdr->len = htons(length);

	if (net_if_need_calc_tx_checksum(net_pkt_iface(pkt), type) || force_chksum) {
		udp_hdr->chksum = net_calc_chksum_udp(pkt);
		net_pkt_set_chksum_done(pkt, true);
	}

	return net_pkt_set_data(pkt, &udp_access);
}

struct net_udp_hdr *net_udp_get_hdr(struct net_pkt *pkt,
				    struct net_udp_hdr *hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(udp_access, struct net_udp_hdr);
	struct net_pkt_cursor backup;
	struct net_udp_hdr *udp_hdr;
	bool overwrite;

	udp_access.data = hdr;

	overwrite = net_pkt_is_being_overwritten(pkt);
	net_pkt_set_overwrite(pkt, true);

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
			 net_pkt_ip_opts_len(pkt))) {
		udp_hdr = NULL;
		goto out;
	}

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);

out:
	net_pkt_cursor_restore(pkt, &backup);
	net_pkt_set_overwrite(pkt, overwrite);

	return udp_hdr;
}

struct net_udp_hdr *net_udp_set_hdr(struct net_pkt *pkt,
				    struct net_udp_hdr *hdr)
{
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	struct net_pkt_cursor backup;
	struct net_udp_hdr *udp_hdr;
	bool overwrite;

	overwrite = net_pkt_is_being_overwritten(pkt);
	net_pkt_set_overwrite(pkt, true);

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
			 net_pkt_ip_opts_len(pkt))) {
		udp_hdr = NULL;
		goto out;
	}

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);
	if (!udp_hdr) {
		goto out;
	}

	memcpy(udp_hdr, hdr, sizeof(struct net_udp_hdr));

	net_pkt_set_data(pkt, &udp_access);
out:
	net_pkt_cursor_restore(pkt, &backup);
	net_pkt_set_overwrite(pkt, overwrite);

	return udp_hdr == NULL ? NULL : hdr;
}

int net_udp_register(uint8_t family,
		     const struct sockaddr *remote_addr,
		     const struct sockaddr *local_addr,
		     uint16_t remote_port,
		     uint16_t local_port,
		     struct net_context *context,
		     net_conn_cb_t cb,
		     void *user_data,
		     struct net_conn_handle **handle)
{
	return net_conn_register(IPPROTO_UDP, family, remote_addr, local_addr,
				 remote_port, local_port, context, cb,
				 user_data, handle);
}

int net_udp_unregister(struct net_conn_handle *handle)
{
	return net_conn_unregister(handle);
}

struct net_udp_hdr *net_udp_input(struct net_pkt *pkt,
				  struct net_pkt_data_access *udp_access)
{
	struct net_udp_hdr *udp_hdr;
	enum net_if_checksum_type type = net_pkt_family(pkt) == AF_INET6 ?
		NET_IF_CHECKSUM_IPV6_UDP : NET_IF_CHECKSUM_IPV4_UDP;

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, udp_access);
	if (!udp_hdr || net_pkt_set_data(pkt, udp_access)) {
		NET_DBG("DROP: corrupted header");
		goto drop;
	}

	if (ntohs(udp_hdr->len) != (net_pkt_get_len(pkt) -
				    net_pkt_ip_hdr_len(pkt) -
				    net_pkt_ip_opts_len(pkt))) {
		NET_DBG("DROP: Invalid hdr length");
		goto drop;
	}

	if (IS_ENABLED(CONFIG_NET_UDP_CHECKSUM) &&
	    (net_if_need_calc_rx_checksum(net_pkt_iface(pkt), type) ||
	     net_pkt_is_ip_reassembled(pkt))) {
		if (!udp_hdr->chksum) {
			if (IS_ENABLED(CONFIG_NET_UDP_MISSING_CHECKSUM) &&
			    net_pkt_family(pkt) == AF_INET) {
				goto out;
			}

			goto drop;
		}

		if (net_calc_verify_chksum_udp(pkt) != 0U) {
			NET_DBG("DROP: checksum mismatch");
			goto drop;
		}
	}
out:
	return udp_hdr;
drop:
	net_stats_update_udp_chkerr(net_pkt_iface(pkt));
	return NULL;
}
