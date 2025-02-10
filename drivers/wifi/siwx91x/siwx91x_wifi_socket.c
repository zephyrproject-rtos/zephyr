/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/net/net_offload.h>
#include <zephyr/logging/log.h>
#include <assert.h>

#include "siwx91x_wifi.h"
#include "siwx91x_wifi_socket.h"

#include "sl_status.h"
#include "sl_net_ip_types.h"
#include "sl_net_si91x.h"
#include "sl_si91x_types.h"
#include "sl_si91x_socket.h"
#include "sl_si91x_socket_utility.h"

LOG_MODULE_DECLARE(siwx91x_wifi);

BUILD_ASSERT(NUMBER_OF_SOCKETS < sizeof(uint32_t) * 8);
BUILD_ASSERT(NUMBER_OF_SOCKETS < SIZEOF_FIELD(sl_si91x_fd_set, __fds_bits) * 8);

NET_BUF_POOL_FIXED_DEFINE(siwx91x_tx_pool, 1, NET_ETH_MTU, 0, NULL);
NET_BUF_POOL_FIXED_DEFINE(siwx91x_rx_pool, 10, NET_ETH_MTU, 0, NULL);

enum offloaded_net_if_types siwx91x_get_type(void)
{
	return L2_OFFLOADED_NET_IF_TYPE_WIFI;
}

/* SiWx91x does not use the standard struct sockaddr (despite it uses the same
 * name):
 *   - uses Little Endian for port number while Posix uses big endian
 *   - IPv6 addresses are bytes swapped
 * Note: this function allows to have in == out.
 */
static void siwx91x_sockaddr_swap_bytes(struct sockaddr *out,
					const struct sockaddr *in, socklen_t in_len)
{
	const struct sockaddr_in6 *in6 = (const struct sockaddr_in6 *)in;
	struct sockaddr_in6 *out6 = (struct sockaddr_in6 *)out;

	/* In Zephyr, size of sockaddr == size of sockaddr_storage
	 * (while in Posix sockaddr is smaller than sockaddr_storage).
	 */
	memcpy(out, in, in_len);
	if (in->sa_family == AF_INET6) {
		ARRAY_FOR_EACH(in6->sin6_addr.s6_addr32, i) {
			out6->sin6_addr.s6_addr32[i] = ntohl(in6->sin6_addr.s6_addr32[i]);
		}
		out6->sin6_port = ntohs(in6->sin6_port);
	} else if (in->sa_family == AF_INET) {
		out6->sin6_port = ntohs(in6->sin6_port);
	}
}

void siwx91x_on_join_ipv4(struct siwx91x_dev *sidev)
{
	sl_net_ip_configuration_t ip_config4 = {
		.mode = SL_IP_MANAGEMENT_DHCP,
		.type = SL_IPV4,
	};
	struct in_addr addr4 = { };
	int ret;

	if (!IS_ENABLED(CONFIG_NET_IPV4)) {
		return;
	}
	/* FIXME: support for static IP configuration */
	ret = sl_si91x_configure_ip_address(&ip_config4, SL_SI91X_WIFI_CLIENT_VAP_ID);
	if (!ret) {
		memcpy(addr4.s4_addr, ip_config4.ip.v4.ip_address.bytes, sizeof(addr4.s4_addr));
		/* FIXME: also report gateway (net_if_ipv4_router_add()) */
		net_if_ipv4_addr_add(sidev->iface, &addr4, NET_ADDR_DHCP, 0);
	} else {
		LOG_ERR("sl_si91x_configure_ip_address(): %#04x", ret);
	}
}

