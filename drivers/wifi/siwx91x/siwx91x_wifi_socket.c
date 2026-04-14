/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/net/net_offload.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/sys/math_extras.h>
#include <assert.h>

#include "siwx91x_nwp_bus.h"
#include "siwx91x_nwp_api.h"
#include "siwx91x_wifi.h"
#include "siwx91x_wifi_socket.h"

LOG_MODULE_DECLARE(siwx91x_wifi, CONFIG_WIFI_LOG_LEVEL);

BUILD_ASSERT(SLI_NUMBER_OF_SOCKETS < 32, "Number of sockets have to fit on uint32_t");
BUILD_ASSERT(SIWX91X_MAX_CONCURRENT_SELECT < 16, "\"select\" slots have to fit on uint16_t");

#define SIWX91X_WIFI_SOCK_TX_HEADROOM (sizeof(struct siwx91x_frame_desc) + \
				       sizeof(sli_si91x_socket_send_request_t) + \
				       NET_IPV6H_LEN + NET_TCPH_LEN)

enum offloaded_net_if_types siwx91x_sock_get_type(void)
{
	return L2_OFFLOADED_NET_IF_TYPE_WIFI;
}


int siwx91x_sock_alloc(struct net_if *iface, struct net_pkt *pkt,
		       size_t size, enum net_ip_protocol proto,
		       k_timeout_t timeout)
{
	int ret;

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	ret = net_pkt_alloc_buffer_with_reserve_debug(pkt,
						      size,
						      SIWX91X_WIFI_SOCK_TX_HEADROOM,
						      proto,
						      timeout,
						      caller,
						      line);
#else
	ret = net_pkt_alloc_buffer_with_reserve(pkt,
						size,
						SIWX91X_WIFI_SOCK_TX_HEADROOM,
						proto,
						timeout);
#endif
	return ret;
}

void siwx91x_sock_on_join_ipv4(const struct device *dev)
{
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;
	struct in_addr addr, gw, mask;
	int ret;

	if (!IS_ENABLED(CONFIG_NET_IPV4)) {
		return;
	}
	LOG_INF("Configuring IPv4");
	/* FIXME: support for static IP configuration */
	ret = siwx91x_nwp_sock_config_ipv4(config->nwp_dev, &addr, &mask, &gw);
	if (ret) {
		LOG_INF("Fail to get IPv6 configuration");
		return;
	}
	net_if_ipv4_addr_add(data->iface, &addr, NET_ADDR_DHCP, 0);
	net_if_ipv4_set_netmask_by_addr(data->iface, &addr, &mask);
	net_if_ipv4_set_gw(data->iface, &gw);
}

void siwx91x_sock_on_join_ipv6(const struct device *dev)
{
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;
	struct in6_addr lua, gua, gw;
	int prefix_len;
	int ret;

	if (!IS_ENABLED(CONFIG_NET_IPV6)) {
		return;
	}
	LOG_INF("Configuring IPv6");
	/* FIXME: support for static IP configuration */
	ret = siwx91x_nwp_sock_config_ipv6(config->nwp_dev, &lua, &gua, &prefix_len, &gw);
	if (ret) {
		LOG_INF("Fail to get IPv6 configuration");
		return;
	}
	/* NWP already take care of DAD and sending ND is not supported anyway. */
	net_if_flag_set(data->iface, NET_IF_IPV6_NO_ND);
	net_if_ipv6_addr_add(data->iface, &lua, NET_ADDR_AUTOCONF, 0);
	net_if_ipv6_addr_add(data->iface, &gua, NET_ADDR_AUTOCONF, 0);
	net_if_ipv6_prefix_add(data->iface, &gua, prefix_len, 0xFFFFFFFF);
	net_if_ipv6_router_add(data->iface, &gw, true, 0);
}

static int siwx91x_sock_get(net_sa_family_t family, enum net_sock_type type,
			    enum net_ip_protocol ip_proto, struct net_context **context)
{
	struct net_if *iface = net_if_get_first_wifi();
	const struct device *dev = net_if_get_device(iface);
	struct siwx91x_wifi_data *data = dev->data;
	int sockfd;

