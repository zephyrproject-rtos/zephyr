/** @file
 * @brief UDP packet helpers.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_udp, CONFIG_NET_UDP_LOG_LEVEL);

#include "net_private.h"
#include "udp_internal.h"
#include "net_stats.h"

#define PKT_WAIT_TIME K_SECONDS(1)

int net_udp_create(struct net_pkt *pkt, u16_t src_port, u16_t dst_port)
{
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	struct net_udp_hdr *udp_hdr;

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data_new(pkt, &udp_access);
	if (!udp_hdr) {
		return -ENOBUFS;
	}

	udp_hdr->src_port = src_port;
	udp_hdr->dst_port = dst_port;
	udp_hdr->len      = 0;
	udp_hdr->chksum   = 0;

	return net_pkt_set_data(pkt, &udp_access);
}

int net_udp_finalize(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	struct net_udp_hdr *udp_hdr;
	u16_t length;

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data_new(pkt, &udp_access);
	if (!udp_hdr) {
		return -ENOBUFS;
	}

	length = net_pkt_get_len(pkt) -
		net_pkt_ip_hdr_len(pkt) -
		net_pkt_ipv6_ext_len(pkt);

	udp_hdr->len    = htons(length);
	udp_hdr->chksum = net_calc_chksum_udp(pkt);

	return net_pkt_set_data(pkt, &udp_access);
}

struct net_pkt *net_udp_insert(struct net_pkt *pkt,
			       u16_t offset,
			       u16_t src_port,
			       u16_t dst_port)
{
	struct net_buf *frag, *prev, *udp;
	u16_t pos;

	frag = net_frag_get_pos(pkt, offset, &pos);
	if (!frag && pos == 0xffff) {
		NET_DBG("Offset %d out of pkt len %zd",
			offset, net_pkt_get_len(pkt));
		return NULL;
	}

	/* We can only insert the UDP header between existing two
	 * fragments.
	 */
	if (frag && pos != 0) {
		NET_DBG("Cannot insert UDP data into offset %d", offset);
		return NULL;
	}

	if (pkt->frags != frag) {
		struct net_buf *tmp = pkt->frags;

		prev = NULL;

		while (tmp->frags) {
			if (tmp->frags == frag) {
				prev = tmp;
				break;
			}

			tmp = tmp->frags;
		}
	} else {
		prev = pkt->frags;
	}

	if (!prev) {
		goto fail;
	}

	udp = net_pkt_get_frag(pkt, PKT_WAIT_TIME);
	if (!udp) {
		goto fail;
	}

	/* Source and destination ports are already in network byte order */
	net_buf_add_mem(udp, &src_port, sizeof(src_port));
	net_buf_add_mem(udp, &dst_port, sizeof(dst_port));

	net_buf_add_be16(udp, net_pkt_get_len(pkt) -
			 net_pkt_ip_hdr_len(pkt) -
			 net_pkt_ipv6_ext_len(pkt) +
			 sizeof(struct net_udp_hdr));

	net_buf_add_be16(udp, 0); /* chksum */

	net_buf_frag_insert(prev, udp);

	frag = net_frag_get_pos(pkt, net_pkt_ip_hdr_len(pkt) +
				net_pkt_ipv6_ext_len(pkt) +
				sizeof(struct net_udp_hdr),
				&pos);
	if (frag) {
		net_pkt_set_appdata(pkt, frag->data + pos);
	}

	return pkt;

fail:
	NET_DBG("Cannot insert UDP header into %p", pkt);
	return NULL;
}

struct net_buf *net_udp_set_chksum(struct net_pkt *pkt, struct net_buf *frag)
{
	struct net_udp_hdr *hdr;
	u16_t chksum = 0U;
	u16_t pos;

	hdr = net_pkt_udp_data(pkt);
	if (net_udp_header_fits(pkt, hdr)) {
		hdr->chksum = 0;
		hdr->chksum = net_calc_chksum_udp(pkt);

		return frag;
	}

	/* We need to set the checksum to 0 first before the calc */
	frag = net_pkt_write(pkt, frag,
			     net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ipv6_ext_len(pkt) +
			     2 + 2 + 2 /* src + dst + len */,
			     &pos, sizeof(chksum), (u8_t *)&chksum,
			     PKT_WAIT_TIME);

	chksum = net_calc_chksum_udp(pkt);

	frag = net_pkt_write(pkt, frag, pos - 2, &pos, sizeof(chksum),
			     (u8_t *)&chksum, PKT_WAIT_TIME);

	NET_ASSERT(frag);

	return frag;
}

struct net_udp_hdr *net_udp_get_hdr(struct net_pkt *pkt,
				    struct net_udp_hdr *hdr)
{
	struct net_udp_hdr *udp_hdr;
	struct net_buf *frag;
	u16_t pos;

	udp_hdr = net_pkt_udp_data(pkt);
	if (net_udp_header_fits(pkt, udp_hdr)) {
		return udp_hdr;
	}

	frag = net_frag_read(pkt->frags, net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ipv6_ext_len(pkt),
			     &pos, sizeof(hdr->src_port),
			     (u8_t *)&hdr->src_port);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->dst_port),
			     (u8_t *)&hdr->dst_port);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->len),
			     (u8_t *)&hdr->len);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->chksum),
			     (u8_t *)&hdr->chksum);
	if (!frag) {
		NET_ASSERT(frag);
		return NULL;
	}

	return hdr;
}

struct net_udp_hdr *net_udp_set_hdr(struct net_pkt *pkt,
				    struct net_udp_hdr *hdr)
{
	struct net_buf *frag;
	u16_t pos;

	if (net_udp_header_fits(pkt, hdr)) {
		return hdr;
	}

	frag = net_pkt_write(pkt, pkt->frags, net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ipv6_ext_len(pkt),
			     &pos, sizeof(hdr->src_port),
			     (u8_t *)&hdr->src_port, PKT_WAIT_TIME);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->dst_port),
			     (u8_t *)&hdr->dst_port, PKT_WAIT_TIME);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->len),
			     (u8_t *)&hdr->len, PKT_WAIT_TIME);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->chksum),
			     (u8_t *)&hdr->chksum, PKT_WAIT_TIME);

	if (!frag) {
		NET_ASSERT(frag);
		return NULL;
	}

	return hdr;
}

int net_udp_register(u8_t family,
		     const struct sockaddr *remote_addr,
		     const struct sockaddr *local_addr,
		     u16_t remote_port,
		     u16_t local_port,
		     net_conn_cb_t cb,
		     void *user_data,
		     struct net_conn_handle **handle)
{
	return net_conn_register(IPPROTO_UDP, family, remote_addr, local_addr,
				 remote_port, local_port, cb, user_data,
				 handle);
}

int net_udp_unregister(struct net_conn_handle *handle)
{
	return net_conn_unregister(handle);
}

struct net_udp_hdr *net_udp_input(struct net_pkt *pkt,
				  struct net_pkt_data_access *udp_access)
{
	struct net_udp_hdr *udp_hdr;

	if (IS_ENABLED(CONFIG_NET_UDP_CHECKSUM) &&
	    net_if_need_calc_rx_checksum(net_pkt_iface(pkt)) &&
	    net_calc_chksum_udp(pkt) != 0) {
		NET_DBG("DROP: checksum mismatch");
		goto drop;
	}

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data_new(pkt, udp_access);
	if (udp_hdr && !net_pkt_set_data(pkt, udp_access)) {
		return udp_hdr;
	}

drop:
	net_stats_update_udp_chkerr(net_pkt_iface(pkt));
	return NULL;
}