void siwx91x_on_join_ipv6(struct siwx91x_dev *sidev)
{
	sl_net_ip_configuration_t ip_config6 = {
		.mode = SL_IP_MANAGEMENT_DHCP,
		.type = SL_IPV6,
	};
	struct in6_addr addr6 = { };
	int ret;

	if (!IS_ENABLED(CONFIG_NET_IPV6)) {
		return;
	}
	/* FIXME: support for static IP configuration */
	ret = sl_si91x_configure_ip_address(&ip_config6, SL_SI91X_WIFI_CLIENT_VAP_ID);
	if (!ret) {
		ARRAY_FOR_EACH(addr6.s6_addr32, i) {
			addr6.s6_addr32[i] = ntohl(ip_config6.ip.v6.global_address.value[i]);
		}
		/* SiWx91x already take care of DAD and sending ND is not
		 * supported anyway.
		 */
		net_if_flag_set(sidev->iface, NET_IF_IPV6_NO_ND);
		/* FIXME: also report gateway and link local address */
		net_if_ipv6_addr_add(sidev->iface, &addr6, NET_ADDR_AUTOCONF, 0);
	} else {
		LOG_ERR("sl_si91x_configure_ip_address(): %#04x", ret);
	}
}

static int siwx91x_sock_recv_sync(struct net_context *context,
				  net_context_recv_cb_t cb, void *user_data)
{
	struct net_if *iface = net_context_get_iface(context);
	int sockfd = (int)context->offload_context;
	struct net_pkt *pkt;
	struct net_buf *buf;
	int ret;

	pkt = net_pkt_rx_alloc_on_iface(iface, K_MSEC(100));
	if (!pkt) {
		return -ENOBUFS;
	}
	buf = net_buf_alloc(&siwx91x_rx_pool, K_MSEC(100));
	if (!buf) {
		net_pkt_unref(pkt);
		return -ENOBUFS;
	}
	net_pkt_append_buffer(pkt, buf);

	ret = sl_si91x_recvfrom(sockfd, buf->data, NET_ETH_MTU, 0, NULL, NULL);
	if (ret < 0) {
		net_pkt_unref(pkt);
		ret = -errno;
	} else {
		net_buf_add(buf, ret);
		net_pkt_cursor_init(pkt);
		ret = 0;
	}
	if (cb) {
		cb(context, pkt, NULL, NULL, ret, user_data);
	}
	return ret;
}

static void siwx91x_sock_on_recv(sl_si91x_fd_set *read_fd, sl_si91x_fd_set *write_fd,
				 sl_si91x_fd_set *except_fd, int status)
{
	/* When CONFIG_NET_SOCKETS_OFFLOAD is set, only one interface exist */
	struct siwx91x_dev *sidev = net_if_get_first_wifi()->if_dev->dev->data;

	ARRAY_FOR_EACH(sidev->fds_cb, i) {
		if (SL_SI91X_FD_ISSET(i, read_fd)) {
			if (sidev->fds_cb[i].cb) {
				siwx91x_sock_recv_sync(sidev->fds_cb[i].context,
						       sidev->fds_cb[i].cb,
						       sidev->fds_cb[i].user_data);
			} else {
				SL_SI91X_FD_CLR(i, &sidev->fds_watch);
				k_event_post(&sidev->fds_recv_event, 1U << i);
			}
		}
	}

	sl_si91x_select(NUMBER_OF_SOCKETS, &sidev->fds_watch, NULL, NULL, NULL,
			siwx91x_sock_on_recv);
}

static int siwx91x_sock_get(sa_family_t family, enum net_sock_type type,
			    enum net_ip_protocol ip_proto, struct net_context **context)
{
	struct siwx91x_dev *sidev = net_if_get_first_wifi()->if_dev->dev->data;
	int sockfd;

	sockfd = sl_si91x_socket(family, type, ip_proto);
	if (sockfd < 0) {
		return -errno;
	}
	assert(!sidev->fds_cb[sockfd].cb);
	(*context)->offload_context = (void *)sockfd;
	return sockfd;
}

static int siwx91x_sock_put(struct net_context *context)
{
	struct siwx91x_dev *sidev = net_context_get_iface(context)->if_dev->dev->data;
	int sockfd = (int)context->offload_context;
	int ret;

	SL_SI91X_FD_CLR(sockfd, &sidev->fds_watch);
	memset(&sidev->fds_cb[sockfd], 0, sizeof(sidev->fds_cb[sockfd]));
	sl_si91x_select(NUMBER_OF_SOCKETS, &sidev->fds_watch, NULL, NULL, NULL,
			siwx91x_sock_on_recv);
	ret = sl_si91x_shutdown(sockfd, 0);
	if (ret < 0) {
		ret = -errno;
	}
	return ret;
}

