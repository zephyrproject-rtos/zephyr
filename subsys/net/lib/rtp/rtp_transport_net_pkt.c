/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/** @file rtp_transport_net_pkt.c
 *
 * @brief Internal functions to handle transport via raw net_pkt.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtp_net_pkt, CONFIG_RTP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/igmp.h>
#ifdef CONFIG_NET_NATIVE_IPV6
#include <zephyr/net/mld.h>
#endif /* CONFIG_NET_IPV6 */
#include <zephyr/net/net_log.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/rtp.h>
#include <zephyr/random/random.h>

#ifdef CONFIG_NET_NATIVE_IPV4
#include "ipv4.h"
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_NATIVE_IPV6
#include "ipv6.h"
#endif /* CONFIG_NET_IPV6 */
#include "net_stats.h"
#include "udp_internal.h"

BUILD_ASSERT(CONFIG_NET_BUF_DATA_SIZE >= RTP_MIN_HEADER_LEN);

#define PKT_WAIT_TIME K_MSEC(1)

static uint16_t get_next_local_port(void)
{
	static atomic_t curr_local_port = ATOMIC_INIT(CONFIG_RTP_LOCAL_PORT_BASE);
	atomic_val_t port = atomic_inc(&curr_local_port);

	if (port >= UINT16_MAX) {
		atomic_set(&curr_local_port, CONFIG_RTP_LOCAL_PORT_BASE);
		port = CONFIG_RTP_LOCAL_PORT_BASE;
	}

	return (uint16_t)port;
}

static enum net_verdict rtp_conn_cb(struct net_conn *conn, struct net_pkt *pkt,
				    union net_ip_header *ip_hdr, union net_proto_header *proto_hdr,
				    void *user_data)
{
	struct rtp_session *session;
	struct rtp_packet rtp_pkt = {};
	struct net_if *iface;

	ARG_UNUSED(ip_hdr);
	ARG_UNUSED(proto_hdr);

	__ASSERT_NO_MSG(user_data != NULL);
	session = (struct rtp_session *)user_data;

	if (conn == NULL) {
		NET_DBG("Invalid connection");
		return NET_DROP;
	}

	if (pkt == NULL) {
		NET_DBG("Invalid net packet");
		return NET_DROP;
	}

	iface = net_pkt_iface(pkt);
	if (iface == NULL) {
		NET_DBG("no iface");
		return NET_DROP;
	}

	if (session->rtp_context.callback == NULL) {
		/* Ignore */
		return NET_CONTINUE;
	}

	switch (pkt->family) {
#ifdef CONFIG_NET_NATIVE_IPV4
	case NET_AF_INET:
		break;
#endif /* CONFIG_NET_NATIVE_IPV4 */
#ifdef CONFIG_NET_NATIVE_IPV6
	case NET_AF_INET6:
		break;
#endif /* CONFIG_NET_NATIVE_IPV6 */
	default:
		/* Ignore */
		return NET_DROP;
	}

	if (net_pkt_remaining_data(pkt) < RTP_MIN_HEADER_LEN) {
		NET_DBG("Invalid RTP packet received of length %zu", net_pkt_remaining_data(pkt));
		return NET_DROP;
	}

	(void)net_pkt_read_u8(pkt, &rtp_pkt.header.vpxcc);
	(void)net_pkt_read_u8(pkt, &rtp_pkt.header.mpt);
	(void)net_pkt_read_be16(pkt, &rtp_pkt.header.seq);
	(void)net_pkt_read_be32(pkt, &rtp_pkt.header.ts);
	(void)net_pkt_read_be32(pkt, &rtp_pkt.header.ssrc);

	if (rtp_header_get_v(&rtp_pkt.header) != RTP_VERSION) {
		NET_DBG("Invalid RTP version (%d)", rtp_header_get_v(&rtp_pkt.header));
		return NET_DROP;
	}

	if (rtp_header_get_cc(&rtp_pkt.header) > 0) {
		size_t csrc_count = rtp_header_get_cc(&rtp_pkt.header);
		size_t csrc_skip = 0;

		if (net_pkt_remaining_data(pkt) < csrc_count * sizeof(uint32_t)) {
			NET_DBG("Payload too small for cc");
			return NET_DROP;
		}

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
		if (csrc_count > ARRAY_SIZE(rtp_pkt.header.csrc)) {
			NET_DBG("Size of csrc too small, ignoring following csrcs. Please increase "
				"CONFIG_RTP_MAX_CSRC_COUNT");
			csrc_skip = csrc_count - ARRAY_SIZE(rtp_pkt.header.csrc);
			csrc_count = ARRAY_SIZE(rtp_pkt.header.csrc);
			rtp_header_set_cc(&rtp_pkt.header, ARRAY_SIZE(rtp_pkt.header.csrc));
		}

		for (size_t i = 0; i < csrc_count; i++) {
			(void)net_pkt_read_be32(pkt, &rtp_pkt.header.csrc[i]);
		}
#else
		NET_DBG("Received packet with cc > 0, but CONFIG_RTP_MAX_CSRC_COUNT = 0");

		csrc_skip = csrc_count;
		rtp_header_set_cc(&rtp_pkt.header, 0);
#endif /* CONFIG_RTP_MAX_CSRC_COUNT > 0 */

		(void)net_pkt_skip(pkt, sizeof(uint32_t) * csrc_skip);
	}

	if (rtp_header_get_x(&rtp_pkt.header) == 1) {
		struct rtp_header_extension *hdr_x = &rtp_pkt.header.header_extension;
		size_t x_data_len;

		if (net_pkt_remaining_data(pkt) < 2 * sizeof(uint16_t)) {
			NET_DBG("Payload too small for header extension");
			return NET_DROP;
		}

		(void)net_pkt_read_be16(pkt, &hdr_x->definition);
		(void)net_pkt_read_be16(pkt, &hdr_x->length);
		x_data_len = hdr_x->length * sizeof(uint32_t);

		if (net_pkt_remaining_data(pkt) < x_data_len) {
			NET_DBG("RTP extension header length too large for pkt");
			return NET_DROP;
		}

		if (!net_pkt_is_contiguous(pkt, x_data_len)) {
			NET_DBG("RTP extension header not contiguous");
			return NET_DROP;
		}
		hdr_x->data = net_pkt_cursor_get_pos(pkt);
		(void)net_pkt_skip(pkt, x_data_len);
	}

	rtp_pkt.payload_len = net_pkt_remaining_data(pkt);

	if (!net_pkt_is_contiguous(pkt, rtp_pkt.payload_len)) {
		NET_DBG("RTP payload not contiguous");
		return NET_DROP;
	}
	rtp_pkt.payload = net_pkt_cursor_get_pos(pkt);

	if (rtp_header_get_p(&rtp_pkt.header) == 1) {
		uint8_t padding;

		if (rtp_pkt.payload_len == 0) {
			NET_DBG("Padding flag is set but no payload");
			return NET_DROP;
		}

		(void)net_pkt_skip(pkt, rtp_pkt.payload_len - 1);
		(void)net_pkt_read_u8(pkt, &padding);
		if (padding == 0) {
			NET_DBG("Padding flag is set but padding is 0");
			return NET_DROP;
		}

		if (padding > rtp_pkt.payload_len) {
			NET_DBG("Padding larger than payload");
			return NET_DROP;
		}

		rtp_pkt.payload_len -= padding;
	}

#ifdef CONFIG_NET_PKT_TIMESTAMP
	memcpy(&rtp_pkt.timestamp, &pkt->timestamp, sizeof(rtp_pkt.timestamp));
#endif

	session->rtp_context.callback(session, &rtp_pkt, session->rtp_context.user_data);

	net_pkt_unref(pkt);

	return NET_OK;
}

