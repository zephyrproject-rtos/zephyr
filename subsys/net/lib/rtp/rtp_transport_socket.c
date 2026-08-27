/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/** @file rtp_transport_socket.c
 *
 * @brief Internal functions to handle RTP transport via the socket API.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtp_socket, CONFIG_RTP_LOG_LEVEL);

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/rtp.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/sys/byteorder.h>
#ifdef CONFIG_NET_PKT_TIMESTAMP
#include <zephyr/net/ptp_time.h>
#endif

#include "rtp_transport.h"

#define CTX_LOCK_TIMEOUT   K_MSEC(1)
#define RTP_SOCKET_MAX_FDS CONFIG_RTP_TRANSPORT_SOCKET_MAX_SESSIONS

struct rtp_socket_context {
	struct zsock_pollfd fds[RTP_SOCKET_MAX_FDS];
	struct rtp_session *sessions[RTP_SOCKET_MAX_FDS];
	struct k_mutex lock;
};

static struct rtp_socket_context socket_ctx;

static void rtp_socket_svc_handler(struct net_socket_service_event *pev);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(rtp_socket_svc, rtp_socket_svc_handler, RTP_SOCKET_MAX_FDS);

static int rtp_parse_raw(struct rtp_packet *packet, uint8_t *data, size_t len)
{
	uint8_t *cursor = data;
	uint8_t *end = cursor + len;

	if (len < RTP_MIN_HEADER_LEN) {
		NET_DBG("RTP packet too small (%zu)", len);
		return -EINVAL;
	}

	packet->header.vpxcc = *cursor;
	cursor += sizeof(uint8_t);
	packet->header.mpt = *cursor;
	cursor += sizeof(uint8_t);

	packet->header.seq = sys_get_be16(cursor);
	cursor += sizeof(uint16_t);

	packet->header.ts = sys_get_be32(cursor);
	cursor += sizeof(uint32_t);

	packet->header.ssrc = sys_get_be32(cursor);
	cursor += sizeof(uint32_t);

	if (rtp_header_get_v(&packet->header) != RTP_VERSION) {
		NET_DBG("Invalid RTP version (%d)", rtp_header_get_v(&packet->header));
		return -EINVAL;
	}

	if (rtp_header_get_cc(&packet->header) > 0) {
		size_t csrc_count = rtp_header_get_cc(&packet->header);
		size_t csrc_skip = 0;

		if (end - cursor < (ptrdiff_t)(csrc_count * sizeof(uint32_t))) {
			NET_DBG("Data too small for cc");
			return -EINVAL;
		}

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
		if (csrc_count > ARRAY_SIZE(packet->header.csrc)) {
			NET_DBG("Size of csrc too small, ignoring following csrcs. Please increase "
				"CONFIG_RTP_MAX_CSRC_COUNT");
			csrc_skip = csrc_count - ARRAY_SIZE(packet->header.csrc);
			csrc_count = ARRAY_SIZE(packet->header.csrc);
			rtp_header_set_cc(&packet->header, csrc_count);
		}

		for (size_t i = 0; i < csrc_count; i++) {
			packet->header.csrc[i] = sys_get_be32(cursor);
			cursor += sizeof(uint32_t);
		}
#else
		NET_DBG("Received packet with cc > 0, but CONFIG_RTP_MAX_CSRC_COUNT = 0");

		csrc_skip = csrc_count;
		rtp_header_set_cc(&packet->header, 0);
#endif /* CONFIG_RTP_MAX_CSRC_COUNT > 0 */

		cursor += csrc_skip * sizeof(uint32_t);
	}

	if (rtp_header_get_x(&packet->header) == 1) {
		struct rtp_header_extension *hdr_x = &packet->header.header_extension;
		size_t x_data_len;

		if (end - cursor < (ptrdiff_t)(2 * sizeof(uint16_t))) {
			NET_DBG("Data too small for header extension");
			return -EINVAL;
		}

		hdr_x->definition = sys_get_be16(cursor);
		cursor += sizeof(uint16_t);

		hdr_x->length = sys_get_be16(cursor);
		cursor += sizeof(uint16_t);
		x_data_len = hdr_x->length * sizeof(uint32_t);

		if (end - cursor < (ptrdiff_t)x_data_len) {
			NET_DBG("RTP extension header length too large for pkt");
			return -EINVAL;
		}

		hdr_x->data = cursor;
		cursor += x_data_len;
	}

	packet->payload_len = end - cursor;
	packet->payload = cursor;

	if (rtp_header_get_p(&packet->header) == 1) {
		uint8_t padding;

		if (packet->payload_len == 0) {
			NET_DBG("Padding flag set but no payload");
			return -EINVAL;
		}

		padding = *(end - 1);

		if (padding == 0) {
			NET_DBG("Padding flag is set but padding is 0");
			return -EINVAL;
		}

		if (padding > packet->payload_len) {
			NET_DBG("Padding larger than payload");
			return -EINVAL;
		}

		packet->payload_len -= padding;
	}

	return 0;
}

