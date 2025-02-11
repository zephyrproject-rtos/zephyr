/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2022 Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_ipv4, CONFIG_NET_IPV4_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/random/random.h>
#include "net_private.h"
#include "connection.h"
#include "icmpv4.h"
#include "udp_internal.h"
#include "tcp_internal.h"
#include "ipv4.h"
#include "route.h"
#include "net_stats.h"

/* Timeout for various buffer allocations in this file. */
#define NET_BUF_TIMEOUT K_MSEC(100)

static void reassembly_timeout(struct k_work *work);

static struct net_ipv4_reassembly reassembly[CONFIG_NET_IPV4_FRAGMENT_MAX_COUNT];

static struct net_ipv4_reassembly *reassembly_get(uint16_t id, struct in_addr *src,
						  struct in_addr *dst, uint8_t protocol)
{
	int i, avail = -1;

	for (i = 0; i < CONFIG_NET_IPV4_FRAGMENT_MAX_COUNT; i++) {
		if (k_work_delayable_remaining_get(&reassembly[i].timer) &&
		    reassembly[i].id == id &&
		    net_ipv4_addr_cmp(src, &reassembly[i].src) &&
		    net_ipv4_addr_cmp(dst, &reassembly[i].dst) &&
		    reassembly[i].protocol == protocol) {
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

	k_work_reschedule(&reassembly[avail].timer, K_SECONDS(CONFIG_NET_IPV4_FRAGMENT_TIMEOUT));

	net_ipaddr_copy(&reassembly[avail].src, src);
	net_ipaddr_copy(&reassembly[avail].dst, dst);

	reassembly[avail].protocol = protocol;
	reassembly[avail].id = id;

	return &reassembly[avail];
}

static bool reassembly_cancel(uint32_t id, struct in_addr *src, struct in_addr *dst)
{
	int i, j;

	LOG_DBG("Cancel 0x%x", id);

	for (i = 0; i < CONFIG_NET_IPV4_FRAGMENT_MAX_COUNT; i++) {
		int32_t remaining;

		if (reassembly[i].id != id ||
		    !net_ipv4_addr_cmp(src, &reassembly[i].src) ||
		    !net_ipv4_addr_cmp(dst, &reassembly[i].dst)) {
			continue;
		}

		remaining = k_ticks_to_ms_ceil32(
			k_work_delayable_remaining_get(&reassembly[i].timer));
		k_work_cancel_delayable(&reassembly[i].timer);

		LOG_DBG("IPv4 reassembly id 0x%x remaining %d ms", reassembly[i].id, remaining);

		reassembly[i].id = 0U;

		for (j = 0; j < CONFIG_NET_IPV4_FRAGMENT_MAX_PKT; j++) {
			if (!reassembly[i].pkt[j]) {
				continue;
			}

			LOG_DBG("[%d] IPv4 reassembly pkt %p %zd bytes data", j,
				reassembly[i].pkt[j], net_pkt_get_len(reassembly[i].pkt[j]));

			net_pkt_unref(reassembly[i].pkt[j]);
			reassembly[i].pkt[j] = NULL;
		}

		return true;
	}

	return false;
}

static void reassembly_info(char *str, struct net_ipv4_reassembly *reass)
{
	LOG_DBG("%s id 0x%x src %s dst %s remain %d ms", str, reass->id,
		net_sprint_ipv4_addr(&reass->src),
		net_sprint_ipv4_addr(&reass->dst),
		k_ticks_to_ms_ceil32(
			k_work_delayable_remaining_get(&reass->timer)));
}

static void reassembly_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct net_ipv4_reassembly *reass =
		CONTAINER_OF(dwork, struct net_ipv4_reassembly, timer);

	reassembly_info("Reassembly cancelled", reass);

	/* Send a ICMPv4 Time Exceeded only if we received the first fragment */
	if (reass->pkt[0] && net_pkt_ipv4_fragment_offset(reass->pkt[0]) == 0) {
		net_icmpv4_send_error(reass->pkt[0], NET_ICMPV4_TIME_EXCEEDED,
				      NET_ICMPV4_TIME_EXCEEDED_FRAGMENT_REASSEMBLY_TIME);
	}

	reassembly_cancel(reass->id, &reass->src, &reass->dst);
}

static void reassemble_packet(struct net_ipv4_reassembly *reass)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	struct net_ipv4_hdr *ipv4_hdr;
	struct net_pkt *pkt;
	struct net_buf *last;
	int i;

	k_work_cancel_delayable(&reass->timer);

	NET_ASSERT(reass->pkt[0]);

	last = net_buf_frag_last(reass->pkt[0]->buffer);