static struct net_pkt *rtp_create_net_pkt(struct rtp_session *session, size_t size)
{
	struct rtp_session_context *rtp_context = &session->rtp_context;
	struct net_sockaddr_storage *sock_addr = &rtp_context->sock_addr;
	struct net_pkt *pkt;
	int ret;

	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_NATIVE_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *sock_addr_in = net_sin(net_sad(sock_addr));
		const struct net_in_addr *if_addr;

		pkt = net_pkt_alloc_with_buffer(rtp_context->iface, size, NET_AF_INET,
						NET_IPPROTO_UDP, PKT_WAIT_TIME);
		if (pkt == NULL) {
			return NULL;
		}

		net_pkt_set_ipv4_ttl(pkt, CONFIG_RTP_MCAST_TTL);

		if_addr = net_if_ipv4_select_src_addr(rtp_context->iface, &sock_addr_in->sin_addr);
		if (if_addr == NULL) {
			goto fail;
		}
		ret = net_ipv4_create(pkt, if_addr, &sock_addr_in->sin_addr);
		if (ret < 0) {
			goto fail;
		}

		ret = net_udp_create(pkt, net_htons(session->local_port), sock_addr_in->sin_port);
		if (ret < 0) {
			goto fail;
		}

		break;
	}
#endif /* CONFIG_NET_NATIVE_IPV4 */
#ifdef CONFIG_NET_NATIVE_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *sock_addr_in6 = net_sin6(net_sad(sock_addr));
		const struct net_in6_addr *if_addr6;

		pkt = net_pkt_alloc_with_buffer(rtp_context->iface, size, NET_AF_INET6,
						NET_IPPROTO_UDP, PKT_WAIT_TIME);
		if (pkt == NULL) {
			return NULL;
		}

		net_pkt_set_ipv6_hop_limit(pkt, CONFIG_RTP_MCAST_TTL);

		if_addr6 =
			net_if_ipv6_select_src_addr(rtp_context->iface, &sock_addr_in6->sin6_addr);
		if (if_addr6 == NULL) {
			goto fail;
		}
		ret = net_ipv6_create(pkt, if_addr6, &sock_addr_in6->sin6_addr);
		if (ret < 0) {
			goto fail;
		}

		ret = net_udp_create(pkt, net_htons(session->local_port), sock_addr_in6->sin6_port);
		if (ret < 0) {
			goto fail;
		}

		break;
	}