	k_mutex_lock(&data->sock_lock, K_FOREVER);
	sockfd = u32_count_trailing_zeros(~data->sock_bitmap);
	if (sockfd >= SLI_NUMBER_OF_SOCKETS) {
		k_mutex_unlock(&data->sock_lock);
		return -ENOBUFS;
	}
	data->sock_bitmap |= BIT(sockfd);
	k_mutex_unlock(&data->sock_lock);
	(*context)->offload_context = (void *)sockfd;
	return sockfd;
}

static int siwx91x_sock_put(struct net_context *context)
{
	struct net_if *iface = net_if_get_first_wifi();
	const struct device *dev = net_if_get_device(iface);
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;
	int sockfd = (int)context->offload_context;

	siwx91x_nwp_sock_close(config->nwp_dev, data->sock_id[sockfd]);
	k_mutex_lock(&data->sock_lock, K_FOREVER);
	if (!(data->sock_bitmap & BIT(sockfd))) {
		k_mutex_unlock(&data->sock_lock);
		return -EINVAL;
	}
	data->sock_bitmap &= ~BIT(sockfd);
	k_mutex_unlock(&data->sock_lock);
	siwx91x_nwp_sock_close(config->nwp_dev, sockfd);

	return 0;
}

void siwx91x_sock_on_select(const struct siwx91x_nwp_wifi_cb *context, int select_slot,
			    uint32_t read_fds, uint32_t write_fds)
{
	struct net_if *iface = net_if_get_first_wifi();
	const struct device *dev = net_if_get_device(iface);
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;
	uint32_t fds = 0;

	__ASSERT(select_slot == 0, "Corrupted state");
	__ASSERT(write_fds == 0, "Corrupted state");
	for (int i = 0; i < ARRAY_SIZE(data->sock_id); i++) {
		if (data->sock_watch & BIT(i) && read_fds & BIT(data->sock_id[i])) {
			fds |= BIT(i);
		}
	}
	data->sock_watch &= ~fds;
	k_event_post(&data->sock_events, fds);
	fds = 0;
	for (int i = 0; i < ARRAY_SIZE(data->sock_id); i++) {
		if (data->sock_watch & BIT(i)) {
			fds |= BIT(data->sock_id[i]);
		}
	}
	if (fds) {
		__ASSERT(0, "FIXME: launch a workqueue");
		siwx91x_nwp_sock_select(config->nwp_dev, 0, fds, 0);
	}
}

static int siwx91x_sock_wait(const struct device *dev, uint32_t sockfd, k_timeout_t timeout)
{
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;
	uint32_t fds = 0;

	data->sock_watch |= BIT(sockfd);
	for (int i = 0; i < ARRAY_SIZE(data->sock_id); i++) {
		if (data->sock_watch & BIT(i)) {
			fds |= BIT(data->sock_id[i]);
		}
	}
	siwx91x_nwp_sock_select(config->nwp_dev, 0, fds, 0);
	return k_event_wait_safe(&data->sock_events, BIT(sockfd), false, timeout);
}

static int siwx91x_sock_bind(struct net_context *context,
			     const struct sockaddr *addr, socklen_t addrlen)
{
	struct net_if *iface = net_context_get_iface(context);
	const struct device *dev = net_if_get_device(iface);
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;
	int sockfd = (int)context->offload_context;
	int ret;

	/* With TCP, we will do the real job during the call to listen() */
	if (net_context_get_type(context) == SOCK_STREAM) {
		return 0;
	}

	ret = siwx91x_nwp_sock_bind(config->nwp_dev, addr);
	if (ret) {
		return ret;
	}
	data->sock_id[sockfd] = ret;
	siwx91x_sock_wait(dev, sockfd, K_FOREVER);
	return 0;
}

