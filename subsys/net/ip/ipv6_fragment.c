/** @file
 * @brief IPv6 Fragment related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_ipv6, CONFIG_NET_IPV6_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/random/random.h>
#include "net_private.h"
#include "connection.h"
#include "icmpv6.h"
#include "udp_internal.h"
#include "tcp_internal.h"
#include "ipv6.h"
#include "nbr.h"
#include "6lo.h"
#include "route.h"
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

int net_ipv6_find_last_ext_hdr(struct net_pkt *pkt, uint16_t *next_hdr_off,
			       uint16_t *last_hdr_off)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	struct net_ipv6_hdr *hdr;
	uint8_t next_nexthdr;
	uint8_t nexthdr;

	if (!pkt || !pkt->frags || !next_hdr_off || !last_hdr_off) {
		return -EINVAL;
	}

	net_pkt_cursor_init(pkt);

	hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (!hdr) {
		return -ENOBUFS;
	}

	net_pkt_acknowledge_data(pkt, &ipv6_access);

	nexthdr = hdr->nexthdr;

	/* Initial values */
	*next_hdr_off = offsetof(struct net_ipv6_hdr, nexthdr);
	*last_hdr_off = sizeof(struct net_ipv6_hdr);

	while (!net_ipv6_is_nexthdr_upper_layer(nexthdr)) {
		if (net_pkt_read_u8(pkt, &next_nexthdr)) {
			goto fail;
		}

		switch (nexthdr) {
		case NET_IPV6_NEXTHDR_HBHO:
		case NET_IPV6_NEXTHDR_DESTO:
			{
				uint8_t val = 0U;
				uint16_t length;

				if (net_pkt_read_u8(pkt, &val)) {
					goto fail;
				}

				length = val * 8U + 8 - 2;

				if (net_pkt_skip(pkt, length)) {
					goto fail;
				}
			}
			break;
		case NET_IPV6_NEXTHDR_FRAG:
			if (net_pkt_skip(pkt, 7)) {
				goto fail;
			}

			break;
		case NET_IPV6_NEXTHDR_NONE:
			goto out;
		default:
			/* TODO: Add more IPv6 extension headers to check */
			goto fail;
		}

		*next_hdr_off = *last_hdr_off;
		*last_hdr_off = net_pkt_get_current_offset(pkt);

		nexthdr = next_nexthdr;
	}
out:
	return 0;
fail:
	return -EINVAL;
}

static struct net_ipv6_reassembly *reassembly_get(uint32_t id,
						  struct in6_addr *src,
						  struct in6_addr *dst)
{
	int i, avail = -1;

	for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {
		if (k_work_delayable_remaining_get(&reassembly[i].timer) &&
		    reassembly[i].id == id &&
		    net_ipv6_addr_cmp(src, &reassembly[i].src) &&
		    net_ipv6_addr_cmp(dst, &reassembly[i].dst)) {
			return &reassembly[i];
		}

		if (k_work_delayable_remaining_get(&reassembly[i].timer)) {
			continue;
		}

		if (avail < 0) {
			avail = i;
		}
	}

	if (avail < 0) {
		return NULL;
	}

	k_work_reschedule(&reassembly[avail].timer, IPV6_REASSEMBLY_TIMEOUT);

	net_ipaddr_copy(&reassembly[avail].src, src);
	net_ipaddr_copy(&reassembly[avail].dst, dst);

	reassembly[avail].id = id;

	return &reassembly[avail];
}

static bool reassembly_cancel(uint32_t id,
			      struct in6_addr *src,
			      struct in6_addr *dst)
{
	int i, j;

	NET_DBG("Cancel 0x%x", id);