static int siwx91x_sock_bind(struct net_context *context,
			     const struct sockaddr *addr, socklen_t addrlen)
{
	struct siwx91x_dev *sidev = net_context_get_iface(context)->if_dev->dev->data;
	int sockfd = (int)context->offload_context;
	struct sockaddr addr_le;
	int ret;

	/* Zephyr tends to call bind() even if the TCP socket is a client. 917
	 * return an error in this case.
	 */
	if (net_context_get_proto(context) == IPPROTO_TCP &&
	    !((struct sockaddr_in *)addr)->sin_port) {
		return 0;
	}
	siwx91x_sockaddr_swap_bytes(&addr_le, addr, addrlen);
	ret = sl_si91x_bind(sockfd, &addr_le, addrlen);
	if (ret) {
		return -errno;
	}
	/* WiseConnect refuses to run select on TCP listening sockets */
	if (net_context_get_proto(context) == IPPROTO_UDP) {
		SL_SI91X_FD_SET(sockfd, &sidev->fds_watch);
		sl_si91x_select(NUMBER_OF_SOCKETS, &sidev->fds_watch, NULL, NULL, NULL,
				siwx91x_sock_on_recv);
	}
	return 0;
}

static int siwx91x_sock_connect(struct net_context *context,
				const struct sockaddr *addr, socklen_t addrlen,
				net_context_connect_cb_t cb, int32_t timeout, void *user_data)
{
	struct siwx91x_dev *sidev = net_context_get_iface(context)->if_dev->dev->data;
	int sockfd = (int)context->offload_context;
	struct sockaddr addr_le;
	int ret;

	/* sl_si91x_connect() always return immediately, so we ignore timeout */
	siwx91x_sockaddr_swap_bytes(&addr_le, addr, addrlen);
	ret = sl_si91x_connect(sockfd, &addr_le, addrlen);
	if (ret) {
		ret = -errno;
	}
	SL_SI91X_FD_SET(sockfd, &sidev->fds_watch);
	sl_si91x_select(NUMBER_OF_SOCKETS, &sidev->fds_watch, NULL, NULL, NULL,
			siwx91x_sock_on_recv);
	net_context_set_state(context, NET_CONTEXT_CONNECTED);
	if (cb) {
		cb(context, ret, user_data);
	}
	return ret;
}

static int siwx91x_sock_listen(struct net_context *context, int backlog)
{
	int sockfd = (int)context->offload_context;
	int ret;

	ret = sl_si91x_listen(sockfd, backlog);
	if (ret) {
		return -errno;
	}
	net_context_set_state(context, NET_CONTEXT_LISTENING);
	return 0;
}

static int siwx91x_sock_accept(struct net_context *context,
			       net_tcp_accept_cb_t cb, int32_t timeout, void *user_data)
{
	struct siwx91x_dev *sidev = net_context_get_iface(context)->if_dev->dev->data;
	int sockfd = (int)context->offload_context;
	struct net_context *newcontext;
	struct sockaddr addr_le;
	int ret;

	/* TODO: support timeout != K_FOREVER */
	assert(timeout < 0);

	ret = net_context_get(net_context_get_family(context),
			      net_context_get_type(context),
			      net_context_get_proto(context), &newcontext);
	if (ret < 0) {
		return ret;
	}
	/* net_context_get() calls siwx91x_sock_get() but sl_si91x_accept() also
	 * allocates a socket.
	 */
	ret = siwx91x_sock_put(newcontext);
	if (ret < 0) {
		return ret;
	}
	/* The iface is reset when getting a new context. */
	newcontext->iface = context->iface;
	ret = sl_si91x_accept(sockfd, &addr_le, sizeof(addr_le));
	if (ret < 0) {
		return -errno;
	}
	newcontext->flags |= NET_CONTEXT_REMOTE_ADDR_SET;
	newcontext->offload_context = (void *)ret;
	siwx91x_sockaddr_swap_bytes(&newcontext->remote, &addr_le, sizeof(addr_le));

	SL_SI91X_FD_SET(ret, &sidev->fds_watch);
	sl_si91x_select(NUMBER_OF_SOCKETS, &sidev->fds_watch, NULL, NULL, NULL,
			siwx91x_sock_on_recv);
	if (cb) {
		cb(newcontext, &addr_le, sizeof(addr_le), 0, user_data);
	}

	return 0;
}