static int siwx91x_sock_listen(struct net_context *context, int backlog)
{
	struct net_if *iface = net_context_get_iface(context);
	const struct device *dev = net_if_get_device(iface);
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;
	int sockfd = (int)context->offload_context;
	int ret;

	ret = siwx91x_nwp_sock_listen(config->nwp_dev, &context->local, backlog);
	if (ret < 0) {
		return ret;
	}
	data->sock_id[sockfd] = ret;
	net_context_set_state(context, NET_CONTEXT_LISTENING);
	return 0;
}

static int siwx91x_sock_connect(struct net_context *context,
				const struct sockaddr *addr, socklen_t addrlen,
				net_context_connect_cb_t cb, int32_t timeout, void *user_data)
{
	struct net_if *iface = net_context_get_iface(context);
	const struct device *dev = net_if_get_device(iface);
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;
	int sockfd = (int)context->offload_context;
	int ret;

	if (timeout == 0) {
		return -ENOTSUP;
	}
	if (net_context_get_state(context) == NET_CONTEXT_CONNECTED) {
		return -EISCONN;
	}
	ret = siwx91x_nwp_sock_connect(config->nwp_dev, net_context_get_type(context), addr);
	if (ret < 0) {
		return ret;
	}
	data->sock_id[sockfd] = ret;
	siwx91x_sock_wait(dev, sockfd, timeout < 0 ? K_FOREVER : K_MSEC(timeout));
	net_context_set_state(context, NET_CONTEXT_CONNECTED);
	if (cb) {
		cb(context, 0, user_data);
	}
	return 0;
}

static int siwx91x_sock_accept(struct net_context *context,
			       net_tcp_accept_cb_t cb, int32_t timeout, void *user_data)
{
	struct net_if *iface = net_context_get_iface(context);
	const struct device *dev = net_if_get_device(iface);
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;
	int sockfd = (int)context->offload_context;
	struct net_context *newcontext;
	int ret;

	if (timeout == 0) {
		return -ENOTSUP;
	}
	ret = net_context_get(net_context_get_family(context),
			      net_context_get_type(context),
			      net_context_get_proto(context), &newcontext);
	if (ret < 0) {
		return ret;
	}
	siwx91x_sock_wait(dev, sockfd, timeout < 0 ? K_FOREVER : K_MSEC(timeout));
	newcontext->flags |= NET_CONTEXT_REMOTE_ADDR_SET;
	ret = siwx91x_nwp_sock_accept(config->nwp_dev, data->sock_id[sockfd], &context->local,
				      &newcontext->remote);
	if (ret < 0) {
		net_context_put(newcontext);
		return ret;
	}
	data->sock_id[(int)newcontext->offload_context] = ret;
	siwx91x_sock_wait(dev, (int)newcontext->offload_context, K_FOREVER);
	net_context_set_state(context, NET_CONTEXT_CONNECTED);
	if (cb) {
		cb(newcontext, &context->remote, sizeof(context->remote), 0, user_data);
	}
	return 0;
};

static int siwx91x_sock_recv(struct net_context *context,
			     net_context_recv_cb_t cb, int32_t timeout, void *user_data)
{
	struct net_if *iface = net_context_get_iface(context);
	const struct device *dev = net_if_get_device(iface);
	const struct siwx91x_wifi_config *config = dev->config;
	int sockfd = (int)context->offload_context;
	struct net_buf *buf;
	struct net_pkt *pkt;
	int ret;

	if (timeout == 0) {
		return -ENOTSUP;
	}
	siwx91x_sock_wait(dev, sockfd, timeout < 0 ? K_FOREVER : K_MSEC(timeout));
	ret = siwx91x_nwp_sock_recv(config->nwp_dev, sockfd, &buf);
	if (ret < 0) {
		return ret;
	}
	pkt = net_pkt_rx_alloc_on_iface(iface, K_MSEC(100));
	if (!pkt) {
		net_buf_unref(buf);
		return -ENOBUFS;
	}
	net_pkt_append_buffer(pkt, buf);
	net_pkt_cursor_init(pkt);
	if (cb) {
		cb(context, pkt, NULL, NULL, 0, user_data);
	}
	return 0;
}

