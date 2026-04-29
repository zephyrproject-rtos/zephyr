/*
 * Copyright (c) 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtp, CONFIG_RTP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/igmp.h>
#include <zephyr/net/rtp.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/random/random.h>

#include "ipv4.h"
#include "udp_internal.h"
#include "net_stats.h"

#define PKT_WAIT_TIME    K_MSEC(5)
#define API_LOCK_TIMEOUT K_MSEC(100)

static uint16_t get_next_local_port(void)
{
	static atomic_t next_local_port = ATOMIC_INIT(CONFIG_RTP_LOCAL_PORT_BASE);

	atomic_cas(&next_local_port, UINT16_MAX, CONFIG_RTP_LOCAL_PORT_BASE);

	return (uint16_t)atomic_inc(&next_local_port);
}

static enum net_verdict rtp_conn_cb(struct net_conn *conn, struct net_pkt *pkt,
				    union net_ip_header *ip_hdr, union net_proto_header *proto_hdr,
				    void *user_data)
{
	struct rtp_session *session;
	struct rtp_packet rtp_pkt = {};
	struct net_if *iface;
	size_t len;

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

	len = net_pkt_get_len(pkt);
	switch (pkt->family) {
	case NET_AF_INET:
		if (len < NET_IPV4UDPH_LEN + RTP_MIN_HEADER_LEN) {
			NET_DBG("Invalid RTP packet received of length %d", len);
			return NET_DROP;
		}
		break;
	default:
		/* Only IPv4 supported yet */
		return NET_DROP;
	}

	net_pkt_cursor_init(pkt);
	if (net_pkt_skip(pkt, NET_IPV4UDPH_LEN) < 0) {
		return NET_DROP;
	}

	(void)net_pkt_read_be16(pkt, &rtp_pkt.header.vpxccmpt);
	(void)net_pkt_read_be16(pkt, &rtp_pkt.header.seq);
	(void)net_pkt_read_be32(pkt, &rtp_pkt.header.ts);
	(void)net_pkt_read_be32(pkt, &rtp_pkt.header.ssrc);

	if (rtp_pkt.header.version != RTP_VERSION) {
		NET_DBG("Invalid RTP version (%d)", rtp_pkt.header.version);
		return NET_DROP;
	}

	if (rtp_pkt.header.cc > 0) {
		if (rtp_pkt.header.cc > ARRAY_SIZE(rtp_pkt.header.csrc)) {
			NET_DBG("Size of csrc too small, ignoring. Please increase "
				"CONFIG_RTP_MAX_CSRC_COUNT");
			return NET_DROP;
		}
		if (net_pkt_remaining_data(pkt) < rtp_pkt.header.cc * sizeof(uint32_t)) {
			NET_DBG("Payload too small for cc");
			return NET_DROP;
		}

		for (size_t i = 0; i < rtp_pkt.header.cc; i++) {
			(void)net_pkt_read_be32(pkt, &rtp_pkt.header.csrc[i]);
		}
	}

	if (rtp_pkt.header.x == 1) {
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

		hdr_x->data = net_pkt_cursor_get_pos(pkt);
		(void)net_pkt_skip(pkt, x_data_len);
	}

	rtp_pkt.payload_len = net_pkt_remaining_data(pkt);
	rtp_pkt.payload = net_pkt_cursor_get_pos(pkt);

	if (rtp_pkt.header.p == 1) {
		uint8_t padding;
		(void)net_pkt_skip(pkt, rtp_pkt.payload_len - 1);
		net_pkt_read_u8(pkt, &padding);
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

	net_pkt_unref(pkt);

	session->rtp_context.callback(session, &rtp_pkt, session->rtp_context.user_data);

	return NET_OK;
}

static struct net_pkt *rtp_create_net_pkt(struct rtp_session *session, size_t size)
{
	struct rtp_session_context *rtp_context = &session->rtp_context;
	struct net_sockaddr *source_addr = &rtp_context->source_addr;
	struct net_pkt *pkt;
	int ret;

	switch (source_addr->sa_family) {
	case NET_AF_UNSPEC:
		/* Ignore */
		return NULL;
	case NET_AF_INET:
		struct net_sockaddr_in *source_addr_in = net_sin(source_addr);

		pkt = net_pkt_alloc_with_buffer(rtp_context->iface, size, NET_AF_INET,
						NET_IPPROTO_UDP, PKT_WAIT_TIME);
		if (pkt == NULL) {
			return pkt;
		}

		net_pkt_set_ipv4_ttl(pkt, 0xFF);

		struct net_if_addr *if_addr = &rtp_context->iface->config.ip.ipv4->unicast->ipv4;
		ret = net_ipv4_create(pkt, &if_addr->address.net_in_addr,
				      &source_addr_in->sin_addr);
		if (ret < 0) {
			goto fail;
		}

		ret = net_udp_create(pkt, htons(session->local_port), source_addr_in->sin_port);
		if (ret < 0) {
			goto fail;
		}

		break;
	default:
		NET_DBG("Only IPv4 supported yet");
		return NULL;
	}