static void rtp_socket_svc_handler(struct net_socket_service_event *pev)
{
	uint8_t data[CONFIG_RTP_TRANSPORT_SOCKET_BUF_SIZE];
	struct net_msghdr msg = {};
	struct net_iovec iov = {.iov_base = data, .iov_len = sizeof(data)};
#ifdef CONFIG_NET_PKT_TIMESTAMP
	uint8_t ctrl_buf[CMSG_SPACE(sizeof(struct net_ptp_time))] = {};
#endif
	struct rtp_packet packet = {};
	struct rtp_socket_context *ctx;
	struct rtp_session *session;
	ssize_t len;
	int ret;

	__ASSERT_NO_MSG(pev->user_data != NULL);

	if (!(pev->event.revents & ZSOCK_POLLIN)) {
		return;
	}

	ctx = (struct rtp_socket_context *)pev->user_data;
	session = NULL;

	ret = k_mutex_lock(&ctx->lock, CTX_LOCK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Failed to lock context (%d)", ret);
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ctx->fds); i++) {
		if (ctx->fds[i].fd == pev->event.fd) {
			session = ctx->sessions[i];
			break;
		}
	}

	if (session == NULL || session->rtp_context.callback == NULL) {
		goto unlock;
	}

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
#ifdef CONFIG_NET_PKT_TIMESTAMP
	msg.msg_control = ctrl_buf;
	msg.msg_controllen = sizeof(ctrl_buf);
#endif

	len = zsock_recvmsg(pev->event.fd, &msg, 0);
	if (len < 0) {
		NET_DBG("Failed to receive from socket (%d)", errno);
		goto unlock;
	}

#ifdef CONFIG_NET_PKT_TIMESTAMP
	for (struct cmsghdr *cmsg = NET_CMSG_FIRSTHDR(&msg); cmsg != NULL;
	     cmsg = NET_CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == ZSOCK_SOL_SOCKET &&
		    cmsg->cmsg_type == ZSOCK_SO_TIMESTAMPING) {
			memcpy(&packet.timestamp, CMSG_DATA(cmsg), sizeof(packet.timestamp));
			break;
		}
	}
#endif

	(void)k_mutex_unlock(&ctx->lock);

	if (rtp_parse_raw(&packet, data, len) < 0) {
		return;
	}

	session->rtp_context.callback(session, &packet, session->rtp_context.user_data);

	return;

unlock:
	(void)k_mutex_unlock(&ctx->lock);
}

int rtp_transport_socket_init(struct rtp_session *session)
{
	session->transport.socket_rx_fd = -1;
	session->transport.socket_tx_fd = -1;

	return 0;
}