	/* We start from 2nd packet which is then appended to the first one */
	for (i = 1; i < CONFIG_NET_IPV4_FRAGMENT_MAX_PKT; i++) {
		pkt = reass->pkt[i];
		if (!pkt) {
			break;
		}

		net_pkt_cursor_init(pkt);

		/* Get rid of IPv4 header which is at the beginning of the fragment. */
		ipv4_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);
		if (!ipv4_hdr) {
			goto error;
		}

		LOG_DBG("Removing %d bytes from start of pkt %p", net_pkt_ip_hdr_len(pkt),
			pkt->buffer);

		if (net_pkt_pull(pkt, net_pkt_ip_hdr_len(pkt))) {
			LOG_ERR("Failed to pull headers");
			reassembly_cancel(reass->id, &reass->src, &reass->dst);
			return;
		}

		/* Attach the data to the previous packet */
		last->frags = pkt->buffer;
		last = net_buf_frag_last(pkt->buffer);

		pkt->buffer = NULL;
		reass->pkt[i] = NULL;

		net_pkt_unref(pkt);
	}

	pkt = reass->pkt[0];
	reass->pkt[0] = NULL;

	/* Update the header details for the packet */
	net_pkt_cursor_init(pkt);

	ipv4_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);
	if (!ipv4_hdr) {
		goto error;
	}

	/* Fix the total length, offset and checksum of the IPv4 packet */
	ipv4_hdr->len = htons(net_pkt_get_len(pkt));
	ipv4_hdr->offset[0] = 0;
	ipv4_hdr->offset[1] = 0;
	ipv4_hdr->chksum = 0;
	ipv4_hdr->chksum = net_calc_chksum_ipv4(pkt);

	net_pkt_set_data(pkt, &ipv4_access);
	net_pkt_set_ip_reassembled(pkt, true);

	LOG_DBG("New pkt %p IPv4 len is %d bytes", pkt, net_pkt_get_len(pkt));

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