#endif /* CONFIG_NET_NATIVE_IPV6 */
	default:
		NET_DBG("Family %s not supported", net_family2str(sock_addr->ss_family));
		return NULL;
	}

	return pkt;
fail:
	NET_DBG("Message creation failed");
	net_pkt_unref(pkt);

	return NULL;
}

int rtp_transport_net_pkt_init(struct rtp_session *session)
{
	ARG_UNUSED(session);

	return 0;
}

int rtp_transport_net_pkt_start_rx(struct rtp_session *session)
{
	struct net_sockaddr_storage *sock_addr;
	struct net_sockaddr_storage *udp_local_addr;
	uint16_t sock_port;
	int ret;

	sock_addr = &session->rtp_context.sock_addr;

	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_NATIVE_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *sock_addr_in = net_sin(net_sad(sock_addr));

		sock_port = sock_addr_in->sin_port;

		if (!net_ipv4_is_addr_mcast(&sock_addr_in->sin_addr)) {
			udp_local_addr = NULL;
			goto callbacks;
		}

		ret = net_ipv4_igmp_join(session->rtp_context.iface, &sock_addr_in->sin_addr, NULL);
		if (ret < 0 && ret != -EALREADY) {
			NET_DBG("Failed to join ipv4 igmp(%d)", ret);
			return ret;
		}

		udp_local_addr = sock_addr;
		break;
	}
#endif /* CONFIG_NET_NATIVE_IPV4 */
#ifdef CONFIG_NET_NATIVE_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *sock_addr_in6 = net_sin6(net_sad(sock_addr));

		sock_port = sock_addr_in6->sin6_port;

		if (!net_ipv6_is_addr_mcast(&sock_addr_in6->sin6_addr)) {
			udp_local_addr = NULL;
			goto callbacks;
		}

		ret = net_ipv6_mld_join(session->rtp_context.iface, &sock_addr_in6->sin6_addr);
		if (ret < 0 && ret != -EALREADY) {
			NET_DBG("Failed to join ipv6 mld (%d)", ret);
			return ret;
		}

		udp_local_addr = sock_addr;
		break;
	}
#endif /* CONFIG_NET_NATIVE_IPV6 */
	default:
		NET_DBG("Family %s not supported", net_family2str(sock_addr->ss_family));
		return -ENOTSUP;
	}

callbacks:
	ret = net_udp_register(sock_addr->ss_family, NULL, net_sad(udp_local_addr), 0,
			       net_htons(sock_port), NULL, rtp_conn_cb, (void *)session,
			       &session->transport.net_handle_rtp);
	if (ret < 0) {
		NET_DBG("UDP callback registration failed (%d)", ret);
		return ret;
	}

	return 0;
}

int rtp_transport_net_pkt_start_tx(struct rtp_session *session)
{
	if (session->local_port == 0) {
		/* Create new local port */
		session->local_port = get_next_local_port();
	}

	return 0;
}

int rtp_transport_net_pkt_stop_rx(struct rtp_session *session)
{
	struct net_sockaddr_storage *sock_addr;
	int ret;

	__ASSERT_NO_MSG(session != NULL);
	sock_addr = &session->rtp_context.sock_addr;

	if (session->transport.net_handle_rtp == NULL) {
		/* Already stopped */
		return 0;
	}

	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_NATIVE_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *sock_addr_in = net_sin(net_sad(sock_addr));

		if (!net_ipv4_is_addr_mcast(&sock_addr_in->sin_addr)) {
			goto callbacks;
		}

		ret = net_ipv4_igmp_leave(session->rtp_context.iface, &sock_addr_in->sin_addr);
		if (ret < 0 && ret != -EALREADY) {
			NET_DBG("Failed to leave ipv4 igmp(%d)", ret);
			goto callbacks;
		}

		break;
	}