int rtp_transport_socket_start_rx(struct rtp_session *session)
{
	struct net_sockaddr_storage *sock_addr = &session->rtp_context.sock_addr;
	int slot = -1;
	int fd;
	int ret;

	ret = k_mutex_lock(&socket_ctx.lock, K_FOREVER);
	if (ret < 0) {
		NET_DBG("Failed to lock context (%d)", ret);
		return ret;
	}

	if (session->transport.socket_rx_fd != -1) {
		ret = -EAGAIN;
		goto unlock;
	}

	for (size_t i = 0; i < ARRAY_SIZE(socket_ctx.fds); i++) {
		if (socket_ctx.fds[i].fd == -1) {
			slot = i;
			break;
		}
	}

	if (slot == -1) {
		NET_DBG("No free socket service slot, consider increasing "
			"CONFIG_RTP_TRANSPORT_SOCKET_MAX_SESSIONS");
		ret = -ENOMEM;
		goto unlock;
	}

	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *addr_in = net_sin(net_sad(sock_addr));
		int on = 1;

		fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
		if (fd < 0) {
			NET_DBG("Failed to create RX socket (%d)", errno);
			ret = -errno;
			goto unlock;
		}

		ret = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &on, sizeof(on));
		if (ret < 0) {
			NET_DBG("Failed to set SO_REUSEADDR (%d)", errno);
			goto close;
		}

		if (net_ipv4_is_addr_mcast(&addr_in->sin_addr)) {
			struct net_ip_mreqn mreqn = {};

			memcpy(&mreqn.imr_multiaddr, &addr_in->sin_addr,
			       sizeof(mreqn.imr_multiaddr));
			mreqn.imr_ifindex = net_if_get_by_iface(session->rtp_context.iface);

			ret = zsock_setsockopt(fd, NET_IPPROTO_IP, ZSOCK_IP_ADD_MEMBERSHIP, &mreqn,
					       sizeof(mreqn));
			if (ret < 0 && errno != EALREADY) {
				NET_DBG("Failed to join multicast group (%d)", errno);
				goto close;
			}
		}

		ret = zsock_bind(fd, net_sad(sock_addr), net_family2size(sock_addr->ss_family));
		if (ret < 0) {
			NET_DBG("Failed to bind RX socket (%d)", errno);
			goto close;
		}

		break;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *addr_in6 = net_sin6(net_sad(sock_addr));
		int on = 1;

		fd = zsock_socket(NET_AF_INET6, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
		if (fd < 0) {
			NET_DBG("Failed to create RX socket (%d)", errno);
			ret = -errno;
			goto unlock;
		}

		ret = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &on, sizeof(on));
		if (ret < 0) {
			NET_DBG("Failed to set SO_REUSEADDR (%d)", errno);
			goto close;
		}

		if (net_ipv6_is_addr_mcast(&addr_in6->sin6_addr)) {
			struct net_ipv6_mreq mreq = {
				.ipv6mr_multiaddr = addr_in6->sin6_addr,
				.ipv6mr_ifindex = net_if_get_by_iface(session->rtp_context.iface),
			};

			ret = zsock_setsockopt(fd, NET_IPPROTO_IPV6, ZSOCK_IPV6_ADD_MEMBERSHIP,
					       &mreq, sizeof(mreq));
			if (ret < 0 && errno != EALREADY) {
				NET_DBG("Failed to join multicast group (%d)", errno);
				goto close;
			}
		}

		ret = zsock_bind(fd, net_sad(sock_addr), net_family2size(sock_addr->ss_family));
		if (ret < 0) {
			NET_DBG("Failed to bind RX socket (%d)", errno);
			goto close;
		}

		break;
	}
#endif /* CONFIG_NET_IPV6 */
	default:
		NET_DBG("Family %s not supported", net_family2str(sock_addr->ss_family));
		ret = -ENOTSUP;
		goto unlock;
	}

#ifdef CONFIG_NET_PKT_TIMESTAMP
	uint8_t ts_flags = ZSOCK_SOF_TIMESTAMPING_RX_HARDWARE;

	ret = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_TIMESTAMPING, &ts_flags,
			       sizeof(ts_flags));
	if (ret < 0) {
		NET_DBG("Failed to set timestamping option (%d), continuing without", errno);
	}
