/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtp, CONFIG_RTP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/igmp.h>
#include <zephyr/net/rtp.h>
#include <zephyr/net/net_log.h>
#include <zephyr/random/random.h>

#include <net_private.h>
#include "rtp_transport.h"

#define API_LOCK_TIMEOUT K_MSEC(100)

static int rtp_session_start_rx(struct rtp_session *session)
{
	int ret;

	__ASSERT_NO_MSG(session != NULL);

	ret = rtp_transport_start_rx(session);
	if (ret < 0) {
		return ret;
	}

#if CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG
	struct net_sockaddr_storage *sock_addr = &session->rtp_context.sock_addr;

	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *sock_addr_in = net_sin(net_sad(sock_addr));

		NET_DBG("%s started receiver on %s:%d", session->name,
			net_sprint_ipv4_addr((void *)&sock_addr_in->sin_addr),
			net_ntohs(sock_addr_in->sin_port));
		break;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *sock_addr_in6 = net_sin6(net_sad(sock_addr));

		NET_DBG("%s started receiver on [%s]:%d", session->name,
			net_sprint_ipv6_addr((void *)&sock_addr_in6->sin6_addr),
			net_ntohs(sock_addr_in6->sin6_port));
		break;
	}
#endif /* CONFIG_NET_IPV6 */
	default:
		/* Ignore */
	}
#endif /* CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG */

	return 0;
}

static int rtp_session_start_tx(struct rtp_session *session)
{
	struct net_sockaddr_storage *sock_addr;
	int ret;

	__ASSERT_NO_MSG(session != NULL);

	sock_addr = &session->rtp_context.sock_addr;

	/* Create random timestamp and sequence number */
	session->timestamp = sys_rand32_get();
	session->sequence_number = sys_rand16_get();

	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *sock_addr_in = net_sin(net_sad(sock_addr));

		if (net_ipv4_is_addr_unspecified(&sock_addr_in->sin_addr)) {
			NET_DBG("Invalid ipv4 address");
			return -EINVAL;
		}
		break;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *sock_addr_in6 = net_sin6(net_sad(sock_addr));

		if (net_ipv6_is_addr_unspecified(&sock_addr_in6->sin6_addr)) {
			NET_DBG("Invalid ipv6 address");
			return -EINVAL;
		}
		break;
	}
#endif /* CONFIG_NET_IPV6 */
	default:
		NET_DBG("Family %s not supported", net_family2str(sock_addr->ss_family));
		return -ENOTSUP;
	}

	ret = rtp_transport_start_tx(session);
	if (ret < 0) {
		return ret;
	}

#if CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG
	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *sock_addr_in = net_sin(net_sad(sock_addr));

		NET_DBG("%s started transmitter to %s:%d", session->name,
			net_sprint_ipv4_addr((void *)&sock_addr_in->sin_addr),
			net_ntohs(sock_addr_in->sin_port));
		break;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *sock_addr_in6 = net_sin6(net_sad(sock_addr));

		NET_DBG("%s started transmitter to [%s]:%d", session->name,
			net_sprint_ipv6_addr((void *)&sock_addr_in6->sin6_addr),
			net_ntohs(sock_addr_in6->sin6_port));
		break;
	}
#endif /* CONFIG_NET_IPV6 */
	default:
		/* Ignore */
	}
#endif /* CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG */

	return 0;
}

static int rtp_session_stop_rx(struct rtp_session *session)
{
	int ret;

	__ASSERT_NO_MSG(session != NULL);

	ret = rtp_transport_stop_rx(session);
	if (ret < 0) {
		return ret;
	}

#if CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG
	struct net_sockaddr_storage *sock_addr = &session->rtp_context.sock_addr;

	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *sock_addr_in = net_sin(net_sad(sock_addr));

		NET_DBG("%s stopped receiver on %s:%d", session->name,
			net_sprint_ipv4_addr((void *)&sock_addr_in->sin_addr),
			net_ntohs(sock_addr_in->sin_port));
		break;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *sock_addr_in6 = net_sin6(net_sad(sock_addr));

		NET_DBG("%s stopped receiver on [%s]:%d", session->name,
			net_sprint_ipv6_addr((void *)&sock_addr_in6->sin6_addr),
			net_ntohs(sock_addr_in6->sin6_port));
		break;
	}