#endif /* CONFIG_NET_NATIVE_IPV4 */
#ifdef CONFIG_NET_NATIVE_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *sock_addr_in6 = net_sin6(net_sad(sock_addr));

		if (!net_ipv6_is_addr_mcast(&sock_addr_in6->sin6_addr)) {
			goto callbacks;
		}

		ret = net_ipv6_mld_leave(session->rtp_context.iface, &sock_addr_in6->sin6_addr);
		if (ret < 0 && ret != -EALREADY) {
			NET_DBG("Failed to leave ipv6 mld (%d)", ret);
			goto callbacks;
		}

		break;
	}
#endif /* CONFIG_NET_NATIVE_IPV6 */
	default:
		NET_DBG("Family %s not supported", net_family2str(sock_addr->ss_family));
		return -ENOTSUP;
	}

callbacks:
	ret = net_udp_unregister(session->transport.net_handle_rtp);
	if (ret < 0) {
		NET_DBG("Failed to deregister udp callback");
		return ret;
	}
	session->transport.net_handle_rtp = NULL;

	return 0;
}

int rtp_transport_net_pkt_stop_tx(struct rtp_session *session)
{
	ARG_UNUSED(session);

	return 0;
}

int rtp_transport_net_pkt_send(struct rtp_session *session, struct rtp_packet *rtp_pkt,
			       uint8_t padding)
{
	struct rtp_session_context *rtp_context;
	struct net_pkt *pkt;
	int ret;

	rtp_context = &session->rtp_context;

	pkt = rtp_create_net_pkt(session, CONFIG_NET_BUF_DATA_SIZE);
	if (pkt == NULL) {
		NET_DBG("Net pkt is NULL");
		ret = -EIO;
		goto fail;
	}

	(void)net_pkt_write_u8(pkt, rtp_pkt->header.vpxcc);
	(void)net_pkt_write_u8(pkt, rtp_pkt->header.mpt);
	(void)net_pkt_write_be16(pkt, rtp_pkt->header.seq);
	(void)net_pkt_write_be32(pkt, rtp_pkt->header.ts);
	(void)net_pkt_write_be32(pkt, rtp_pkt->header.ssrc);

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
	for (size_t i = 0; i < rtp_header_get_cc(&rtp_pkt->header); i++) {
		(void)net_pkt_write_be32(pkt, rtp_pkt->header.csrc[i]);
	}
#endif /* CONFIG_RTP_MAX_CSRC_COUNT > 0 */

	if (rtp_header_get_x(&rtp_pkt->header) != 0) {
		struct rtp_header_extension *hdr_x = &rtp_pkt->header.header_extension;
		size_t x_data_len = hdr_x->length * sizeof(uint32_t);

		__ASSERT_NO_MSG(hdr_x->data != NULL);

		if (net_pkt_available_buffer(pkt) < 2 * sizeof(uint16_t) + x_data_len) {
			NET_DBG("Extension header length too large");
			ret = -ENOMEM;
			goto fail;
		}

		(void)net_pkt_write_be16(pkt, hdr_x->definition);
		(void)net_pkt_write_be16(pkt, hdr_x->length);
		(void)net_pkt_write(pkt, hdr_x->data, x_data_len);
	}

	if (net_pkt_available_buffer(pkt) < rtp_pkt->payload_len) {
		NET_DBG("Payload of len %zu too large", rtp_pkt->payload_len);
		ret = -ENOMEM;
		goto fail;
	}
	(void)net_pkt_write(pkt, rtp_pkt->payload, rtp_pkt->payload_len);

	if (padding > 0) {
		if (net_pkt_available_buffer(pkt) < padding) {
			NET_DBG("Padding of size %u too large", padding);
			ret = -ENOMEM;
			goto fail;
		}

		(void)net_pkt_memset(pkt, 0, padding - 1);
		/* Padding count includes padding byte */
		(void)net_pkt_write_u8(pkt, padding);
	}

	net_pkt_cursor_init(pkt);

	switch (rtp_context->sock_addr.ss_family) {
#ifdef CONFIG_NET_NATIVE_IPV4
	case NET_AF_INET:
		net_ipv4_finalize(pkt, NET_IPPROTO_UDP);
		break;
#endif /* CONFIG_NET_NATIVE_IPV4 */
#ifdef CONFIG_NET_NATIVE_IPV6
	case NET_AF_INET6:
		net_ipv6_finalize(pkt, NET_IPPROTO_UDP);
		break;
#endif /* CONFIG_NET_NATIVE_IPV6 */
	default:
		break;
	}

	ret = net_send_data(pkt);
	if (ret < 0) {
		NET_DBG("Failed to send rtp (%d)", ret);
		goto fail;
	}

	net_stats_update_udp_sent(rtp_context->iface);

	return 0;

fail:
	if (pkt != NULL) {
		net_pkt_unref(pkt);
	}

	return ret;
}