#endif

	socket_ctx.fds[slot].fd = fd;
	socket_ctx.fds[slot].events = ZSOCK_POLLIN;
	socket_ctx.sessions[slot] = session;
	session->transport.socket_rx_fd = fd;

	ret = net_socket_service_register(&rtp_socket_svc, socket_ctx.fds,
					  ARRAY_SIZE(socket_ctx.fds), &socket_ctx);
	if (ret < 0) {
		NET_DBG("Failed to register socket service (%d)", ret);
		socket_ctx.fds[slot].fd = -1;
		socket_ctx.sessions[slot] = NULL;
		session->transport.socket_rx_fd = -1;
		(void)zsock_close(fd);
		goto unlock;
	}

	goto unlock;
close:
	(void)zsock_close(fd);
	ret = -errno;
unlock:
	(void)k_mutex_unlock(&socket_ctx.lock);
	return ret;
}

int rtp_transport_socket_start_tx(struct rtp_session *session)
{
	struct net_sockaddr_storage *sock_addr = &session->rtp_context.sock_addr;
	int fd;
	int ret;

	if (session->transport.socket_tx_fd != -1) {
		return -EAGAIN;
	}

	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_IPV4
	case NET_AF_INET: {
		fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
		if (fd < 0) {
			NET_DBG("Failed to create TX socket (%d)", errno);
			return -errno;
		}

		if (net_ipv4_is_addr_mcast(&net_sin(net_sad(sock_addr))->sin_addr)) {
			struct net_ip_mreqn mreqn = {};
			int ttl = CONFIG_RTP_MCAST_TTL;

			mreqn.imr_ifindex = net_if_get_by_iface(session->rtp_context.iface);

			ret = zsock_setsockopt(fd, NET_IPPROTO_IP, ZSOCK_IP_MULTICAST_IF, &mreqn,
					       sizeof(mreqn));
			if (ret < 0) {
				NET_DBG("Failed to set IP_MULTICAST_IF (%d)", errno);
				goto close;
			}

			ret = zsock_setsockopt(fd, NET_IPPROTO_IP, ZSOCK_IP_MULTICAST_TTL, &ttl,
					       sizeof(ttl));
			if (ret < 0) {
				NET_DBG("Failed to set multicast ttl (%d)", errno);
				goto close;
			}
		}

		ret = zsock_connect(fd, net_sad(sock_addr), net_family2size(sock_addr->ss_family));
		if (ret < 0) {
			NET_DBG("Failed to connect TX socket (%d)", errno);
			goto close;
		}

		break;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *addr_in6 = net_sin6(net_sad(sock_addr));

		fd = zsock_socket(NET_AF_INET6, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
		if (fd < 0) {
			NET_DBG("Failed to create TX socket (%d)", errno);
			return -errno;
		}

		if (net_ipv6_is_addr_mcast(&addr_in6->sin6_addr)) {
			int ifindex = net_if_get_by_iface(session->rtp_context.iface);
			int hops = CONFIG_RTP_MCAST_TTL;

			ret = zsock_setsockopt(fd, NET_IPPROTO_IPV6, ZSOCK_IPV6_MULTICAST_IF,
					       &ifindex, sizeof(ifindex));
			if (ret < 0) {
				NET_DBG("Failed to set IPV6_MULTICAST_IF (%d)", errno);
				goto close;
			}

			ret = zsock_setsockopt(fd, NET_IPPROTO_IPV6, ZSOCK_IPV6_MULTICAST_HOPS,
					       &hops, sizeof(hops));
			if (ret < 0) {
				NET_DBG("Failed to set multicast hop limit (%d)", errno);
				goto close;
			}
		}

		ret = zsock_connect(fd, net_sad(sock_addr), net_family2size(sock_addr->ss_family));
		if (ret < 0) {
			NET_DBG("Failed to connect TX socket (%d)", errno);
			goto close;
		}

		break;
	}
#endif /* CONFIG_NET_IPV6 */
	default:
		NET_DBG("Family %s not supported", net_family2str(sock_addr->ss_family));
		return -ENOTSUP;
	}

	session->transport.socket_tx_fd = fd;

	return 0;
close:
	(void)zsock_close(fd);
	return -errno;
}