static int siwx91x_sock_sendto(struct net_pkt *pkt,
			       const struct sockaddr *addr, socklen_t addrlen,
			       net_context_send_cb_t cb, int32_t timeout, void *user_data)
{
	struct net_context *context = pkt->context;
	struct net_if *iface = net_context_get_iface(context);
	const struct device *dev = net_if_get_device(iface);
	const struct siwx91x_wifi_config *config = dev->config;
	int sockfd = (int)context->offload_context;
	sli_si91x_socket_send_request_t *params;
	struct sockaddr_in6 *addr6;
	struct sockaddr_in *addr4;
	int length = net_buf_frags_len(pkt->buffer);

	__ASSERT(net_buf_headroom(pkt->buffer) >= SIWX91X_WIFI_SOCK_TX_HEADROOM, "No supported");

	if (!addr) {
		addr = &context->remote;
	}
	addr6 = (struct sockaddr_in6 *)addr;
	addr4 = (struct sockaddr_in *)addr;
	net_buf_push(pkt->buffer, NET_TCPH_LEN);
	net_buf_push(pkt->buffer, NET_IPV6H_LEN);
	params = net_buf_push(pkt->buffer, sizeof(sli_si91x_socket_send_request_t));
	params->data_offset = sizeof(sli_si91x_socket_send_request_t) +
			      NET_IPV6H_LEN + NET_TCPH_LEN;
	params->socket_id = sockfd;
	params->length = length;
	switch (addr->sa_family) {
	case NET_AF_INET:
		params->ip_version = SL_IPV4_ADDRESS_LENGTH;
		params->dest_port = ntohs(addr4->sin_port);
		memcpy(params->dest_ip_addr.ipv4_address,
		       &addr4->sin_addr,
		       sizeof(addr4->sin_addr));
		break;
	case NET_AF_INET6:
		params->ip_version = SL_IPV6_ADDRESS_LENGTH;
		params->dest_port = ntohs(addr6->sin6_port);
		sys_put_be32(addr6->sin6_addr.s6_addr32[0], &params->dest_ip_addr.ipv6_address[0]);
		sys_put_be32(addr6->sin6_addr.s6_addr32[1], &params->dest_ip_addr.ipv6_address[4]);
		sys_put_be32(addr6->sin6_addr.s6_addr32[2], &params->dest_ip_addr.ipv6_address[8]);
		sys_put_be32(addr6->sin6_addr.s6_addr32[3], &params->dest_ip_addr.ipv6_address[12]);
		break;
	default:
		return -EAFNOSUPPORT;
	}
	net_buf_push(pkt->buffer, sizeof(struct siwx91x_frame_desc));
	siwx91x_nwp_send_frame(config->nwp_dev, pkt->buffer,
			       SLI_SEND_RAW_DATA, SLI_WLAN_DATA_Q,
			       SIWX91X_FRAME_FLAG_ASYNC);
	/* FIXME: In fact, the callback has to be sent once the frame has been sent over the air.
	 * However, we don't have this information.
	 */
	if (cb) {
		cb(pkt->context, 0, user_data);
	}
	net_pkt_unref(pkt);
	return 0;
}

static int siwx91x_sock_send(struct net_pkt *pkt,
			     net_context_send_cb_t cb, int32_t timeout, void *user_data)
{
	return siwx91x_sock_sendto(pkt, NULL, 0, cb, timeout, user_data);
}

static struct net_offload siwx91x_offload = {
	.get      = siwx91x_sock_get,
	.put      = siwx91x_sock_put,
	.bind     = siwx91x_sock_bind,
	.listen   = siwx91x_sock_listen,
	.connect  = siwx91x_sock_connect,
	.accept   = siwx91x_sock_accept,
	.recv     = siwx91x_sock_recv,
	.sendto   = siwx91x_sock_sendto,
	.send     = siwx91x_sock_send,
};

void siwx91x_sock_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct siwx91x_wifi_data *data = dev->data;

	k_mutex_init(&data->sock_lock);
	k_event_init(&data->sock_events);
	iface->if_dev->offload = &siwx91x_offload;
}
