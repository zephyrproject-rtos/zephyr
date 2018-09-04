/** @file
 * @brief IPv6 Fragment related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_IPV6)
#define SYS_LOG_DOMAIN "net/ipv6-frag"
#define NET_LOG_ENABLED 1

/* By default this prints too much data, set the value to 1 to see
 * neighbor cache contents.
 */
#define NET_DEBUG_NBR 0
#endif

#include <errno.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_stats.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <net/tcp.h>
#include "net_private.h"
#include "connection.h"
#include "icmpv6.h"
#include "udp_internal.h"
#include "tcp_internal.h"
#include "ipv6.h"
#include "nbr.h"
#include "6lo.h"
#include "route.h"
#include "rpl.h"
#include "net_stats.h"

/* Timeout for various buffer allocations in this file. */
#define NET_BUF_TIMEOUT K_MSEC(50)

#if defined(CONFIG_NET_IPV6_FRAGMENT_TIMEOUT)
#define IPV6_REASSEMBLY_TIMEOUT K_SECONDS(CONFIG_NET_IPV6_FRAGMENT_TIMEOUT)
#else
#define IPV6_REASSEMBLY_TIMEOUT K_SECONDS(5)
#endif /* CONFIG_NET_IPV6_FRAGMENT_TIMEOUT */

#define FRAG_BUF_WAIT K_MSEC(10) /* how long to max wait for a buffer */

static void reassembly_timeout(struct k_work *work);
static bool reassembly_init_done;

static struct net_ipv6_reassembly
reassembly[CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT];

int net_ipv6_find_last_ext_hdr(struct net_pkt *pkt, u16_t *next_hdr_idx,
			       u16_t *last_hdr_idx)
{
	struct net_buf *next_hdr_frag;
	struct net_buf *last_hdr_frag;
	struct net_buf *frag;
	u16_t pkt_offset;
	u16_t offset;
	u16_t length;
	u8_t next_hdr;
	u8_t next;

	if (!pkt || !pkt->frags || !next_hdr_idx || !last_hdr_idx) {
		return -EINVAL;
	}

	next = NET_IPV6_HDR(pkt)->nexthdr;

	/* Initial value if no extension fragments are found */
	*next_hdr_idx = 6;
	*last_hdr_idx = sizeof(struct net_ipv6_hdr);

	/* First check the simplest case where there is no extension headers
	 * in the packet. There cannot be any extensions after the normal or
	 * typical IP protocols
	 */
	if (next == IPPROTO_ICMPV6 || next == IPPROTO_UDP ||
	    next == IPPROTO_TCP || next == NET_IPV6_NEXTHDR_NONE) {
		return 0;
	}

	frag = pkt->frags;
	offset = *last_hdr_idx;
	*next_hdr_idx = *last_hdr_idx;
	next_hdr_frag = last_hdr_frag = frag;

	while (frag) {
		frag = net_frag_read_u8(frag, offset, &offset, &next_hdr);
		if (!frag) {
			goto fail;
		}

		switch (next) {
		case NET_IPV6_NEXTHDR_FRAG:
			frag = net_frag_skip(frag, offset, &offset, 7);
			if (!frag) {
				goto fail;
			}

			break;

		case NET_IPV6_NEXTHDR_HBHO:
			length = 0;
			frag = net_frag_read_u8(frag, offset, &offset,
						(u8_t *)&length);
			if (!frag) {
				goto fail;
			}

			length = length * 8 + 8;

			frag = net_frag_skip(frag, offset, &offset, length - 2);
			if (!frag) {
				goto fail;
			}

			break;

		case NET_IPV6_NEXTHDR_NONE:
		case IPPROTO_ICMPV6:
		case IPPROTO_UDP:
		case IPPROTO_TCP:
			goto out;

		default:
			/* TODO: Add more IPv6 extension headers to check */
			goto fail;
		}

		*next_hdr_idx = *last_hdr_idx;
		next_hdr_frag = last_hdr_frag;

		*last_hdr_idx = offset;
		last_hdr_frag = frag;

		next = next_hdr;
	}

fail:
	return -EINVAL;

out:
	/* Current next_hdr_idx offset is based on respective fragment, but we
	 * need to calculate next_hdr_idx offset based on whole packet.
	 */
	pkt_offset = 0;
	frag = pkt->frags;
	while (frag) {
		if (next_hdr_frag == frag) {
			*next_hdr_idx += pkt_offset;
			break;
		}

		pkt_offset += frag->len;
		frag = frag->frags;
	}

	/* Current last_hdr_idx offset is based on respective fragment, but we
	 * need to calculate last_hdr_idx offset based on whole packet.
	 */
	pkt_offset = 0;
	frag = pkt->frags;
	while (frag) {
		if (last_hdr_frag == frag) {
			*last_hdr_idx += pkt_offset;
			break;
		}

		pkt_offset += frag->len;
		frag = frag->frags;
	}

	return 0;
}