int rtp_transport_socket_stop_rx(struct rtp_session *session)
{
	struct net_sockaddr_storage *sock_addr = &session->rtp_context.sock_addr;
	int fd = session->transport.socket_rx_fd;
	int ret;

	if (fd < 0) {
		return 0;
	}

	switch (sock_addr->ss_family) {
#ifdef CONFIG_NET_IPV4
	case NET_AF_INET: {
		if (net_ipv4_is_addr_mcast(&net_sin(net_sad(sock_addr))->sin_addr)) {
			struct net_ip_mreqn mreqn = {};

			memcpy(&mreqn.imr_multiaddr, &net_sin(net_sad(sock_addr))->sin_addr,
			       sizeof(mreqn.imr_multiaddr));
			mreqn.imr_ifindex = net_if_get_by_iface(session->rtp_context.iface);

			ret = zsock_setsockopt(fd, NET_IPPROTO_IP, ZSOCK_IP_DROP_MEMBERSHIP, &mreqn,
					       sizeof(mreqn));
			if (ret < 0 && errno != EALREADY) {
				NET_DBG("Failed to leave multicast group (%d)", errno);
			}
		}
		break;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *addr_in6 = net_sin6(net_sad(sock_addr));

		if (net_ipv6_is_addr_mcast(&addr_in6->sin6_addr)) {
			struct net_ipv6_mreq mreq = {
				.ipv6mr_multiaddr = addr_in6->sin6_addr,
				.ipv6mr_ifindex = net_if_get_by_iface(session->rtp_context.iface),
			};

			ret = zsock_setsockopt(fd, NET_IPPROTO_IPV6, ZSOCK_IPV6_DROP_MEMBERSHIP,
					       &mreq, sizeof(mreq));
			if (ret < 0 && errno != EALREADY) {
				NET_DBG("Failed to leave multicast group (%d)", errno);
			}
		}
		break;
	}
#endif /* CONFIG_NET_IPV6 */
	default:
		NET_DBG("Family %s not supported", net_family2str(sock_addr->ss_family));
		return -ENOTSUP;
	}

	ret = k_mutex_lock(&socket_ctx.lock, K_FOREVER);
	if (ret < 0) {
		NET_DBG("Failed to lock context (%d)", ret);
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(socket_ctx.fds); i++) {
		if (socket_ctx.fds[i].fd == fd) {
			socket_ctx.fds[i].fd = -1;
			socket_ctx.sessions[i] = NULL;
			break;
		}
	}

	ret = net_socket_service_register(&rtp_socket_svc, socket_ctx.fds,
					  ARRAY_SIZE(socket_ctx.fds), &socket_ctx);
	if (ret < 0) {
		NET_DBG("Failed to re-register socket service (%d)", ret);
	}

	(void)k_mutex_unlock(&socket_ctx.lock);

	ret = zsock_close(fd);
	if (ret < 0) {
		NET_DBG("Failed to close socket (%d)", errno);
		return -errno;
	}

	session->transport.socket_rx_fd = -1;

	return ret;
}

int rtp_transport_socket_stop_tx(struct rtp_session *session)
{
	int fd = session->transport.socket_tx_fd;
	int ret;

	if (fd < 0) {
		return 0;
	}

	ret = zsock_close(fd);
	if (ret < 0) {
		NET_DBG("Failed to close socket (%d)", errno);
		return -errno;
	}

	session->transport.socket_tx_fd = -1;

	return 0;
}