	for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {
		int32_t remaining;

		if (reassembly[i].id != id ||
		    !net_ipv6_addr_cmp(src, &reassembly[i].src) ||
		    !net_ipv6_addr_cmp(dst, &reassembly[i].dst)) {
			continue;
		}

		remaining = k_ticks_to_ms_ceil32(
			k_work_delayable_remaining_get(&reassembly[i].timer));
		k_work_cancel_delayable(&reassembly[i].timer);

		NET_DBG("IPv6 reassembly id 0x%x remaining %d ms",
			reassembly[i].id, remaining);

		reassembly[i].id = 0U;

		for (j = 0; j < CONFIG_NET_IPV6_FRAGMENT_MAX_PKT; j++) {
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
	NET_DBG("%s id 0x%x src %s dst %s remain %d ms", str, reass->id,
		net_sprint_ipv6_addr(&reass->src),
		net_sprint_ipv6_addr(&reass->dst),
		k_ticks_to_ms_ceil32(
			k_work_delayable_remaining_get(&reass->timer)));
}

static void reassembly_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct net_ipv6_reassembly *reass =
		CONTAINER_OF(dwork, struct net_ipv6_reassembly, timer);

	reassembly_info("Reassembly cancelled", reass);

	/* Send a ICMPv6 Time Exceeded only if we received the first fragment (RFC 2460 Sec. 5) */
	if (reass->pkt[0] && net_pkt_ipv6_fragment_offset(reass->pkt[0]) == 0) {
		net_icmpv6_send_error(reass->pkt[0], NET_ICMPV6_TIME_EXCEEDED, 1, 0);
	}

	reassembly_cancel(reass->id, &reass->src, &reass->dst);
}

static void reassemble_packet(struct net_ipv6_reassembly *reass)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(frag_access, struct net_ipv6_frag_hdr);
	union {
		struct net_ipv6_hdr *hdr;
		struct net_ipv6_frag_hdr *frag_hdr;
	} ipv6;

	struct net_pkt *pkt;
	struct net_buf *last;
	uint8_t next_hdr;
	int i, len;

	k_work_cancel_delayable(&reass->timer);

	NET_ASSERT(reass->pkt[0]);

	last = net_buf_frag_last(reass->pkt[0]->buffer);

	/* We start from 2nd packet which is then appended to
	 * the first one.
	 */
	for (i = 1; i < CONFIG_NET_IPV6_FRAGMENT_MAX_PKT; i++) {
		int removed_len;

		pkt = reass->pkt[i];
		if (!pkt) {
			break;
		}

		net_pkt_cursor_init(pkt);

		/* Get rid of IPv6 and fragment header which are at
		 * the beginning of the fragment.
		 */
		removed_len = net_pkt_ipv6_fragment_start(pkt) +
			      sizeof(struct net_ipv6_frag_hdr);

		NET_DBG("Removing %d bytes from start of pkt %p",
			removed_len, pkt->buffer);

		if (net_pkt_pull(pkt, removed_len)) {
			NET_ERR("Failed to pull headers");
			reassembly_cancel(reass->id, &reass->src, &reass->dst);
			return;
		}

		/* Attach the data to previous pkt */
		last->frags = pkt->buffer;
		last = net_buf_frag_last(pkt->buffer);

		pkt->buffer = NULL;
		reass->pkt[i] = NULL;

		net_pkt_unref(pkt);
	}

	pkt = reass->pkt[0];
	reass->pkt[0] = NULL;

	/* Next we need to strip away the fragment header from the first packet
	 * and set the various pointers and values in packet.
	 */
	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, net_pkt_ipv6_fragment_start(pkt))) {
		NET_ERR("Failed to move to fragment header");
		goto error;
	}

	ipv6.frag_hdr = (struct net_ipv6_frag_hdr *)net_pkt_get_data(
							pkt, &frag_access);
	if (!ipv6.frag_hdr) {
		NET_ERR("Failed to get fragment header");
		goto error;
	}

	next_hdr = ipv6.frag_hdr->nexthdr;

	if (net_pkt_pull(pkt, sizeof(struct net_ipv6_frag_hdr))) {
		NET_ERR("Failed to remove fragment header");
		goto error;
	}

	/* This one updates the previous header's nexthdr value */
	if (net_pkt_skip(pkt, net_pkt_ipv6_hdr_prev(pkt)) ||
	    net_pkt_write_u8(pkt, next_hdr)) {
		goto error;
	}

	net_pkt_cursor_init(pkt);

	ipv6.hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (!ipv6.hdr) {
		goto error;
	}

	/* Fix the total length of the IPv6 packet. */
	len = net_pkt_ipv6_ext_len(pkt);
	if (len > 0) {
		NET_DBG("Old pkt %p IPv6 ext len is %d bytes", pkt, len);
		net_pkt_set_ipv6_ext_len(pkt,
				len - sizeof(struct net_ipv6_frag_hdr));
	}

	len = net_pkt_get_len(pkt) - sizeof(struct net_ipv6_hdr);

	ipv6.hdr->len = htons(len);

	net_pkt_set_data(pkt, &ipv6_access);
	net_pkt_set_ip_reassembled(pkt, true);

	NET_DBG("New pkt %p IPv6 len is %d bytes", pkt,
		len + NET_IPV6H_LEN);

	/* We need to use the queue when feeding the packet back into the
	 * IP stack as we might run out of stack if we call processing_data()
	 * directly. As the packet does not contain link layer header, we
	 * MUST NOT pass it to L2 so there will be a special check for that
	 * in process_data() when handling the packet.
	 */
	if (net_recv_data(net_pkt_iface(pkt), pkt) >= 0) {
		return;
	}