	return pkt;
fail:
	NET_DBG("Message creation failed");
	net_pkt_unref(pkt);

	return NULL;
}

static int rtp_session_start_rx(struct rtp_session *session)
{
	struct net_sockaddr *sink_addr;
	uint16_t sink_port;
	int ret;

	__ASSERT_NO_MSG(session != NULL);
	sink_addr = &session->rtp_context.sink_addr;

	switch (sink_addr->sa_family) {
	case NET_AF_INET:
		struct net_sockaddr_in *sink_addr_in = net_sin(sink_addr);
		sink_port = sink_addr_in->sin_port;

		if (net_ipv4_is_addr_unspecified(&sink_addr_in->sin_addr)) {
			NET_DBG("Invalid ipv4 address");
			return -EINVAL;
		}

		if (!net_ipv4_is_addr_mcast(&sink_addr_in->sin_addr)) {
			goto callbacks;
		}

		ret = net_ipv4_igmp_join(session->rtp_context.iface, &sink_addr_in->sin_addr, NULL);
		if (ret < 0 && ret != -EALREADY) {
			NET_DBG("Failed to join ipv4 igmp(%d)", ret);
			return ret;
		}

		break;
	default:
		NET_DBG("Only IPv4 supported yet");
		return -ENOTSUP;
	}

callbacks:
	ret = net_udp_register(sink_addr->sa_family, NULL, sink_addr, 0, htons(sink_port), NULL,
			       rtp_conn_cb, (void *)session, &session->net_handle_rtp);
	if (ret < 0) {
		NET_DBG("UDP callback registration failed (%d)", ret);
		return ret;
	}

	/* TODO: Register rtcp */

#if CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG
	switch (sink_addr->sa_family) {
	case NET_AF_INET:
		struct net_sockaddr_in *sink_addr_in = net_sin(sink_addr);
		char buf[NET_IPV4_ADDR_LEN];

		NET_DBG("%s started receiver on %s:%d", session->name,
			net_addr_ntop(NET_AF_INET, &sink_addr_in->sin_addr, buf, sizeof(buf)),
			net_ntohs(sink_addr_in->sin_port));
		break;
	default:
		/* Only ipv4 support yet */
	}
#endif

	return 0;
}

static int rtp_session_start_tx(struct rtp_session *session)
{
	__ASSERT_NO_MSG(session != NULL);

	/* Create random timestamp and ssrc */
	session->timestamp = sys_rand32_get();
	session->ssrc = sys_rand32_get();
	session->sequence_number = sys_rand16_get();

	if (session->local_port == 0) {
		/* Create new local port */
		session->local_port = get_next_local_port();
	}

	session->csrc_len = 0;

	/* TODO: check with RTCP if ssrc is already known */

	return 0;
}

static int rtp_session_stop_rx(struct rtp_session *session)
{
	struct net_sockaddr *sink_addr;
	int ret;

	__ASSERT_NO_MSG(session != NULL);
	sink_addr = &session->rtp_context.sink_addr;

	switch (sink_addr->sa_family) {
	case NET_AF_UNSPEC:
		/* Ignore - rx not configured */
		return 0;
	case NET_AF_INET:
		struct net_sockaddr_in *sink_addr_in = net_sin(sink_addr);

		if (net_ipv4_is_addr_unspecified(&sink_addr_in->sin_addr)) {
			/* Ignore */
			return 0;
		}

		if (!net_ipv4_is_addr_mcast(&sink_addr_in->sin_addr)) {
			goto callbacks;
		}

		ret = net_ipv4_igmp_leave(session->rtp_context.iface, &sink_addr_in->sin_addr);
		if (ret < 0 && ret != -EALREADY) {
			NET_DBG("Failed to leave ipv4 igmp(%d)", ret);
			return ret;
		}

		break;
	default:
		NET_DBG("Only IPv4 supported yet");
		return -ENOTSUP;
		break;
	}

callbacks:
	ret = net_udp_unregister(session->net_handle_rtp);
	if (ret < 0) {
		NET_DBG("Failed to deregister udp callback");
		return ret;
	}

	/* TODO: Unregister rtcp */

#if CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG
	switch (sink_addr->sa_family) {
	case NET_AF_INET:
		struct net_sockaddr_in *sink_addr_in = net_sin(sink_addr);
		char buf[NET_IPV4_ADDR_LEN];

		NET_DBG("%s stopped receiver on %s:%d", session->name,
			net_addr_ntop(NET_AF_INET, &sink_addr_in->sin_addr, buf, sizeof(buf)),
			net_ntohs(sink_addr_in->sin_port));
		break;
	default:
		/* Only ipv4 support yet */
	}
#endif

	return 0;
}