void net_ipv4_frag_foreach(net_ipv4_frag_cb_t cb, void *user_data)
{
	int i;

	for (i = 0; i < CONFIG_NET_IPV4_FRAGMENT_MAX_COUNT; i++) {
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
static int fragments_are_ready(struct net_ipv4_reassembly *reass)
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
	for (i = 0; i < CONFIG_NET_IPV4_FRAGMENT_MAX_PKT; i++) {
		struct net_pkt *pkt = reass->pkt[i];
		unsigned int offset;
		int payload_len;

		if (!pkt) {
			break;
		}

		offset = net_pkt_ipv4_fragment_offset(pkt);

		if (offset < expected_offset) {
			/* Overlapping or duplicated, drop it */
			return -EBADMSG;
		} else if (offset != expected_offset) {
			/* Not contiguous, let's wait for fragments */
			return 0;
		}

		payload_len = net_pkt_get_len(pkt) - net_pkt_ip_hdr_len(pkt);

		if (payload_len < 0) {
			return -EBADMSG;
		}

		expected_offset += payload_len;
		more = net_pkt_ipv4_fragment_more(pkt);
	}

	if (more) {
		return 0;
	}

	return 1;
}

static int shift_packets(struct net_ipv4_reassembly *reass, int pos)
{
	int i;

	for (i = pos + 1; i < CONFIG_NET_IPV4_FRAGMENT_MAX_PKT; i++) {
		if (!reass->pkt[i]) {
			LOG_DBG("Moving [%d] %p (offset 0x%x) to [%d]", pos, reass->pkt[pos],
				net_pkt_ipv4_fragment_offset(reass->pkt[pos]), pos + 1);

			/* pkt[i] is free, so shift everything between [pos] and [i - 1] by one
			 * element
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

enum net_verdict net_ipv4_handle_fragment_hdr(struct net_pkt *pkt, struct net_ipv4_hdr *hdr)
{
	struct net_ipv4_reassembly *reass = NULL;
	uint16_t flag;
	bool found;
	uint8_t more;
	uint16_t id;
	int ret;
	int i;

	flag = ntohs(*((uint16_t *)&hdr->offset));
	id = ntohs(*((uint16_t *)&hdr->id));

	reass = reassembly_get(id, (struct in_addr *)hdr->src,
			       (struct in_addr *)hdr->dst, hdr->proto);
	if (!reass) {
		LOG_ERR("Cannot get reassembly slot, dropping pkt %p", pkt);
		goto drop;
	}

	more = (flag & NET_IPV4_MORE_FRAG_MASK) ? true : false;
	net_pkt_set_ipv4_fragment_flags(pkt, flag);

	if (more && (net_pkt_get_len(pkt) - net_pkt_ip_hdr_len(pkt)) % 8) {
		/* Fragment length is not multiple of 8, discard the packet and send bad IP
		 * header error.
		 */
		net_icmpv4_send_error(pkt, NET_ICMPV4_BAD_IP_HEADER,
				      NET_ICMPV4_BAD_IP_HEADER_LENGTH);
		goto drop;
	}

	/* The fragments might come in wrong order so place them in the reassembly chain in the
	 * correct order.
	 */
	for (i = 0, found = false; i < CONFIG_NET_IPV4_FRAGMENT_MAX_PKT; i++) {
		if (reass->pkt[i]) {
			if (net_pkt_ipv4_fragment_offset(reass->pkt[i]) <
			    net_pkt_ipv4_fragment_offset(pkt)) {
				continue;
			}

			/* Make room for this fragment. If there is no room then it will discard
			 * the whole reassembly.
			 */
			if (shift_packets(reass, i)) {
				break;
			}
		}

		LOG_DBG("Storing pkt %p to slot %d offset %d", pkt, i,
			net_pkt_ipv4_fragment_offset(pkt));
		reass->pkt[i] = pkt;
		found = true;

		break;
	}

	if (!found) {
		/* We could not add this fragment into our saved fragment list. The whole packet
		 * must be discarded at this point.
		 */
		LOG_ERR("No slots available for 0x%x", reass->id);
		net_pkt_unref(pkt);
		goto drop;
	}

	ret = fragments_are_ready(reass);
	if (ret < 0) {
		LOG_ERR("Reassembled IPv4 verify failed, dropping id %u", reass->id);

		/* Let the caller release the already inserted pkt */
		if (i < CONFIG_NET_IPV4_FRAGMENT_MAX_PKT) {
			reass->pkt[i] = NULL;
		}

		net_pkt_unref(pkt);
		goto drop;
	} else if (ret == 0) {
		reassembly_info("Reassembly nth pkt", reass);

		LOG_DBG("More fragments to be received");
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

static int send_ipv4_fragment(struct net_pkt *pkt, uint16_t rand_id, uint16_t fit_len,
			      uint16_t frag_offset, bool final)
{
	int ret = -ENOBUFS;
	struct net_pkt *frag_pkt;
	struct net_pkt_cursor cur;
	struct net_pkt_cursor cur_pkt;
	uint16_t offset_pkt;

	frag_pkt = net_pkt_alloc_with_buffer(net_pkt_iface(pkt), fit_len +
					     net_pkt_ip_hdr_len(pkt),
					     AF_INET, 0, NET_BUF_TIMEOUT);
	if (!frag_pkt) {
		return -ENOMEM;
	}

	net_pkt_cursor_init(frag_pkt);
	net_pkt_cursor_backup(pkt, &cur_pkt);
	net_pkt_cursor_backup(frag_pkt, &cur);

	/* Copy the original IPv4 headers back to the fragment packet */
	if (net_pkt_copy(frag_pkt, pkt, net_pkt_ip_hdr_len(pkt))) {
		goto fail;
	}

	net_pkt_cursor_restore(pkt, &cur_pkt);

	/* Copy the payload part of this fragment from the original packet */
	if (net_pkt_skip(pkt, (frag_offset + net_pkt_ip_hdr_len(pkt))) ||
	    net_pkt_copy(frag_pkt, pkt, fit_len)) {
		goto fail;
	}

	net_pkt_cursor_restore(frag_pkt, &cur);
	net_pkt_cursor_restore(pkt, &cur_pkt);

	net_pkt_set_ip_hdr_len(frag_pkt, net_pkt_ip_hdr_len(pkt));

	net_pkt_set_overwrite(frag_pkt, true);
	net_pkt_cursor_init(frag_pkt);

	/* Update the header of the packet */
	NET_PKT_DATA_ACCESS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	struct net_ipv4_hdr *ipv4_hdr;

	ipv4_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(frag_pkt, &ipv4_access);
	if (!ipv4_hdr) {
		return -ENOBUFS;
	}

	memcpy(ipv4_hdr->id, &rand_id, sizeof(rand_id));
	offset_pkt = frag_offset / 8;

	if (!final) {
		offset_pkt |= NET_IPV4_MORE_FRAG_MASK;
	}

	sys_put_be16(offset_pkt, ipv4_hdr->offset);
	ipv4_hdr->len = htons((fit_len + net_pkt_ip_hdr_len(pkt)));

	ipv4_hdr->chksum = 0;
	ipv4_hdr->chksum = net_calc_chksum_ipv4(frag_pkt);

	net_pkt_set_chksum_done(frag_pkt, true);

	net_pkt_set_data(frag_pkt, &ipv4_access);

	net_pkt_set_overwrite(frag_pkt, false);
	net_pkt_cursor_restore(frag_pkt, &cur);

	if (final) {
		net_pkt_set_context(frag_pkt, net_pkt_context(pkt));
	}

	/* If everything has been ok so far, we can send the packet. */
	ret = net_send_data(frag_pkt);
	if (ret < 0) {
		goto fail;
	}

	/* Let this packet to be sent and hopefully it will release the memory that can be
	 * utilized for next IPv4 fragment.
	 */
	k_yield();

	return 0;

fail:
	LOG_ERR("Cannot send fragment (%d)", ret);
	net_pkt_unref(frag_pkt);

	return ret;
}

int net_ipv4_send_fragmented_pkt(struct net_if *iface, struct net_pkt *pkt,
				 uint16_t pkt_len, uint16_t mtu)
{
	uint16_t frag_offset = 0;
	uint16_t flag;
	int fit_len;
	int ret;
	struct net_ipv4_hdr *frag_hdr;

	NET_PKT_DATA_ACCESS_DEFINE(frag_access, struct net_ipv4_hdr);
	frag_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &frag_access);
	if (!frag_hdr) {
		return -EINVAL;
	}

	/* Check if the DF (Don't Fragment) flag is set, if so, we cannot fragment the packet */
	flag = ntohs(*((uint16_t *)&frag_hdr->offset));

	if (flag & NET_IPV4_DO_NOT_FRAG_MASK) {
		/* This packet cannot be fragmented */
		return -EPERM;
	}

	/* Generate a random ID to be used for packet identification, ensuring that it is not 0 */
	uint16_t rand_id = sys_rand16_get();

	if (rand_id == 0) {
		rand_id = 1;
	}

	/* Calculate maximum payload that can fit into each packet after IPv4 header. Offsets are
	 * multiples of 8, therefore round down to nearest 8-byte boundary.
	 */
	fit_len = (mtu - net_pkt_ip_hdr_len(pkt)) / 8;

	if (fit_len <= 0) {
		LOG_ERR("No room for IPv4 payload MTU %d hdrs_len %d", mtu,
			net_pkt_ip_hdr_len(pkt));
		return -EINVAL;
	}

	fit_len *= 8;

	pkt_len -= net_pkt_ip_hdr_len(pkt);

	/* Calculate the L4 checksum (if not done already) before the fragmentation. */
	if (!net_pkt_is_chksum_done(pkt)) {
		struct net_pkt_cursor backup;

		net_pkt_cursor_backup(pkt, &backup);
		net_pkt_acknowledge_data(pkt, &frag_access);

		switch (frag_hdr->proto) {
		case IPPROTO_ICMP:
			ret = net_icmpv4_finalize(pkt, true);
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

		net_pkt_cursor_restore(pkt, &backup);
	}

	while (frag_offset < pkt_len) {
		bool final = false;

		if ((frag_offset + fit_len) >= pkt_len) {
			final = true;
			fit_len = (pkt_len - frag_offset);
		}

		ret = send_ipv4_fragment(pkt, rand_id, fit_len, frag_offset, final);
		if (ret < 0) {
			return ret;
		}

		frag_offset += fit_len;
	}

	return 0;
}

enum net_verdict net_ipv4_prepare_for_send(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	struct net_ipv4_hdr *ip_hdr;
	int ret;

	NET_ASSERT(pkt && pkt->buffer);

	ip_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);
	if (!ip_hdr) {
		return NET_DROP;
	}

	/* If we have already fragmented the packet, the ID field will contain a non-zero value
	 * and we can skip other checks.
	 */
	if (ip_hdr->id[0] == 0 && ip_hdr->id[1] == 0) {
		uint16_t mtu = net_if_get_mtu(net_pkt_iface(pkt));
		size_t pkt_len = net_pkt_get_len(pkt);

		mtu = MAX(NET_IPV4_MTU, mtu);

		if (pkt_len > mtu) {
			ret = net_ipv4_send_fragmented_pkt(net_pkt_iface(pkt), pkt, pkt_len, mtu);

			if (ret < 0) {
				LOG_DBG("Cannot fragment IPv4 pkt (%d)", ret);

				if (ret == -ENOMEM || ret == -ENOBUFS || ret == -EPERM) {
					/* Try to send the packet if we could not allocate enough
					 * network packets or if the don't fragment flag is set
					 * and hope the original large packet can be sent OK.
					 */
					goto ignore_frag_error;
				} else {
					/* Other error, drop the packet */
					return NET_DROP;
				}
			}

			/* We need to unref here because we simulate the packet being sent. */
			net_pkt_unref(pkt);

			/* No need to continue with the sending as the packet is now split and
			 * its fragments will be sent separately to the network.
			 */
			return NET_CONTINUE;
		}
	}

ignore_frag_error:

	return NET_OK;
}

void net_ipv4_setup_fragment_buffers(void)
{
	/* Static initialising does not work here because of the array, so we must do it at
	 * runtime.
	 */
	for (int i = 0; i < CONFIG_NET_IPV4_FRAGMENT_MAX_COUNT; i++) {
		k_work_init_delayable(&reassembly[i].timer, reassembly_timeout);
	}
}