error:
	net_pkt_unref(pkt);
}

void net_ipv6_frag_foreach(net_ipv6_frag_cb_t cb, void *user_data)
{
	int i;

	for (i = 0; reassembly_init_done &&
		     i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {
		if (!k_work_delayable_remaining_get(&reassembly[i].timer)) {
			continue;
		}

		cb(&reassembly[i], user_data);
	}
}

/* Verify that we have all the fragments received and in correct order.
 * Return:
 * - a negative value if the fragments are erroneous and must be dropped
 * - zero if we are expecting more fragments
 * - a positive value if we can proceed with the reassembly
 */
static int fragments_are_ready(struct net_ipv6_reassembly *reass)
{
	unsigned int expected_offset = 0;
	bool more = true;
	int i;

	/* Fragments can arrive in any order, for example in reverse order:
	 *   1 -> Fragment3(M=0, offset=x2)
	 *   2 -> Fragment2(M=1, offset=x1)
	 *   3 -> Fragment1(M=1, offset=0)
	 * We have to test several requirements before proceeding with the reassembly:
	 * - We received the first fragment (Fragment Offset is 0)
	 * - All intermediate fragments are contiguous
	 * - The More bit of the last fragment is 0
	 */
	for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_PKT; i++) {
		struct net_pkt *pkt = reass->pkt[i];
		unsigned int offset;
		int payload_len;

		if (!pkt) {
			break;
		}

		offset = net_pkt_ipv6_fragment_offset(pkt);

		if (offset < expected_offset) {
			/* Overlapping or duplicated
			 * According to RFC8200 we can drop it
			 */
			return -EBADMSG;
		} else if (offset != expected_offset) {
			/* Not contiguous, let's wait for fragments */
			return 0;
		}

		payload_len = net_pkt_get_len(pkt) - net_pkt_ipv6_fragment_start(pkt);
		payload_len -= sizeof(struct net_ipv6_frag_hdr);
		if (payload_len < 0) {
			return -EBADMSG;
		}

		expected_offset += payload_len;
		more = net_pkt_ipv6_fragment_more(pkt);
	}

	if (more) {
		return 0;
	}

	return 1;
}

static int shift_packets(struct net_ipv6_reassembly *reass, int pos)
{
	int i;

	for (i = pos + 1; i < CONFIG_NET_IPV6_FRAGMENT_MAX_PKT; i++) {
		if (!reass->pkt[i]) {
			NET_DBG("Moving [%d] %p (offset 0x%x) to [%d]",
				pos, reass->pkt[pos],
				net_pkt_ipv6_fragment_offset(reass->pkt[pos]),
				pos + 1);

			/* pkt[i] is free, so shift everything between
			 * [pos] and [i - 1] by one element
			 */
			memmove(&reass->pkt[pos + 1], &reass->pkt[pos],
				sizeof(void *) * (i - pos));

			/* pkt[pos] is now free */
			reass->pkt[pos] = NULL;

			return 0;
		}
	}

	/* We do not have free space left in the array */
	return -ENOMEM;
}