int rtp_transport_socket_send(struct rtp_session *session, struct rtp_packet *rtp_pkt,
			      uint8_t padding)
{
	int fd = session->transport.socket_tx_fd;
	uint8_t buf[CONFIG_RTP_TRANSPORT_SOCKET_BUF_SIZE];
	const uint8_t *end = buf + sizeof(buf);
	uint8_t *cursor = buf;
	ssize_t ret;

	if (fd < 0) {
		NET_DBG("TX socket not open");
		return -ENOTCONN;
	}

	*cursor = rtp_pkt->header.vpxcc;
	cursor += sizeof(uint8_t);
	*cursor = rtp_pkt->header.mpt;
	cursor += sizeof(uint8_t);

	sys_put_be16(rtp_pkt->header.seq, cursor);
	cursor += sizeof(uint16_t);

	sys_put_be32(rtp_pkt->header.ts, cursor);
	cursor += sizeof(uint32_t);

	sys_put_be32(rtp_pkt->header.ssrc, cursor);
	cursor += sizeof(uint32_t);

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
	if (rtp_header_get_cc(&rtp_pkt->header) > CONFIG_RTP_MAX_CSRC_COUNT) {
		NET_DBG("CSRC count %u exceeds maximum %d", rtp_header_get_cc(&rtp_pkt->header),
			CONFIG_RTP_MAX_CSRC_COUNT);
		return -EINVAL;
	}

	if (end - cursor <
	    (ptrdiff_t)((size_t)rtp_header_get_cc(&rtp_pkt->header) * sizeof(uint32_t))) {
		NET_DBG("Not enough buffer space for CSRC list");
		return -EINVAL;
	}

	for (size_t i = 0; i < rtp_header_get_cc(&rtp_pkt->header); i++) {
		sys_put_be32(rtp_pkt->header.csrc[i], cursor);
		cursor += sizeof(uint32_t);
	}
#endif

	if (rtp_header_get_x(&rtp_pkt->header) == 1) {
		struct rtp_header_extension *hdr_x = &rtp_pkt->header.header_extension;
		size_t x_data_len = (size_t)hdr_x->length * sizeof(uint32_t);

		if (hdr_x->length > 0 && hdr_x->data == NULL) {
			NET_DBG("Extension header data pointer is NULL");
			return -EINVAL;
		}

		if (end - cursor < (ptrdiff_t)(2 * sizeof(uint16_t) + x_data_len)) {
			NET_DBG("Extension header length too large");
			return -EINVAL;
		}

		sys_put_be16(hdr_x->definition, cursor);
		cursor += sizeof(uint16_t);

		sys_put_be16(hdr_x->length, cursor);
		cursor += sizeof(uint16_t);

		memcpy(cursor, hdr_x->data, x_data_len);
		cursor += x_data_len;
	}

	if (rtp_pkt->payload_len > 0) {
		if (rtp_pkt->payload == NULL) {
			NET_DBG("Payload pointer is NULL with non-zero length");
			return -EINVAL;
		}

		if (end - cursor < (ptrdiff_t)rtp_pkt->payload_len) {
			NET_DBG("Payload of len %zu too large", rtp_pkt->payload_len);
			return -EINVAL;
		}

		memcpy(cursor, rtp_pkt->payload, rtp_pkt->payload_len);
		cursor += rtp_pkt->payload_len;
	}

	if (padding > 0) {
		if (end - cursor < (ptrdiff_t)padding) {
			NET_DBG("Padding of size %u too large", padding);
			return -EINVAL;
		}

		memset(cursor, 0, padding - 1);
		cursor += padding - 1;
		*cursor++ = padding;
	}

	ret = zsock_send(fd, buf, cursor - buf, 0);
	if (ret < 0) {
		NET_DBG("Failed to send RTP packet (%d)", errno);
		return -errno;
	}

	return 0;
}

static int rtp_socket_init(void)
{
	k_mutex_init(&socket_ctx.lock);

	for (size_t i = 0; i < ARRAY_SIZE(socket_ctx.fds); i++) {
		socket_ctx.fds[i].fd = -1;
	}

	return 0;
}

SYS_INIT(rtp_socket_init, POST_KERNEL, CONFIG_RTP_TRANSPORT_SOCKET_INIT_PRIO);

BUILD_ASSERT(CONFIG_RTP_TRANSPORT_SOCKET_BUF_SIZE >= RTP_MIN_HEADER_LEN);