static struct net_ipv6_reassembly *reassembly_get(u32_t id,
						  struct in6_addr *src,
						  struct in6_addr *dst)
{
	int i, avail = -1;

	for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {

		if (k_delayed_work_remaining_get(&reassembly[i].timer) &&
		    reassembly[i].id == id &&
		    net_ipv6_addr_cmp(src, &reassembly[i].src) &&
		    net_ipv6_addr_cmp(dst, &reassembly[i].dst)) {
			return &reassembly[i];
		}

		if (k_delayed_work_remaining_get(&reassembly[i].timer)) {
			continue;
		}

		if (avail < 0) {
			avail = i;
		}
	}

	if (avail < 0) {
		return NULL;
	}

	k_delayed_work_submit(&reassembly[avail].timer,
			      IPV6_REASSEMBLY_TIMEOUT);

	net_ipaddr_copy(&reassembly[avail].src, src);
	net_ipaddr_copy(&reassembly[avail].dst, dst);

	reassembly[avail].id = id;

	return &reassembly[avail];
}

static bool reassembly_cancel(u32_t id,
			      struct in6_addr *src,
			      struct in6_addr *dst)
{
	int i, j;

	NET_DBG("Cancel 0x%x", id);

	for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {
		s32_t remaining;

		if (reassembly[i].id != id ||
		    !net_ipv6_addr_cmp(src, &reassembly[i].src) ||
		    !net_ipv6_addr_cmp(dst, &reassembly[i].dst)) {
			continue;
		}

		remaining = k_delayed_work_remaining_get(&reassembly[i].timer);
		if (remaining) {
			k_delayed_work_cancel(&reassembly[i].timer);
		}

		NET_DBG("IPv6 reassembly id 0x%x remaining %d ms",
			reassembly[i].id, remaining);

		reassembly[i].id = 0;

		for (j = 0; j < NET_IPV6_FRAGMENTS_MAX_PKT; j++) {
			if (!reassembly[i].pkt[j]) {
				continue;
			}

			NET_DBG("[%d] IPv6 reassembly pkt %p %zd bytes data",
				j, reassembly[i].pkt[j],
				net_pkt_get_len(reassembly[i].pkt[j]));

			net_pkt_unref(reassembly[i].pkt[j]);
			reassembly[i].pkt[j] = NULL;
		}

		return true;
	}

	return false;
}

static void reassembly_info(char *str, struct net_ipv6_reassembly *reass)
{
	int i, len;

	for (i = 0, len = 0; i < NET_IPV6_FRAGMENTS_MAX_PKT; i++) {
		if (reass->pkt[i]) {
			len += net_pkt_get_len(reass->pkt[i]);
		}
	}

	NET_DBG("%s id 0x%x src %s dst %s remain %d ms len %d", str, reass->id,
		net_sprint_ipv6_addr(&reass->src),
		net_sprint_ipv6_addr(&reass->dst),
		k_delayed_work_remaining_get(&reass->timer), len);
}

static void reassembly_timeout(struct k_work *work)
{
	struct net_ipv6_reassembly *reass =
		CONTAINER_OF(work, struct net_ipv6_reassembly, timer);

	reassembly_info("Reassembly cancelled", reass);

	reassembly_cancel(reass->id, &reass->src, &reass->dst);
}