#endif /* CONFIG_NET_IPV6 */
	default:
		/* Ignore */
	}
#endif

	return 0;
}

static int rtp_session_stop_tx(struct rtp_session *session)
{
	int ret;

	__ASSERT_NO_MSG(session != NULL);

	ret = rtp_transport_stop_tx(session);
	if (ret < 0) {
		return ret;
	}

	session->timestamp = 0;
	session->sequence_number = 0;
	session->csrc_len = 0;

	memset(session->csrc, 0, sizeof(session->csrc));

#if CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG
	struct net_sockaddr_storage *sock_addr = &session->rtp_context.sock_addr;

	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *sock_addr_in = net_sin(net_sad(sock_addr));

		NET_DBG("%s stopped transmitter to %s:%d", session->name,
			net_sprint_ipv4_addr((void *)&sock_addr_in->sin_addr),
			net_ntohs(sock_addr_in->sin_port));
		break;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *sock_addr_in6 = net_sin6(net_sad(sock_addr));

		NET_DBG("%s stopped transmitter to [%s]:%d", session->name,
			net_sprint_ipv6_addr((void *)&sock_addr_in6->sin6_addr),
			net_ntohs(sock_addr_in6->sin6_port));
		break;
	}
#endif /* CONFIG_NET_IPV6 */
	default:
		/* Ignore */
	}
#endif /* CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG */

	return 0;
}

/* API FUNCTIONS */

int rtp_init_header_extension(struct rtp_header_extension *hdr_x, uint16_t definition,
			      uint8_t *data, size_t len)
{
	if (hdr_x == NULL) {
		return -EINVAL;
	}

	if (len > 0 && data == NULL) {
		return -EINVAL;
	}

	if (len % sizeof(uint32_t) != 0) {
		NET_DBG("Data len in header extension needs to be aligned to uint32_t");
		return -EINVAL;
	}

	hdr_x->definition = definition;
	hdr_x->length = len / sizeof(uint32_t);
	hdr_x->data = data;

	return 0;
}

int rtp_session_init(struct rtp_session *session, struct net_if *iface,
		     struct net_sockaddr *sock_addr, enum rtp_role role, uint8_t payload_type,
		     rtp_rx_cb_t callback, void *user_data, enum rtp_transport_type transport_type)
{
	struct rtp_session_context *rtp_context;
	size_t addr_size;
	int ret;

	if (session == NULL || sock_addr == NULL) {
		return -EINVAL;
	}

	if ((role == RTP_ROLE_SINK || role == RTP_ROLE_BOTH) && callback == NULL) {
		return -EINVAL;
	}

	if (transport_type >= RTP_TRANSPORT_NUM) {
		return -EINVAL;
	}

	if (payload_type > RTP_PAYLOAD_TYPE_MAX) {
		return -EINVAL;
	}

	rtp_context = &session->rtp_context;
	rtp_context->iface = iface;

	addr_size = net_family2size(sock_addr->sa_family);
	if (addr_size == 0 || addr_size > sizeof(rtp_context->sock_addr)) {
		return -EINVAL;
	}

	memcpy(&rtp_context->sock_addr, sock_addr, addr_size);
	rtp_context->role = role;
	rtp_context->payload_type = payload_type;
	rtp_context->callback = callback;
	rtp_context->user_data = user_data;

	session->transport.type = transport_type;

	ret = rtp_transport_init(session);
	if (ret < 0) {
		return ret;
	}

	session->ssrc = sys_rand32_get();

	return k_mutex_init(&session->lock);
}

int rtp_session_start(struct rtp_session *session)
{
	enum rtp_role role;
	int ret;

	if (session == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&session->lock, API_LOCK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Failed to take session lock (%d)", ret);
		return ret;
	}

	role = session->rtp_context.role;

	if (role == RTP_ROLE_SINK || role == RTP_ROLE_BOTH) {
		ret = rtp_session_start_rx(session);
		if (ret < 0) {
			goto unlock;
		}
	}

	if (role == RTP_ROLE_SOURCE || role == RTP_ROLE_BOTH) {
		ret = rtp_session_start_tx(session);
		if (ret < 0) {
			if (role == RTP_ROLE_BOTH) {
				(void)rtp_session_stop_rx(session);
			}
		}
	}

unlock:
	(void)k_mutex_unlock(&session->lock);
	return ret;
}