static int siwx91x_sock_sendto(struct net_pkt *pkt,
			       const struct sockaddr *addr, socklen_t addrlen,
			       net_context_send_cb_t cb, int32_t timeout, void *user_data)
{
	struct net_context *context = pkt->context;
	int sockfd = (int)context->offload_context;
	struct sockaddr addr_le;
	struct net_buf *buf;
	int ret;

	/* struct net_pkt use fragmented buffers while SiWx91x API need a
	 * continuous buffer.
	 */
	if (net_pkt_get_len(pkt) > NET_ETH_MTU) {
		LOG_ERR("unexpected buffer size");
		ret = -ENOBUFS;
		goto out_cb;
	}
	buf = net_buf_alloc(&siwx91x_tx_pool, K_FOREVER);
	if (!buf) {
		ret = -ENOBUFS;
		goto out_cb;
	}
	if (net_pkt_read(pkt, buf->data, net_pkt_get_len(pkt))) {
		ret = -ENOBUFS;
		goto out_release_buf;
	}
	net_buf_add(buf, net_pkt_get_len(pkt));

	/* sl_si91x_sendto() always return immediately, so we ignore timeout */
	siwx91x_sockaddr_swap_bytes(&addr_le, addr, addrlen);
	ret = sl_si91x_sendto(sockfd, buf->data, net_pkt_get_len(pkt), 0, &addr_le, addrlen);
	if (ret < 0) {
		ret = -errno;
		goto out_release_buf;
	}
	net_pkt_unref(pkt);

out_release_buf:
	net_buf_unref(buf);

out_cb:
	if (cb) {
		cb(pkt->context, ret, user_data);
	}
	return ret;
}

static int siwx91x_sock_send(struct net_pkt *pkt,
			     net_context_send_cb_t cb, int32_t timeout, void *user_data)
{
	return siwx91x_sock_sendto(pkt, NULL, 0, cb, timeout, user_data);
}

static int siwx91x_sock_recv(struct net_context *context,
			     net_context_recv_cb_t cb, int32_t timeout, void *user_data)
{
	struct net_if *iface = net_context_get_iface(context);
	struct siwx91x_dev *sidev = iface->if_dev->dev->data;
	int sockfd = (int)context->offload_context;
	int ret;

	ret = k_event_wait(&sidev->fds_recv_event, 1U << sockfd, false,
			   timeout < 0 ? K_FOREVER : K_MSEC(timeout));
	if (timeout == 0) {
		sidev->fds_cb[sockfd].context = context;
		sidev->fds_cb[sockfd].cb = cb;
		sidev->fds_cb[sockfd].user_data = user_data;
	} else {
		memset(&sidev->fds_cb[sockfd], 0, sizeof(sidev->fds_cb[sockfd]));
	}

	if (ret) {
		k_event_clear(&sidev->fds_recv_event, 1U << sockfd);
		ret = siwx91x_sock_recv_sync(context, cb, user_data);
		SL_SI91X_FD_SET(sockfd, &sidev->fds_watch);
	}

	sl_si91x_select(NUMBER_OF_SOCKETS, &sidev->fds_watch, NULL, NULL, NULL,
			siwx91x_sock_on_recv);
	return ret;
}

static struct net_offload siwx91x_offload = {
	.get      = siwx91x_sock_get,
	.put      = siwx91x_sock_put,
	.bind     = siwx91x_sock_bind,
	.listen   = siwx91x_sock_listen,
	.connect  = siwx91x_sock_connect,
	.accept   = siwx91x_sock_accept,
	.sendto   = siwx91x_sock_sendto,
	.send     = siwx91x_sock_send,
	.recv     = siwx91x_sock_recv,
};

void siwx91x_sock_init(struct net_if *iface)
{
	struct siwx91x_dev *sidev = iface->if_dev->dev->data;

	iface->if_dev->offload = &siwx91x_offload;
	k_event_init(&sidev->fds_recv_event);
}