enum net_verdict net_ipv6_handle_fragment_hdr(struct net_pkt *pkt,
					      struct net_ipv6_hdr *hdr,
					      uint8_t nexthdr)
{
	struct net_ipv6_reassembly *reass = NULL;
	uint16_t flag;
	bool found;
	uint8_t more;
	uint32_t id;
	int ret;
	int i;

	if (!reassembly_init_done) {
		/* Static initializing does not work here because of the array
		 * so we must do it at runtime.
		 */
		for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {
			k_work_init_delayable(&reassembly[i].timer,
					      reassembly_timeout);
		}

		reassembly_init_done = true;
	}

	/* Each fragment has a fragment header, however since we already
	 * read the nexthdr part of it, we are not going to use
	 * net_pkt_get_data() and access the header directly: the cursor
	 * being 1 byte too far, let's just read the next relevant pieces.
	 */
	if (net_pkt_skip(pkt, 1) || /* reserved */
	    net_pkt_read_be16(pkt, &flag) ||
	    net_pkt_read_be32(pkt, &id)) {
		goto drop;
	}

	reass = reassembly_get(id, (struct in6_addr *)hdr->src,
			       (struct in6_addr *)hdr->dst);
	if (!reass) {
		NET_DBG("Cannot get reassembly slot, dropping pkt %p", pkt);
		goto drop;
	}

	more = flag & 0x01;
	net_pkt_set_ipv6_fragment_flags(pkt, flag);

	if (more && net_pkt_get_len(pkt) % 8) {
		/* Fragment length is not multiple of 8, discard
		 * the packet and send parameter problem error with the
		 * offset of the "Payload Length" field in the IPv6 header.
		 */
		net_icmpv6_send_error(pkt, NET_ICMPV6_PARAM_PROBLEM,
				      NET_ICMPV6_PARAM_PROB_HEADER, NET_IPV6H_LENGTH_OFFSET);
		goto drop;
	}

	/* The fragments might come in wrong order so place them
	 * in reassembly chain in correct order.
	 */
	for (i = 0, found = false; i < CONFIG_NET_IPV6_FRAGMENT_MAX_PKT; i++) {
		if (reass->pkt[i]) {
			if (net_pkt_ipv6_fragment_offset(reass->pkt[i]) <
			    net_pkt_ipv6_fragment_offset(pkt)) {
				continue;
			}

			/* Make room for this fragment. If there is no room,
			 * then it will discard the whole reassembly.
			 */
			if (shift_packets(reass, i)) {
				break;
			}
		}

		NET_DBG("Storing pkt %p to slot %d offset %d",
			pkt, i, net_pkt_ipv6_fragment_offset(pkt));
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

	ret = fragments_are_ready(reass);
	if (ret < 0) {
		NET_DBG("Reassembled IPv6 verify failed, dropping id %u",
			reass->id);

		/* Let the caller release the already inserted pkt */
		if (i < CONFIG_NET_IPV6_FRAGMENT_MAX_PKT) {
			reass->pkt[i] = NULL;
		}

		net_pkt_unref(pkt);
		goto drop;
	} else if (ret == 0) {
		reassembly_info("Reassembly nth pkt", reass);

		NET_DBG("More fragments to be received");
		goto accept;
	}

	reassembly_info("Reassembly last pkt", reass);

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

static int send_ipv6_fragment(struct net_pkt *pkt,
			      uint16_t fit_len,
			      uint16_t frag_offset,
			      uint16_t next_hdr_off,
			      uint8_t next_hdr,
			      bool final)
{
	NET_PKT_DATA_ACCESS_DEFINE(frag_access, struct net_ipv6_frag_hdr);
	uint8_t frag_pkt_next_hdr = NET_IPV6_NEXTHDR_HBHO;
	int ret = -ENOBUFS;
	struct net_ipv6_frag_hdr *frag_hdr;
	struct net_pkt *frag_pkt;

	frag_pkt = net_pkt_alloc_with_buffer(net_pkt_iface(pkt), fit_len +
					     net_pkt_ipv6_ext_len(pkt) +
					     NET_IPV6_FRAGH_LEN,
					     AF_INET6, 0, BUF_ALLOC_TIMEOUT);
	if (!frag_pkt) {
		return -ENOMEM;
	}

	net_pkt_cursor_init(pkt);

	/* We copy original headers back to the fragment packet
	 * Note that we insert the right next header to point to fragment header
	 */
	if (net_pkt_copy(frag_pkt, pkt, next_hdr_off) ||
	    net_pkt_write_u8(frag_pkt, NET_IPV6_NEXTHDR_FRAG) ||
	    net_pkt_skip(pkt, 1) ||
	    net_pkt_copy(frag_pkt, pkt, net_pkt_ip_hdr_len(pkt) +
			 net_pkt_ipv6_ext_len(pkt) - next_hdr_off - 1)) {
		goto fail;
	}

	if (!net_pkt_ipv6_ext_len(pkt)) {
		frag_pkt_next_hdr = NET_IPV6_NEXTHDR_FRAG;
	}

	/* And we append the fragmentation header */
	frag_hdr = (struct net_ipv6_frag_hdr *)net_pkt_get_data(frag_pkt,
								&frag_access);
	if (!frag_hdr) {
		goto fail;
	}

	frag_hdr->nexthdr = next_hdr;
	frag_hdr->reserved = 0U;
	frag_hdr->id = net_pkt_ipv6_fragment_id(pkt);
	frag_hdr->offset = htons(((frag_offset / 8U) << 3) | !final);

	net_pkt_set_chksum_done(frag_pkt, true);

	if (net_pkt_set_data(frag_pkt, &frag_access)) {
		goto fail;
	}

	net_pkt_set_ip_hdr_len(frag_pkt, net_pkt_ip_hdr_len(pkt));
	net_pkt_set_ipv6_ext_len(frag_pkt,
				 net_pkt_ipv6_ext_len(pkt) +
				 sizeof(struct net_ipv6_frag_hdr));

	/* Finally we copy the payload part of this fragment from
	 * the original packet
	 */
	if (net_pkt_skip(pkt, frag_offset) ||
	    net_pkt_copy(frag_pkt, pkt, fit_len)) {
		goto fail;
	}

	net_pkt_cursor_init(frag_pkt);

	if (net_ipv6_finalize(frag_pkt, frag_pkt_next_hdr) < 0) {
		goto fail;
	}

	if (final) {
		net_pkt_set_context(frag_pkt, net_pkt_context(pkt));
	}

	/* If everything has been ok so far, we can send the packet. */
	ret = net_send_data(frag_pkt);
	if (ret < 0) {
		goto fail;
	}

	/* Let this packet to be sent and hopefully it will release
	 * the memory that can be utilized for next sent IPv6 fragment.
	 */
	k_yield();

	return 0;

fail:
	NET_DBG("Cannot send fragment (%d)", ret);
	net_pkt_unref(frag_pkt);

	return ret;
}

int net_ipv6_send_fragmented_pkt(struct net_if *iface, struct net_pkt *pkt,
				 uint16_t pkt_len, uint16_t mtu)
{
	uint16_t next_hdr_off;
	uint16_t last_hdr_off;
	uint16_t frag_offset;
	size_t length;
	uint8_t next_hdr;
	int fit_len;
	int ret;

	net_pkt_set_ipv6_fragment_id(pkt, sys_rand32_get());

	ret = net_ipv6_find_last_ext_hdr(pkt, &next_hdr_off, &last_hdr_off);
	if (ret < 0) {
		return ret;
	}

	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, next_hdr_off) ||
	    net_pkt_read_u8(pkt, &next_hdr)) {
		return -ENOBUFS;
	}

	/* The Maximum payload can fit into each packet after IPv6 header,
	 * Extension headers and Fragmentation header.
	 */
	fit_len = (int)mtu - NET_IPV6_FRAGH_LEN -
		(net_pkt_ip_hdr_len(pkt) + net_pkt_ipv6_ext_len(pkt));
	if (fit_len <= 0) {
		/* Must be invalid extension headers length */
		NET_DBG("No room for IPv6 payload MTU %d hdrs_len %d",
			mtu, NET_IPV6_FRAGH_LEN +
			net_pkt_ip_hdr_len(pkt) + net_pkt_ipv6_ext_len(pkt));
		return -EINVAL;
	}

	frag_offset = 0U;

	/* Calculate the L4 checksum (if not done already) before the fragmentation. */
	if (!net_pkt_is_chksum_done(pkt)) {
		net_pkt_cursor_init(pkt);
		net_pkt_skip(pkt, last_hdr_off);

		switch (next_hdr) {
		case IPPROTO_ICMPV6:
			ret = net_icmpv6_finalize(pkt, true);
			break;
		case IPPROTO_TCP:
			ret = net_tcp_finalize(pkt, true);
			break;
		case IPPROTO_UDP:
			ret = net_udp_finalize(pkt, true);
			break;
		default:
			ret = 0;
			break;
		}

		if (ret < 0) {
			return ret;
		}
	}

	length = net_pkt_get_len(pkt) -
		(net_pkt_ip_hdr_len(pkt) + net_pkt_ipv6_ext_len(pkt));
	while (length) {
		bool final = false;

		if (fit_len >= length) {
			final = true;
			fit_len = length;
		}

		ret = send_ipv6_fragment(pkt, fit_len, frag_offset,
					 next_hdr_off, next_hdr, final);
		if (ret < 0) {
			return ret;
		}

		length -= fit_len;
		frag_offset += fit_len;
	}

	return 0;
}