int rtp_session_stop(struct rtp_session *session)
{
	enum rtp_role role;
	int ret;

	if (session == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&session->lock, API_LOCK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Failed to take session lock (%d)", ret);
		return ret;
	}

	role = session->rtp_context.role;

	if (role == RTP_ROLE_SINK || role == RTP_ROLE_BOTH) {
		ret = rtp_session_stop_rx(session);
		if (ret < 0) {
			if (role == RTP_ROLE_BOTH) {
				(void)rtp_session_stop_tx(session);
			}
			goto unlock;
		}
	}

	if (role == RTP_ROLE_SOURCE || role == RTP_ROLE_BOTH) {
		ret = rtp_session_stop_tx(session);
	}

unlock:
	(void)k_mutex_unlock(&session->lock);
	return ret;
}

int rtp_session_add_csrc(struct rtp_session *session, uint32_t csrc)
{
#if CONFIG_RTP_MAX_CSRC_COUNT == 0
	ARG_UNUSED(session);
	ARG_UNUSED(csrc);

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

	for (size_t i = 0; i < session->csrc_len; i++) {
		if (session->csrc[i] == csrc) {
			NET_DBG("0x%x already present in csrc list", csrc);
			ret = -EAGAIN;
			goto unlock;
		}
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
	size_t i;
	int ret;

	if (session == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&session->lock, API_LOCK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Failed to take session lock (%d)", ret);
		return ret;
	}

	for (i = 0; i < session->csrc_len; i++) {
		if (session->csrc[i] == csrc) {
			break;
		}
	}

	if (i == session->csrc_len) {
		ret = -EINVAL;
		goto unlock;
	}

	if (i == session->csrc_len - 1) {
		session->csrc_len--;
		ret = 0;
		goto unlock;
	}

	memmove(&session->csrc[i], &session->csrc[i + 1],
		sizeof(uint32_t) * (session->csrc_len - i - 1));
	session->csrc_len--;
	ret = 0;

unlock:
	(void)k_mutex_unlock(&session->lock);
	return ret;
#endif /* CONFIG_RTP_MAX_CSRC_COUNT == 0 */
}

int rtp_session_send(struct rtp_session *session, void *data, size_t len, uint32_t delta_ts,
		     uint8_t padding, uint8_t marker, struct rtp_header_extension *hdr_x,
		     uint32_t *timestamp)
{
	struct rtp_session_context *rtp_context;
	struct rtp_packet rtp_pkt = {};
	struct rtp_header *rtp_header = &rtp_pkt.header;
	int ret;

	if (session == NULL) {
		return -EINVAL;
	}

	if (data == NULL && len > 0) {
		return -EINVAL;
	}

	if (marker > 1) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&session->lock, API_LOCK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Failed to take session lock (%d)", ret);
		return ret;
	}
	rtp_context = &session->rtp_context;

	rtp_header_set_v(rtp_header, RTP_VERSION);
	rtp_header_set_p(rtp_header, (padding > 0) ? 1 : 0);
	rtp_header_set_cc(rtp_header, session->csrc_len);
	rtp_header_set_m(rtp_header, marker);
	rtp_header_set_pt(rtp_header, rtp_context->payload_type);

	/* RFC allows these to overflow */
	rtp_header->seq = session->sequence_number++;
	rtp_header->ts = session->timestamp;

	rtp_header->ssrc = session->ssrc;

	if (hdr_x != NULL) {
		if (hdr_x->length > 0 && hdr_x->data == NULL) {
			ret = -EINVAL;
			goto unlock;
		}

		rtp_header_set_x(rtp_header, 1);
		memcpy(&rtp_header->header_extension, hdr_x, sizeof(rtp_header->header_extension));
	}

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
	for (size_t i = 0; i < rtp_header_get_cc(rtp_header); i++) {
		rtp_header->csrc[i] = session->csrc[i];
	}
#endif /* CONFIG_RTP_MAX_CSRC_COUNT > 0 */

	rtp_pkt.payload = data;
	rtp_pkt.payload_len = len;

	ret = rtp_transport_send(session, &rtp_pkt, padding);

	if (timestamp != NULL) {
		*timestamp = session->timestamp;
	}

unlock:
	/* Timestamp is increased, regardless of whether the block is transmitted or not */
	session->timestamp += delta_ts;

	(void)k_mutex_unlock(&session->lock);
	return ret;
}