static int rtp_session_stop_tx(struct rtp_session *session)
{
	__ASSERT_NO_MSG(session != NULL);

	session->timestamp = 0;
	session->ssrc = 0;
	session->sequence_number = 0;
	session->csrc_len = 0;

	memset(session->csrc, 0, sizeof(session->csrc));

	/* TODO: let RTCP know we stopped */

	return 0;
}

/* API FUNCTIONS */

int rtp_init_header_extension(struct rtp_header_extension *hdr_x, uint16_t definition,
			      uint8_t *data, size_t len)
{
	if (hdr_x == NULL) {
		return -EINVAL;
	}

	if (len % sizeof(uint32_t) != 0) {
		NET_DBG("Data len in header extension needs to be alligned to uint32_t");
		return -EINVAL;
	}

	hdr_x->definition = definition;
	hdr_x->length = len / sizeof(uint32_t);
	hdr_x->data = data;

	return 0;
}

int rtp_session_init(struct rtp_session *session, struct net_if *iface,
		     struct net_sockaddr *sink_addr, struct net_sockaddr *source_addr,
		     uint8_t payload_type, uint8_t marker, rtp_rx_cb_t callback, void *user_data)
{
	struct rtp_session_context *rtp_context;

	if (session == NULL) {
		return -EINVAL;
	}

	if (sink_addr == NULL && source_addr == NULL) {
		return -EINVAL;
	}

	rtp_context = &session->rtp_context;
	rtp_context->iface = iface;

	if (sink_addr != NULL) {
		memcpy(&rtp_context->sink_addr, sink_addr, sizeof(rtp_context->sink_addr));
		rtp_context->callback = callback;
		rtp_context->user_data = user_data;
	} else {
		memset(&rtp_context->sink_addr, 0, sizeof(rtp_context->sink_addr));
	}

	if (source_addr != NULL) {
		memcpy(&rtp_context->source_addr, source_addr, sizeof(rtp_context->source_addr));
		rtp_context->payload_type = payload_type;
		rtp_context->marker = marker;
	} else {
		memset(&rtp_context->source_addr, 0, sizeof(rtp_context->source_addr));
	}

	return k_mutex_init(&session->lock);
}

int rtp_session_start(struct rtp_session *session)
{
	int ret;

	if (session == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&session->lock, API_LOCK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Failed to take session lock (%d)", ret);
		return ret;
	}

	/* Start receiver if configured */
	if (session->rtp_context.sink_addr.sa_family != NET_AF_UNSPEC) {
		ret = rtp_session_start_rx(session);
		if (ret < 0) {
			goto unlock;
		}
	}

	/* Start transmitter if configured */
	if (session->rtp_context.source_addr.sa_family != NET_AF_UNSPEC) {
		ret = rtp_session_start_tx(session);
		if (ret < 0) {
			goto unlock;
		}
	}

unlock:
	(void)k_mutex_unlock(&session->lock);
	return ret;
}

int rtp_session_stop(struct rtp_session *session)
{
	int ret;

	if (session == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&session->lock, API_LOCK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Failed to take session lock (%d)", ret);
		return ret;
	}

	/* Stop receiver if configured */
	if (session->rtp_context.sink_addr.sa_family != NET_AF_UNSPEC) {
		ret = rtp_session_stop_rx(session);
		if (ret < 0) {
			goto unlock;
		}
	}

	/* Stop transmitter if configured */
	if (session->rtp_context.source_addr.sa_family != NET_AF_UNSPEC) {
		ret = rtp_session_stop_tx(session);
		if (ret < 0) {
			goto unlock;
		}
	}
unlock:
	(void)k_mutex_unlock(&session->lock);
	return ret;
}

int rtp_session_add_csrc(struct rtp_session *session, uint32_t csrc)
{
#if CONFIG_RTP_MAX_CSRC_COUNT == 0
	return -ENOTSUP;
#else
	int ret;

	if (session == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&session->lock, API_LOCK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Failed to take session lock (%d)", ret);
		return ret;
	}

	if (session->csrc_len >= ARRAY_SIZE(session->csrc)) {
		NET_DBG("csrc size too small. Please increase CONFIG_RTP_MAX_CSRC_COUNT");
		ret = -ENOMEM;
		goto unlock;
	}

	session->csrc[session->csrc_len++] = csrc;
	ret = 0;

unlock:
	(void)k_mutex_unlock(&session->lock);
	return ret;
#endif /* CONFIG_RTP_MAX_CSRC_COUNT == 0 */
}