static void reassemble_packet(struct net_ipv6_reassembly *reass)
{
	struct net_pkt *pkt;
	struct net_buf *last;
	struct net_buf *frag;
	u8_t next_hdr;
	int i, len, ret;
	u16_t pos;

	k_delayed_work_cancel(&reass->timer);

	NET_ASSERT(reass->pkt[0]);

	last = net_buf_frag_last(reass->pkt[0]->frags);

	/* We start from 2nd packet which is then appended to
	 * the first one.
	 */
	for (i = 1; i < NET_IPV6_FRAGMENTS_MAX_PKT; i++) {
		int removed_len;
		int ret;

		pkt = reass->pkt[i];

		/* Get rid of IPv6 and fragment header which are at
		 * the beginning of the fragment.
		 */
		removed_len = net_pkt_ipv6_fragment_start(pkt) +
			      sizeof(struct net_ipv6_frag_hdr);

		NET_DBG("Removing %d bytes from start of pkt %p",
			removed_len, pkt->frags);

		ret = net_pkt_pull(pkt, 0, removed_len);
		if (ret) {
			NET_ERR("Failed to pull headers");
			NET_ASSERT(ret != 0);
		}

		/* Attach the data to previous pkt */
		last->frags = pkt->frags;
		last = net_buf_frag_last(pkt->frags);

		pkt->frags = NULL;
		reass->pkt[i] = NULL;

		net_pkt_unref(pkt);
	}

	pkt = reass->pkt[0];
	reass->pkt[0] = NULL;

	/* Next we need to strip away the fragment header from the first packet
	 * and set the various pointers and values in packet.
	 */

	frag = net_frag_read_u8(pkt->frags, net_pkt_ipv6_fragment_start(pkt),
				&pos, &next_hdr);
	if (!frag && pos == 0xFFFF) {
		NET_ERR("Failed to read next header");
		NET_ASSERT(frag);
	}

	ret = net_pkt_pull(pkt, net_pkt_ipv6_fragment_start(pkt),
			   sizeof(struct net_ipv6_frag_hdr));
	if (ret) {
		NET_ERR("Failed to pull fragmentation header");
		NET_ASSERT(ret);
	}

	/* This one updates the previous header's nexthdr value */
	if (!net_pkt_write_u8_timeout(pkt, pkt->frags,
				      net_pkt_ipv6_hdr_prev(pkt),
				      &pos, next_hdr, NET_BUF_TIMEOUT)) {
		net_pkt_unref(pkt);
		return;
	}

	if (!net_pkt_compact(pkt)) {
		NET_ERR("Cannot compact reassembly packet %p", pkt);
		net_pkt_unref(pkt);
		return;
	}

	/* Fix the total length of the IPv6 packet. */
	len = net_pkt_ipv6_ext_len(pkt);
	if (len > 0) {
		NET_DBG("Old pkt %p IPv6 ext len is %d bytes", pkt, len);
		net_pkt_set_ipv6_ext_len(pkt,
				len - sizeof(struct net_ipv6_frag_hdr));
	}

	len = net_pkt_get_len(pkt) - sizeof(struct net_ipv6_hdr);

	NET_IPV6_HDR(pkt)->len = htons(len);

	NET_DBG("New pkt %p IPv6 len is %d bytes", pkt, len);

	/* We need to use the queue when feeding the packet back into the
	 * IP stack as we might run out of stack if we call processing_data()
	 * directly. As the packet does not contain link layer header, we
	 * MUST NOT pass it to L2 so there will be a special check for that
	 * in process_data() when handling the packet.
	 */
	ret = net_recv_data(net_pkt_iface(pkt), pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}
}

void net_ipv6_frag_foreach(net_ipv6_frag_cb_t cb, void *user_data)
{
	int i;

	for (i = 0; reassembly_init_done &&
		     i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {
		if (!k_delayed_work_remaining_get(&reassembly[i].timer)) {
			continue;
		}

		cb(&reassembly[i], user_data);
	}
}

/* Verify that we have all the fragments received and in correct order.
 */
static bool fragment_verify(struct net_ipv6_reassembly *reass)
{
	u16_t offset;
	int i, prev_len;

	prev_len = net_pkt_get_len(reass->pkt[0]);
	offset = net_pkt_ipv6_fragment_offset(reass->pkt[0]);

	NET_DBG("pkt %p offset %u", reass->pkt[0], offset);

	if (offset != 0) {
		return false;
	}

	for (i = 1; i < NET_IPV6_FRAGMENTS_MAX_PKT; i++) {
		offset = net_pkt_ipv6_fragment_offset(reass->pkt[i]);

		NET_DBG("pkt %p offset %u prev_len %d", reass->pkt[i],
			offset, prev_len);

		if (prev_len < offset) {
			/* Something wrong with the offset value */
			return false;
		}

		prev_len = net_pkt_get_len(reass->pkt[i]);
	}

	return true;
}

static int shift_packets(struct net_ipv6_reassembly *reass, int pos)
{
	int i;

	for (i = pos + 1; i < NET_IPV6_FRAGMENTS_MAX_PKT; i++) {
		if (!reass->pkt[i]) {
			NET_DBG("Moving [%d] %p (offset 0x%x) to [%d]",
				pos, reass->pkt[pos],
				net_pkt_ipv6_fragment_offset(reass->pkt[pos]),
				i);

			/* Do we have enough space in packet array to make
			 * the move?
			 */
			if (((i - pos) + 1) >
			    (NET_IPV6_FRAGMENTS_MAX_PKT - i)) {
				return -ENOMEM;
			}

			memmove(&reass->pkt[i], &reass->pkt[pos],
				sizeof(void *) * (i - pos));

			return 0;
		}
	}

	return -EINVAL;
}

enum net_verdict net_ipv6_handle_fragment_hdr(struct net_pkt *pkt,
					      struct net_buf *frag,
					      int total_len,
					      u16_t buf_offset,
					      u16_t *loc,
					      u8_t nexthdr)
{
	struct net_ipv6_reassembly *reass = NULL;
	u32_t id;
	u16_t offset;
	u16_t flag;
	u8_t more;
	bool found;
	int i;

	if (!reassembly_init_done) {
		/* Static initializing does not work here because of the array
		 * so we must do it at runtime.
		 */
		for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {
			k_delayed_work_init(&reassembly[i].timer,
					    reassembly_timeout);
		}

		reassembly_init_done = true;
	}

	/* Each fragment has a fragment header. */
	frag = net_frag_skip(frag, buf_offset, loc, 1); /* reserved */
	frag = net_frag_read_be16(frag, *loc, loc, &flag);
	frag = net_frag_read_be32(frag, *loc, loc, &id);
	if (!frag && *loc == 0xffff) {
		goto drop;
	}

	reass = reassembly_get(id, &NET_IPV6_HDR(pkt)->src,
			       &NET_IPV6_HDR(pkt)->dst);
	if (!reass) {
		NET_DBG("Cannot get reassembly slot, dropping pkt %p", pkt);
		goto drop;
	}

	offset = flag & 0xfff8;
	more = flag & 0x01;

	net_pkt_set_ipv6_fragment_offset(pkt, offset);

	if (!reass->pkt[0]) {
		NET_DBG("Storing pkt %p to slot %d offset 0x%x", pkt, 0,
			offset);
		reass->pkt[0] = pkt;

		reassembly_info("Reassembly 1st pkt", reass);

		/* Wait for more fragments to receive. */
		goto accept;
	}

	/* The fragments might come in wrong order so place them
	 * in reassembly chain in correct order.
	 */
	for (i = 0, found = false; i < NET_IPV6_FRAGMENTS_MAX_PKT; i++) {
		if (!reass->pkt[i]) {
			NET_DBG("Storing pkt %p to slot %d offset 0x%x", pkt,
				i, offset);
			reass->pkt[i] = pkt;
			found = true;
			break;
		}

		if (net_pkt_ipv6_fragment_offset(reass->pkt[i]) < offset) {
			continue;
		}

		/* Make room for this fragment, if there is no room, then
		 * discard the whole reassembly.
		 */
		if (shift_packets(reass, i)) {
			break;
		}

		NET_DBG("Storing %p (offset 0x%x) to [%d]", pkt, offset, i);
		reass->pkt[i] = pkt;
		found = true;
		break;
	}

	if (!found) {
		/* We could not add this fragment into our saved fragment
		 * list. We must discard the whole packet at this point.
		 */
		NET_DBG("No slots available for 0x%x", reass->id);
		net_pkt_unref(pkt);
		goto drop;
	}

	if (more) {
		if (net_pkt_get_len(pkt) % 8) {
			/* Fragment length is not multiple of 8, discard
			 * the packet and send parameter problem error.
			 */
			net_icmpv6_send_error(pkt, NET_ICMPV6_PARAM_PROBLEM,
					      NET_ICMPV6_PARAM_PROB_OPTION, 0);
			goto drop;
		}

		reassembly_info("Reassembly nth pkt", reass);

		NET_DBG("More fragments to be received");
		goto accept;
	}

	reassembly_info("Reassembly last pkt", reass);

	if (!fragment_verify(reass)) {
		NET_DBG("Reassembled IPv6 verify failed, dropping id %u",
			reass->id);

		/* Let the caller release the already inserted pkt */
		if (i < NET_IPV6_FRAGMENTS_MAX_PKT) {
			reass->pkt[i] = NULL;
		}

		net_pkt_unref(pkt);
		goto drop;
	}

	/* The last fragment received, reassemble the packet */
	reassemble_packet(reass);

accept:
	return NET_OK;

drop:
	if (reass) {
		if (reassembly_cancel(reass->id, &reass->src, &reass->dst)) {
			return NET_OK;
		}
	}

	return NET_DROP;
}

#define BUF_ALLOC_TIMEOUT K_MSEC(100)

static int send_ipv6_fragment(struct net_if *iface,
			      struct net_pkt *pkt,
			      struct net_buf **rest,
			      u16_t ipv6_hdrs_len,
			      u16_t fit_len,
			      u16_t frag_offset,
			      u8_t next_hdr,
			      u16_t next_hdr_idx,
			      u8_t last_hdr,
			      u16_t last_hdr_idx,
			      u16_t frag_count)
{
	struct net_pkt *ipv6 = NULL;
	bool final;
	struct net_ipv6_frag_hdr hdr;
	struct net_buf *frag;
	struct net_buf *temp;
	u16_t pos;
	bool res;
	int ret;

	ipv6 = net_pkt_clone(pkt, BUF_ALLOC_TIMEOUT);
	if (!ipv6) {
		NET_DBG("Cannot clone %p", ipv6);
		return -ENOMEM;
	}

	/* And we need to update the last header in the IPv6 packet to point to
	 * fragment header.
	 */
	temp = net_pkt_write_u8_timeout(ipv6, ipv6->frags, next_hdr_idx, &pos,
					NET_IPV6_NEXTHDR_FRAG,
					BUF_ALLOC_TIMEOUT);
	if (!temp) {
		if (pos == 0xffff) {
			ret = -EINVAL;
		} else {
			ret = -ENOMEM;
		}

		goto fail;
	}

	/* Update the extension length metadata so that upper layer checksum
	 * will be calculated properly by net_ipv6_finalize().
	 */
	net_pkt_set_ipv6_ext_len(ipv6,
				 net_pkt_ipv6_ext_len(pkt) +
				 sizeof(struct net_ipv6_frag_hdr));

	frag = *rest;
	if (fit_len < net_buf_frags_len(*rest)) {
		ret = net_pkt_split(pkt, frag, fit_len, rest, FRAG_BUF_WAIT);
		if (ret < 0) {
			net_buf_unref(frag);
			goto fail;
		}
	} else {
		*rest = NULL;
	}

	final = false;
	/* *rest == NULL means no more data to send */
	if (!*rest) {
		final = true;
	}

	/* Append the Fragmentation Header */
	hdr.nexthdr = next_hdr;
	hdr.reserved = 0;
	hdr.id = net_pkt_ipv6_fragment_id(pkt);
	hdr.offset = htons(((frag_offset / 8) << 3) | !final);

	res = net_pkt_append_all(ipv6, sizeof(struct net_ipv6_frag_hdr),
				 (u8_t *)&hdr, FRAG_BUF_WAIT);
	if (!res) {
		net_buf_unref(frag);
		ret = EINVAL;
		goto fail;
	}

	/* Attach the first part of split payload to end of the packet. And
	 * "rest" of the packet will be sent in next iteration.
	 */
	temp = ipv6->frags;
	while (1) {
		if (!temp->frags) {
			temp->frags = frag;
			break;
		}

		temp = temp->frags;
	}

	res = net_pkt_compact(ipv6);
	if (!res) {
		ret = -EINVAL;
		goto fail;
	}

	/* Note that we must not calculate possible UDP/TCP/ICMPv6 checksum
	 * as that is already calculated in the non-fragmented packet.
	 */
	ret = net_ipv6_finalize(ipv6, NET_IPV6_NEXTHDR_FRAG);
	if (ret < 0) {
		NET_DBG("Cannot create IPv6 packet (%d)", ret);
		goto fail;
	}

	/* If everything has been ok so far, we can send the packet.
	 * Note that we cannot send this re-constructed packet directly
	 * as the link layer headers will not be properly set (because
	 * we recreated the packet). So pass this packet back to TX
	 * so that the pkt is going back to L2 for setup.
	 */
	ret = net_send_data(ipv6);
	if (ret < 0) {
		NET_DBG("Cannot send fragment (%d)", ret);
		goto fail;
	}

	/* Let this packet to be sent and hopefully it will release
	 * the memory that can be utilized for next sent IPv6 fragment.
	 */
	k_yield();

	return 0;

fail:
	if (ipv6) {
		net_pkt_unref(ipv6);
	}

	return ret;
}

int net_ipv6_send_fragmented_pkt(struct net_if *iface, struct net_pkt *pkt,
				 u16_t pkt_len)
{
	struct net_buf *rest = NULL;
	struct net_pkt *clone;
	struct net_buf *temp;
	u16_t next_hdr_idx;
	u16_t last_hdr_idx;
	u16_t ipv6_hdrs_len;
	u16_t frag_offset;
	u16_t frag_count;
	u16_t pos;
	u8_t next_hdr;
	u8_t last_hdr;
	int fit_len;
	int ret = -EINVAL;

	/* We cannot touch original pkt because it might be used for
	 * some other purposes, like TCP resend etc. So we need to copy
	 * the large pkt here and do the fragmenting with the clone.
	 */
	clone = net_pkt_clone(pkt, BUF_ALLOC_TIMEOUT);
	if (!clone) {
		NET_DBG("Cannot clone %p", pkt);
		return -ENOMEM;
	}

	pkt = clone;
	net_pkt_set_ipv6_fragment_id(pkt, sys_rand32_get());

	ret = net_ipv6_find_last_ext_hdr(pkt, &next_hdr_idx, &last_hdr_idx);
	if (ret < 0) {
		goto fail;
	}

	temp = net_frag_read_u8(pkt->frags, next_hdr_idx, &pos, &next_hdr);
	if (!temp && pos == 0xffff) {
		ret = -EINVAL;
		goto fail;
	}

	temp = net_frag_read_u8(pkt->frags, last_hdr_idx, &pos, &last_hdr);
	if (!temp && pos == 0xffff) {
		ret = -EINVAL;
		goto fail;
	}

	ipv6_hdrs_len = net_pkt_ip_hdr_len(pkt) + net_pkt_ipv6_ext_len(pkt);

	ret = net_pkt_split(pkt, pkt->frags, ipv6_hdrs_len, &rest,
			    FRAG_BUF_WAIT);
	if (ret < 0 || ipv6_hdrs_len != net_pkt_get_len(pkt)) {
		NET_DBG("Cannot split packet (%d)", ret);
		goto fail;
	}

	frag_count = 0;
	frag_offset = 0;

	/* The Maximum payload can fit into each packet after IPv6 header,
	 * Extenstion headers and Fragmentation header.
	 */
	fit_len = NET_IPV6_MTU - NET_IPV6_FRAGH_LEN - ipv6_hdrs_len;
	if (fit_len <= 0) {
		/* Must be invalid extension headers length */
		NET_DBG("No room for IPv6 payload MTU %d hdrs_len %d",
			NET_IPV6_MTU, NET_IPV6_FRAGH_LEN + ipv6_hdrs_len);
		ret = -EINVAL;
		goto fail;
	}

	while (rest) {
		ret = send_ipv6_fragment(iface, pkt, &rest, ipv6_hdrs_len,
					 fit_len, frag_offset, next_hdr,
					 next_hdr_idx, last_hdr, last_hdr_idx,
					 frag_count);
		if (ret < 0) {
			goto fail;
		}

		frag_count++;
		frag_offset += fit_len;
	}

	net_pkt_unref(pkt);

	return 0;

fail:
	net_pkt_unref(pkt);

	if (rest) {
		net_buf_unref(rest);
	}

	return ret;
}