int rtp_session_remove_csrc(struct rtp_session *session, uint32_t csrc)
{
#if CONFIG_RTP_MAX_CSRC_COUNT == 0
	return -ENOTSUP;
#else
	int ret;

	if (session == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&session->lock, API_LOCK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Failed to take session lock (%d)", ret);
		return ret;
	}

	size_t i;
	for (i = 0; i < session->csrc_len; i++) {
		if (session->csrc[i] == csrc) {
			break;
		}
	}

	if (i == session->csrc_len) {
		/* csrc not found */
		ret = -EINVAL;
		goto unlock;
	}

	if (i == session->csrc_len - 1) {
		session->csrc_len--;
		ret = 0;
		goto unlock;
	}

	memmove(&session->csrc[i], &session->csrc[i + 1], session->csrc_len - i - 1);
	session->csrc_len--;
	ret = 0;

unlock:
	(void)k_mutex_unlock(&session->lock);
	return ret;
#endif /* CONFIG_RTP_MAX_CSRC_COUNT == 0 */
}

int rtp_session_send(struct rtp_session *session, void *data, size_t len, uint32_t delta_ts,
		     size_t padding, struct rtp_header_extension *hdr_x)
{
	struct rtp_session_context *rtp_context;
	struct rtp_header rtp_header = {};
	struct net_pkt *pkt;
	int ret;

	if (session == NULL) {
		return -EINVAL;
	}

	if (data == NULL && len > 0) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&session->lock, API_LOCK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Failed to take session lock (%d)", ret);
		return ret;
	}
	rtp_context = &session->rtp_context;

	/* Timestamp is increased, regardless of whether the block is transmitted or not */
	session->timestamp += delta_ts;

	pkt = rtp_create_net_pkt(session, CONFIG_NET_BUF_DATA_SIZE);
	if (pkt == NULL) {
		NET_DBG("Net pkt is NULL");

		ret = -EIO;
		goto fail;
	}

	rtp_header.version = RTP_VERSION;
	rtp_header.p = (padding > 0) ? 1 : 0;
	rtp_header.cc = session->csrc_len;
	rtp_header.m = rtp_context->marker;
	rtp_header.pt = rtp_context->payload_type;

	/* RFC allows these to overflow */
	rtp_header.seq = session->sequence_number++;
	rtp_header.ts = session->timestamp;

	rtp_header.ssrc = session->ssrc;

	if (hdr_x != NULL) {
		rtp_header.x = 1;
	}

	(void)net_pkt_write_be16(pkt, rtp_header.vpxccmpt);
	(void)net_pkt_write_be16(pkt, rtp_header.seq);
	(void)net_pkt_write_be32(pkt, rtp_header.ts);
	(void)net_pkt_write_be32(pkt, rtp_header.ssrc);

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
	for (size_t i = 0; i < rtp_header.cc; i++) {
		(void)net_pkt_write_be32(pkt, session->csrc[i]);
	}
#endif /* CONFIG_RTP_MAX_CSRC_COUNT > 0 */

	if (hdr_x != NULL) {
		size_t x_data_len = hdr_x->length * sizeof(uint32_t);

		if (hdr_x->data == NULL) {
			ret = -EINVAL;
			goto fail;
		}

		if (net_pkt_available_buffer(pkt) < 2 * sizeof(uint16_t) + x_data_len) {
			NET_DBG("Extension header length too large");
			ret = -ENOMEM;
			goto fail;
		}

		(void)net_pkt_write_be16(pkt, hdr_x->definition);
		(void)net_pkt_write_be16(pkt, hdr_x->length);
		(void)net_pkt_write(pkt, hdr_x->data, x_data_len);
	}

	if (net_pkt_available_buffer(pkt) < len) {
		NET_DBG("Payload of len %d too large", len);
		ret = -ENOMEM;
		goto fail;
	}
	(void)net_pkt_write(pkt, data, len);

	if (padding > 0) {
		if (padding > UINT8_MAX) {
			NET_DBG("Can only add max of 255 padding bytes");
			ret = -EINVAL;
			goto fail;
		}

		if (net_pkt_available_buffer(pkt) < padding) {
			NET_DBG("Padding of size %d too large", padding);
			ret = -ENOMEM;
			goto fail;
		}

		(void)net_pkt_memset(pkt, 0, padding - 1);
		/* Padding count includes padding byte */
		(void)net_pkt_write_u8(pkt, padding);
	}

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, NET_IPPROTO_UDP);

	ret = net_send_data(pkt);
	if (ret < 0) {
		NET_DBG("Failed to send rtp (%d)", ret);
		goto fail;
	}

	net_stats_update_udp_sent(rtp_context->iface);

	(void)k_mutex_unlock(&session->lock);
	return 0;

fail:
	if (pkt != NULL) {
		net_pkt_unref(pkt);
	}

	(void)k_mutex_unlock(&session->lock);
	return ret;
}
